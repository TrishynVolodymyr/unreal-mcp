#include "Commands/BlueprintAction/FindInBlueprintsCommand.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "WidgetBlueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_FunctionEntry.h"
#include "EdGraphSchema_K2.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Services/AssetDiscoveryService.h"
#include "Utils/GraphUtils.h"

FFindInBlueprintsCommand::FFindInBlueprintsCommand(TSharedPtr<IBlueprintActionService> InBlueprintActionService)
	: BlueprintActionService(InBlueprintActionService)
{
}

FString FFindInBlueprintsCommand::Execute(const FString& Parameters)
{
	FString SearchQuery;
	EBlueprintSearchType SearchType;
	FString Path;
	int32 MaxResults;
	bool bCaseSensitive;
	FString ParseError;

	if (!ParseParameters(Parameters, SearchQuery, SearchType, Path, MaxResults, bCaseSensitive, ParseError))
	{
		return CreateErrorResponse(ParseError);
	}

	// Find all blueprints in the specified path
	TArray<FString> BlueprintPaths = FAssetDiscoveryService::Get().FindBlueprints(TEXT(""), Path);

	TArray<FBlueprintSearchMatch> AllMatches;
	int32 BlueprintsSearched = 0;

	for (const FString& BPPath : BlueprintPaths)
	{
		// Load the blueprint
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BPPath);
		if (!Blueprint)
		{
			continue;
		}

		BlueprintsSearched++;

		// Search this blueprint
		SearchBlueprint(Blueprint, SearchQuery, SearchType, bCaseSensitive, AllMatches, MaxResults);

		// Stop if we have enough results
		if (AllMatches.Num() >= MaxResults)
		{
			break;
		}
	}

	// Convert search type to string for response
	FString SearchTypeStr;
	switch (SearchType)
	{
		case EBlueprintSearchType::Function: SearchTypeStr = TEXT("function"); break;
		case EBlueprintSearchType::Variable: SearchTypeStr = TEXT("variable"); break;
		case EBlueprintSearchType::Event: SearchTypeStr = TEXT("event"); break;
		case EBlueprintSearchType::Comment: SearchTypeStr = TEXT("comment"); break;
		case EBlueprintSearchType::Custom: SearchTypeStr = TEXT("custom"); break;
		default: SearchTypeStr = TEXT("all"); break;
	}

	return CreateSuccessResponse(AllMatches, SearchQuery, SearchTypeStr, BlueprintsSearched);
}

FString FFindInBlueprintsCommand::GetCommandName() const
{
	return TEXT("find_in_blueprints");
}

bool FFindInBlueprintsCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	// Check required parameter
	FString SearchQuery;
	return JsonObject->TryGetStringField(TEXT("search_query"), SearchQuery) && !SearchQuery.IsEmpty();
}

bool FFindInBlueprintsCommand::ParseParameters(
	const FString& Parameters,
	FString& OutSearchQuery,
	EBlueprintSearchType& OutSearchType,
	FString& OutPath,
	int32& OutMaxResults,
	bool& OutCaseSensitive,
	FString& OutError) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		OutError = TEXT("Invalid JSON parameters");
		return false;
	}

	// Parse required parameter
	OutSearchQuery = JsonObject->GetStringField(TEXT("search_query"));
	if (OutSearchQuery.IsEmpty())
	{
		OutError = TEXT("search_query is required and cannot be empty");
		return false;
	}

	// Parse optional parameters
	FString SearchTypeStr = JsonObject->GetStringField(TEXT("search_type"));
	OutSearchType = ParseSearchType(SearchTypeStr);

	OutPath = JsonObject->GetStringField(TEXT("path"));
	if (OutPath.IsEmpty())
	{
		OutPath = TEXT("/Game");
	}

	// Parse max_results with default value
	if (JsonObject->HasField(TEXT("max_results")))
	{
		OutMaxResults = JsonObject->GetIntegerField(TEXT("max_results"));
	}
	else
	{
		OutMaxResults = 50; // Default value
	}

	// Validate max_results
	if (OutMaxResults <= 0 || OutMaxResults > 500)
	{
		OutError = TEXT("max_results must be between 1 and 500");
		return false;
	}

	// Parse case_sensitive with default false
	OutCaseSensitive = false;
	if (JsonObject->HasField(TEXT("case_sensitive")))
	{
		OutCaseSensitive = JsonObject->GetBoolField(TEXT("case_sensitive"));
	}

	return true;
}

