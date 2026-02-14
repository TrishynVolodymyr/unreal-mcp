#include "Commands/PCG/SearchPCGPaletteCommand.h"
#include "PCGSettings.h"
#include "PCGCommon.h"
#include "UObject/UObjectIterator.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSearchPCGPaletteCommand::FSearchPCGPaletteCommand()
{
}

FString FSearchPCGPaletteCommand::GetCommandName() const
{
	return TEXT("search_pcg_palette");
}

bool FSearchPCGPaletteCommand::ValidateParams(const FString& Parameters) const
{
	// All parameters are optional, so always valid
	return true;
}

FString FSearchPCGPaletteCommand::SettingsTypeToString(uint8 TypeValue)
{
	// Maps EPCGSettingsType enum values to human-readable strings.
	// Order must match the EPCGSettingsType enum declaration in PCGSettings.h.
	switch (TypeValue)
	{
	case 0:  return TEXT("InputOutput");
	case 1:  return TEXT("Spatial");
	case 2:  return TEXT("Density");
	case 3:  return TEXT("Blueprint");
	case 4:  return TEXT("Metadata");
	case 5:  return TEXT("Filter");
	case 6:  return TEXT("Sampler");
	case 7:  return TEXT("Spawner");
	case 8:  return TEXT("Subgraph");
	case 9:  return TEXT("Debug");
	case 10: return TEXT("Generic");
	case 11: return TEXT("Param");
	case 12: return TEXT("HierarchicalGeneration");
	case 13: return TEXT("ControlFlow");
	case 14: return TEXT("PointOps");
	case 15: return TEXT("GraphParameters");
	case 16: return TEXT("Reroute");
	case 17: return TEXT("GPU");
	case 18: return TEXT("DynamicMesh");
	case 19: return TEXT("DataLayers");
	case 20: return TEXT("Resource");
	default: return TEXT("Unknown");
	}
}

FString FSearchPCGPaletteCommand::Execute(const FString& Parameters)
{
	// Parse parameters
	TSharedPtr<FJsonObject> JsonParams;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonParams) || !JsonParams.IsValid())
	{
		return CreateErrorResponse(TEXT("Invalid JSON parameters"));
	}

	FString SearchQuery = JsonParams->HasField(TEXT("search_query"))
		? JsonParams->GetStringField(TEXT("search_query")) : TEXT("");
	int32 MaxResults = JsonParams->HasField(TEXT("max_results"))
		? (int32)JsonParams->GetNumberField(TEXT("max_results")) : 50;

	// Struct to hold results for sorting
	struct FPCGPaletteEntry
	{
		FString ClassName;
		FString DisplayName;
		FString Category;
		FString Description;
	};

	TArray<FPCGPaletteEntry> MatchingEntries;
	int32 TotalAvailable = 0;

	// Iterate all UClass subclasses of UPCGSettings
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;

		// Must be a child of UPCGSettings
		if (!Class->IsChildOf(UPCGSettings::StaticClass()))
			continue;

		// Skip abstract and deprecated classes
		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
			continue;

		// Skip UPCGSettings itself
		if (Class == UPCGSettings::StaticClass())
			continue;

		TotalAvailable++;

		// Get class name
		FString ClassName = Class->GetName();

		// Get display name
		FString DisplayName = Class->GetDisplayNameText().ToString();

		// Get CDO for type/category and description
		FString Category = TEXT("Generic");
		FString Description;

		UPCGSettings* CDO = Class->GetDefaultObject<UPCGSettings>();
		if (CDO)
		{
			// Get type via GetType() which returns EPCGSettingsType
			EPCGSettingsType SettingsType = CDO->GetType();
			Category = SettingsTypeToString(static_cast<uint8>(SettingsType));

			// Get description from node tooltip
			FText TooltipText = CDO->GetNodeTooltipText();
			if (!TooltipText.IsEmpty())
			{
				Description = TooltipText.ToString();
			}

			// Try default node title as a better display name if available
			FText NodeTitle = CDO->GetDefaultNodeTitle();
			if (!NodeTitle.IsEmpty())
			{
				DisplayName = NodeTitle.ToString();
			}
		}

		// Apply search filter — split query into tokens and match ALL
		if (!SearchQuery.IsEmpty())
		{
			TArray<FString> SearchTokens;
			SearchQuery.ParseIntoArray(SearchTokens, TEXT(" "), true);

			bool bMatchesAllTokens = true;
			for (const FString& Token : SearchTokens)
			{
				if (Token.IsEmpty())
					continue;

				bool bTokenFound = DisplayName.Contains(Token, ESearchCase::IgnoreCase);
				if (!bTokenFound)
				{
					bTokenFound = ClassName.Contains(Token, ESearchCase::IgnoreCase);
				}
				if (!bTokenFound)
				{
					bTokenFound = Category.Contains(Token, ESearchCase::IgnoreCase);
				}
				if (!bTokenFound && !Description.IsEmpty())
				{
					bTokenFound = Description.Contains(Token, ESearchCase::IgnoreCase);
				}

				if (!bTokenFound)
				{
					bMatchesAllTokens = false;
					break;
				}
			}

			if (!bMatchesAllTokens)
				continue;
		}

		FPCGPaletteEntry Entry;
		Entry.ClassName = ClassName;
		Entry.DisplayName = DisplayName;
		Entry.Category = Category;
		Entry.Description = Description;
		MatchingEntries.Add(Entry);
	}

	// Sort by display name
	MatchingEntries.Sort([](const FPCGPaletteEntry& A, const FPCGPaletteEntry& B)
	{
		return A.DisplayName.Compare(B.DisplayName, ESearchCase::IgnoreCase) < 0;
	});

	// Build JSON results array (capped to MaxResults)
	TArray<TSharedPtr<FJsonValue>> Results;
	int32 ResultCount = FMath::Min(MatchingEntries.Num(), MaxResults);

	for (int32 i = 0; i < ResultCount; ++i)
	{
		const FPCGPaletteEntry& Entry = MatchingEntries[i];

		TSharedPtr<FJsonObject> ItemObj = MakeShared<FJsonObject>();
		ItemObj->SetStringField(TEXT("class_name"), Entry.ClassName);
		ItemObj->SetStringField(TEXT("display_name"), Entry.DisplayName);
		ItemObj->SetStringField(TEXT("category"), Entry.Category);
		ItemObj->SetStringField(TEXT("description"), Entry.Description);

		Results.Add(MakeShared<FJsonValueObject>(ItemObj));
	}

	// Build response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetArrayField(TEXT("results"), Results);
	ResultObj->SetNumberField(TEXT("result_count"), Results.Num());
	ResultObj->SetNumberField(TEXT("total_available"), TotalAvailable);
	ResultObj->SetStringField(TEXT("message"),
		FString::Printf(TEXT("Found %d matching PCG node types (showing %d of %d total)"),
			MatchingEntries.Num(), Results.Num(), TotalAvailable));

	return CreateSuccessResponse(ResultObj);
}

FString FSearchPCGPaletteCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
	ErrorObj->SetBoolField(TEXT("success"), false);
	ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

	return OutputString;
}

FString FSearchPCGPaletteCommand::CreateSuccessResponse(TSharedPtr<FJsonObject> ResultObj) const
{
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);

	return OutputString;
}
