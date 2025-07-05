#include "Commands/UnrealMCPNodeCreationHelpers.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EdGraphSchema_K2.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "BlueprintNodeBinder.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "Framework/Commands/UIAction.h"
#include "Engine/Engine.h"

bool UnrealMCPNodeCreationHelpers::ParseJsonParameters(const FString& JsonParams, TSharedPtr<FJsonObject>& OutParamsObject, TSharedPtr<FJsonObject>& OutResultObj)
{
    if (!JsonParams.IsEmpty())
    {
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonParams);
        if (!FJsonSerializer::Deserialize(Reader, OutParamsObject) || !OutParamsObject.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Failed to parse JSON parameters"));
            OutResultObj->SetBoolField(TEXT("success"), false);
            OutResultObj->SetStringField(TEXT("message"), TEXT("Invalid JSON parameters"));
            return false;
        }
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully parsed JSON parameters"));
    }
    return true;
}

void UnrealMCPNodeCreationHelpers::ParseNodePosition(const FString& NodePosition, int32& OutPositionX, int32& OutPositionY)
{
    OutPositionX = 0;
    OutPositionY = 0;
    
    if (!NodePosition.IsEmpty())
    {
        // Try to parse as JSON array [x, y] first (from Python)
        TSharedPtr<FJsonValue> JsonValue;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NodePosition);
        
        if (FJsonSerializer::Deserialize(Reader, JsonValue) && JsonValue.IsValid())
        {
            const TArray<TSharedPtr<FJsonValue>>* JsonArray = nullptr;
            if (JsonValue->TryGetArray(JsonArray) && JsonArray->Num() >= 2)
            {
                OutPositionX = FMath::RoundToInt((*JsonArray)[0]->AsNumber());
                OutPositionY = FMath::RoundToInt((*JsonArray)[1]->AsNumber());
                return;
            }
        }
        
        // Fallback: parse as string format "[x, y]" or "x,y"
        FString CleanPosition = NodePosition;
        CleanPosition = CleanPosition.Replace(TEXT("["), TEXT(""));
        CleanPosition = CleanPosition.Replace(TEXT("]"), TEXT(""));
        
        TArray<FString> Coords;
        CleanPosition.ParseIntoArray(Coords, TEXT(","));
        
        if (Coords.Num() == 2)
        {
            OutPositionX = FCString::Atoi(*Coords[0].TrimStartAndEnd());
            OutPositionY = FCString::Atoi(*Coords[1].TrimStartAndEnd());
        }
    }
}

UClass* UnrealMCPNodeCreationHelpers::FindTargetClass(const FString& ClassName)
{
    if (ClassName.IsEmpty()) return nullptr;
    
    UClass* TargetClass = UClass::TryFindTypeSlow<UClass>(ClassName);
    if (TargetClass) return TargetClass;
    
    // Try with common prefixes
    FString TestClassName = ClassName;
    if (!TestClassName.StartsWith(TEXT("U")) && !TestClassName.StartsWith(TEXT("A")) && !TestClassName.StartsWith(TEXT("/Script/")))
    {
        TestClassName = TEXT("U") + ClassName;
        TargetClass = UClass::TryFindTypeSlow<UClass>(TestClassName);
        if (TargetClass) return TargetClass;
    }
    
    // Try with full path for common Unreal classes
    if (ClassName.Equals(TEXT("KismetMathLibrary"), ESearchCase::IgnoreCase))
    {
        return UKismetMathLibrary::StaticClass();
    }
    else if (ClassName.Equals(TEXT("KismetSystemLibrary"), ESearchCase::IgnoreCase))
    {
        return UKismetSystemLibrary::StaticClass();
    }
    else if (ClassName.Equals(TEXT("GameplayStatics"), ESearchCase::IgnoreCase))
    {
        return UGameplayStatics::StaticClass();
    }
    
    return nullptr;
}