EBlueprintSearchType FFindInBlueprintsCommand::ParseSearchType(const FString& TypeString) const
{
	if (TypeString.IsEmpty() || TypeString.Equals(TEXT("all"), ESearchCase::IgnoreCase))
	{
		return EBlueprintSearchType::All;
	}
	else if (TypeString.Equals(TEXT("function"), ESearchCase::IgnoreCase))
	{
		return EBlueprintSearchType::Function;
	}
	else if (TypeString.Equals(TEXT("variable"), ESearchCase::IgnoreCase))
	{
		return EBlueprintSearchType::Variable;
	}
	else if (TypeString.Equals(TEXT("event"), ESearchCase::IgnoreCase))
	{
		return EBlueprintSearchType::Event;
	}
	else if (TypeString.Equals(TEXT("comment"), ESearchCase::IgnoreCase))
	{
		return EBlueprintSearchType::Comment;
	}
	else if (TypeString.Equals(TEXT("custom"), ESearchCase::IgnoreCase))
	{
		return EBlueprintSearchType::Custom;
	}

	return EBlueprintSearchType::All;
}

void FFindInBlueprintsCommand::SearchBlueprint(
	UBlueprint* Blueprint,
	const FString& SearchQuery,
	EBlueprintSearchType SearchType,
	bool bCaseSensitive,
	TArray<FBlueprintSearchMatch>& OutMatches,
	int32 MaxResults) const
{
	if (!Blueprint)
	{
		return;
	}

	// Collect all graphs
	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			// Check max results
			if (OutMatches.Num() >= MaxResults)
			{
				return;
			}

			FString MatchContext;
			if (MatchesSearchCriteria(Node, SearchQuery, SearchType, bCaseSensitive, MatchContext))
			{
				FBlueprintSearchMatch Match;
				Match.BlueprintPath = Blueprint->GetPathName();
				Match.BlueprintName = Blueprint->GetName();
				Match.GraphName = Graph->GetName();
				Match.NodeId = FGraphUtils::GetReliableNodeId(Node);
				Match.NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
				Match.NodeClass = Node->GetClass()->GetName();
				Match.MatchContext = MatchContext;

				OutMatches.Add(Match);
			}
		}
	}
}

bool FFindInBlueprintsCommand::MatchesSearchCriteria(
	UEdGraphNode* Node,
	const FString& SearchQuery,
	EBlueprintSearchType SearchType,
	bool bCaseSensitive,
	FString& OutMatchContext) const
{
	if (!Node)
	{
		return false;
	}

	// First check if node type matches the filter
	if (!MatchesNodeTypeFilter(Node, SearchType))
	{
		return false;
	}

	// Get searchable strings from the node
	TArray<FString> SearchableStrings;

	// Node title
	FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
	SearchableStrings.Add(NodeTitle);

	// Full title
	FString FullTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
	if (FullTitle != NodeTitle)
	{
		SearchableStrings.Add(FullTitle);
	}

	// For function calls, get the function name
	if (UK2Node_CallFunction* CallFunc = Cast<UK2Node_CallFunction>(Node))
	{
		FName FuncName = CallFunc->FunctionReference.GetMemberName();
		if (!FuncName.IsNone())
		{
			SearchableStrings.Add(FuncName.ToString());
		}
	}

	// For variable nodes, get the variable name
	if (UK2Node_VariableGet* VarGet = Cast<UK2Node_VariableGet>(Node))
	{
		FName VarName = VarGet->VariableReference.GetMemberName();
		if (!VarName.IsNone())
		{
			SearchableStrings.Add(VarName.ToString());
		}
	}
	else if (UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(Node))
	{
		FName VarName = VarSet->VariableReference.GetMemberName();
		if (!VarName.IsNone())
		{
			SearchableStrings.Add(VarName.ToString());
		}
	}

	// For events, get the event name
	if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
	{
		FName EventName = EventNode->GetFunctionName();
		if (!EventName.IsNone())
		{
			SearchableStrings.Add(EventName.ToString());
		}
	}

	// For custom events, get the custom event name
	if (UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(Node))
	{
		FString CustomEventName = CustomEvent->CustomFunctionName.ToString();
		if (!CustomEventName.IsEmpty())
		{
			SearchableStrings.Add(CustomEventName);
		}
	}

	// Check comment nodes
	if (Node->GetClass()->GetName().Contains(TEXT("Comment")))
	{
		FString CommentText = Node->NodeComment;
		if (!CommentText.IsEmpty())
		{
			SearchableStrings.Add(CommentText);
		}
	}

	// Search through all strings
	ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;

	for (const FString& Searchable : SearchableStrings)
	{
		if (Searchable.Contains(SearchQuery, SearchCase))
		{
			OutMatchContext = Searchable;
			return true;
		}
	}

	return false;
}

bool FFindInBlueprintsCommand::MatchesNodeTypeFilter(UEdGraphNode* Node, EBlueprintSearchType SearchType) const
{
	if (SearchType == EBlueprintSearchType::All)
	{
		return true;
	}

	FString NodeClassName = Node->GetClass()->GetName();

	switch (SearchType)
	{
		case EBlueprintSearchType::Function:
			return Node->IsA<UK2Node_CallFunction>() ||
				   Node->IsA<UK2Node_FunctionEntry>() ||
				   NodeClassName.Contains(TEXT("CallFunction"));

		case EBlueprintSearchType::Variable:
			return Node->IsA<UK2Node_VariableGet>() ||
				   Node->IsA<UK2Node_VariableSet>() ||
				   NodeClassName.Contains(TEXT("Variable"));

		case EBlueprintSearchType::Event:
			return Node->IsA<UK2Node_Event>();

		case EBlueprintSearchType::Custom:
			return Node->IsA<UK2Node_CustomEvent>();

		case EBlueprintSearchType::Comment:
			return NodeClassName.Contains(TEXT("Comment"));

		default:
			return true;
	}
}

FString FFindInBlueprintsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), false);
	ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

	return OutputString;
}

FString FFindInBlueprintsCommand::CreateSuccessResponse(
	const TArray<FBlueprintSearchMatch>& Matches,
	const FString& SearchQuery,
	const FString& SearchType,
	int32 BlueprintsSearched) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), true);
	ResponseObj->SetStringField(TEXT("search_query"), SearchQuery);
	ResponseObj->SetStringField(TEXT("search_type"), SearchType);
	ResponseObj->SetNumberField(TEXT("blueprints_searched"), BlueprintsSearched);
	ResponseObj->SetNumberField(TEXT("match_count"), Matches.Num());

	// Build matches array
	TArray<TSharedPtr<FJsonValue>> MatchesArray;
	for (const FBlueprintSearchMatch& Match : Matches)
	{
		TSharedPtr<FJsonObject> MatchObj = MakeShared<FJsonObject>();
		MatchObj->SetStringField(TEXT("blueprint_path"), Match.BlueprintPath);
		MatchObj->SetStringField(TEXT("blueprint_name"), Match.BlueprintName);
		MatchObj->SetStringField(TEXT("graph_name"), Match.GraphName);
		MatchObj->SetStringField(TEXT("node_id"), Match.NodeId);
		MatchObj->SetStringField(TEXT("node_title"), Match.NodeTitle);
		MatchObj->SetStringField(TEXT("node_class"), Match.NodeClass);
		MatchObj->SetStringField(TEXT("match_context"), Match.MatchContext);

		MatchesArray.Add(MakeShared<FJsonValueObject>(MatchObj));
	}

	ResponseObj->SetArrayField(TEXT("matches"), MatchesArray);

	// Group results by blueprint for easier consumption
	TMap<FString, TArray<const FBlueprintSearchMatch*>> ByBlueprint;
	for (const FBlueprintSearchMatch& Match : Matches)
	{
		ByBlueprint.FindOrAdd(Match.BlueprintName).Add(&Match);
	}

	TArray<TSharedPtr<FJsonValue>> GroupedArray;
	for (const auto& Pair : ByBlueprint)
	{
		TSharedPtr<FJsonObject> GroupObj = MakeShared<FJsonObject>();
		GroupObj->SetStringField(TEXT("blueprint_name"), Pair.Key);
		GroupObj->SetNumberField(TEXT("match_count"), Pair.Value.Num());

		if (Pair.Value.Num() > 0)
		{
			GroupObj->SetStringField(TEXT("blueprint_path"), Pair.Value[0]->BlueprintPath);
		}

		TArray<TSharedPtr<FJsonValue>> NodeMatches;
		for (const FBlueprintSearchMatch* Match : Pair.Value)
		{
			TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
			NodeObj->SetStringField(TEXT("graph"), Match->GraphName);
			NodeObj->SetStringField(TEXT("node_id"), Match->NodeId);
			NodeObj->SetStringField(TEXT("title"), Match->NodeTitle);
			NodeObj->SetStringField(TEXT("context"), Match->MatchContext);

			NodeMatches.Add(MakeShared<FJsonValueObject>(NodeObj));
		}
		GroupObj->SetArrayField(TEXT("nodes"), NodeMatches);

		GroupedArray.Add(MakeShared<FJsonValueObject>(GroupObj));
	}

	ResponseObj->SetArrayField(TEXT("by_blueprint"), GroupedArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

	return OutputString;
}