FString UnrealMCPNodeCreationHelpers::BuildNodeResult(bool bSuccess, const FString& Message, const FString& BlueprintName, 
                                                      const FString& FunctionName, UEdGraphNode* NewNode, 
                                                      const FString& NodeTitle, const FString& NodeType, 
                                                      UClass* TargetClass, int32 PositionX, int32 PositionY)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), bSuccess);
    ResultObj->SetStringField(TEXT("message"), Message);
    
    if (bSuccess && NewNode)
    {
        ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
        ResultObj->SetStringField(TEXT("function_name"), FunctionName);
        ResultObj->SetStringField(TEXT("node_type"), NodeType);
        ResultObj->SetStringField(TEXT("class_name"), NodeType.Equals(TEXT("UK2Node_CallFunction")) ? (TargetClass ? TargetClass->GetName() : TEXT("")) : TEXT(""));
        ResultObj->SetStringField(TEXT("node_id"), NewNode->NodeGuid.ToString());
        ResultObj->SetStringField(TEXT("node_title"), NodeTitle);
        
        // Add position info
        TSharedPtr<FJsonObject> PositionObj = MakeShared<FJsonObject>();
        PositionObj->SetNumberField(TEXT("x"), PositionX);
        PositionObj->SetNumberField(TEXT("y"), PositionY);
        ResultObj->SetObjectField(TEXT("position"), PositionObj);
        
        // Add pin information
        TArray<TSharedPtr<FJsonValue>> PinsArray;
        for (UEdGraphPin* Pin : NewNode->Pins)
        {
            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
            PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
            PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
            PinObj->SetBoolField(TEXT("is_execution"), Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
            PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
        }
        ResultObj->SetArrayField(TEXT("pins"), PinsArray);
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
    return OutputString;
}

bool UnrealMCPNodeCreationHelpers::TryCreateNodeUsingBlueprintActionDatabase(const FString& FunctionName, UEdGraph* EventGraph, float PositionX, float PositionY, UEdGraphNode*& NewNode, FString& NodeTitle, FString& NodeType)
{
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Attempting dynamic creation for '%s'"), *FunctionName);
    
    // Use Blueprint Action Database to find the appropriate spawner
    FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
    FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();
    
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Found %d action categories"), ActionRegistry.Num());
    
    // Search through spawners directly (similar to main file approach)
    for (const auto& ActionPair : ActionRegistry)
    {
        for (const UBlueprintNodeSpawner* NodeSpawner : ActionPair.Value)
        {
            if (NodeSpawner && IsValid(NodeSpawner))
            {
                // Get template node to determine what type of node this is
                UEdGraphNode* TemplateNode = NodeSpawner->GetTemplateNode();
                if (!TemplateNode)
                {
                    continue;
                }
                
                // Try to match based on node type and function name
                FString NodeName = TEXT("");
                FString NodeClass = TemplateNode->GetClass()->GetName();
                
                // Get human-readable node name
                if (UK2Node* K2Node = Cast<UK2Node>(TemplateNode))
                {
                    NodeName = K2Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                    if (NodeName.IsEmpty())
                    {
                        NodeName = K2Node->GetClass()->GetName();
                    }
                    
                    // For function calls, get the function name
                    if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(K2Node))
                    {
                        if (UFunction* Function = FunctionNode->GetTargetFunction())
                        {
                            NodeName = Function->GetName();
                        }
                    }
                }
                else
                {
                    NodeName = TemplateNode->GetClass()->GetName();
                }
                
                // Multiple matching strategies
                bool bMatches = false;
                
                if (NodeName.Equals(FunctionName, ESearchCase::IgnoreCase) ||
                    NodeClass.Equals(FunctionName, ESearchCase::IgnoreCase) ||
                    NodeName.Contains(FunctionName, ESearchCase::IgnoreCase))
                {
                    bMatches = true;
                }
                
                // Special case: Handle common display name vs function name mismatches
                if (!bMatches && FunctionName.Equals(TEXT("Print String"), ESearchCase::IgnoreCase))
                {
                    if (NodeName.Equals(TEXT("PrintString"), ESearchCase::IgnoreCase))
                    {
                        bMatches = true;
                    }
                }
                
                if (bMatches)
                {
                    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Found match '%s' for '%s'"), *NodeName, *FunctionName);
                    
                                         // Try to spawn the node using the spawner
                     IBlueprintNodeBinder::FBindingSet Bindings;
                     UEdGraphNode* SpawnedNode = NodeSpawner->Invoke(EventGraph, Bindings, FVector2D(PositionX, PositionY));
                    
                    if (SpawnedNode)
                    {
                        NewNode = SpawnedNode;
                        NodeTitle = NodeName;
                        NodeType = SpawnedNode->GetClass()->GetName();
                        UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: SUCCESS - Created node '%s' of type '%s'"), *NodeTitle, *NodeType);
                        return true;
                    }
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: No matching action found for '%s'"), *FunctionName);
    return false;
} 