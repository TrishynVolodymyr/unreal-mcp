#include "Utils/UnrealMCPCommonUtils.h"
#include "GameFramework/Actor.h"
#include "Engine/Blueprint.h"
#include "WidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Variable.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Selection.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabase.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "JsonObjectConverter.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/StructOnScope.h"

// JSON Utilities
TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateErrorResponse(const FString& Message)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), false);
    ResponseObject->SetStringField(TEXT("error"), Message);
    return ResponseObject;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateSuccessResponse(const FString& Message /* = TEXT("") */)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), true);
    
    if (!Message.IsEmpty())
    {
        ResponseObject->SetStringField(TEXT("message"), Message);
    }
    
    return ResponseObject;
}

void FUnrealMCPCommonUtils::GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((int32)Value->AsNumber());
        }
    }
}

void FUnrealMCPCommonUtils::GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((float)Value->AsNumber());
        }
    }
}

FVector2D FUnrealMCPCommonUtils::GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector2D Result(0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 2)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
    }
    
    return Result;
}

FVector FUnrealMCPCommonUtils::GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
        Result.Z = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

FRotator FUnrealMCPCommonUtils::GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FRotator Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.Pitch = (float)(*JsonArray)[0]->AsNumber();
        Result.Yaw = (float)(*JsonArray)[1]->AsNumber();
        Result.Roll = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

// Blueprint Utilities
UBlueprint* FUnrealMCPCommonUtils::FindBlueprint(const FString& BlueprintName)
{
    return FindBlueprintByName(BlueprintName);
}

UBlueprint* FUnrealMCPCommonUtils::FindBlueprintByName(const FString& BlueprintName)
{
    // Early exit for empty names
    if (BlueprintName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Empty blueprint name provided"));
        return nullptr;
    }

    // Step 1: Normalize the path
    FString NormalizedName = BlueprintName;
    
    // Remove .uasset extension if present
    if (NormalizedName.EndsWith(TEXT(".uasset"), ESearchCase::IgnoreCase))
    {
        NormalizedName = NormalizedName.LeftChop(7);
    }
    
    // Handle absolute vs relative paths
    bool bIsAbsolutePath = NormalizedName.StartsWith(TEXT("/"));
    if (bIsAbsolutePath)
    {
        // If it's an absolute path starting with /Game/, use it directly
        if (NormalizedName.StartsWith(TEXT("/Game/")))
        {
            UE_LOG(LogTemp, Display, TEXT("Using absolute path: %s"), *NormalizedName);
            UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *NormalizedName);
            if (Blueprint)
            {
                return Blueprint;
            }
        }
        // If it starts with / but not /Game/, prepend /Game/
        else
        {
            NormalizedName = FString(TEXT("/Game")) + NormalizedName;
            UE_LOG(LogTemp, Display, TEXT("Converted to game path: %s"), *NormalizedName);
            UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *NormalizedName);
            if (Blueprint)
            {
                return Blueprint;
            }
        }
    }
    else
    {
        // For relative paths, extract any subdirectories
        FString SubPath;
        FString BaseName;
        if (NormalizedName.Contains(TEXT("/")))
        {
            NormalizedName.Split(TEXT("/"), &SubPath, &BaseName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
            
            // Fix: Check if SubPath already starts with "Game"
            if (SubPath.StartsWith(TEXT("Game"), ESearchCase::IgnoreCase))
            {
                // Already has Game prefix, just add leading slash
                NormalizedName = FString::Printf(TEXT("/%s/%s"), *SubPath, *BaseName);
            }
            else
            {
                // Reconstruct with /Game/ prefix
                NormalizedName = FString::Printf(TEXT("/Game/%s/%s"), *SubPath, *BaseName);
            }
            
            UE_LOG(LogTemp, Display, TEXT("Reconstructed path with subdirectory: %s"), *NormalizedName);
            UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *NormalizedName);
            if (Blueprint)
            {
                return Blueprint;
            }
        }
        else
        {
            BaseName = NormalizedName;
        }

        // Try standard locations for relative paths
        TArray<FString> DefaultPaths = {
            FString::Printf(TEXT("/Game/Blueprints/%s"), *BaseName),
            FString::Printf(TEXT("/Game/%s"), *BaseName)
        };

        // Try each default path
        for (const FString& Path : DefaultPaths)
        {
            UE_LOG(LogTemp, Display, TEXT("Trying blueprint at path: %s"), *Path);
            UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Path);
            if (Blueprint)
            {
                return Blueprint;
            }
        }
    }

    // If still not found, use asset registry for a thorough search
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AllBlueprintAssetData;
    
    // Create a filter for blueprints and widget blueprints
    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(TEXT("/Game"));
    Filter.bRecursivePaths = true;

    UE_LOG(LogTemp, Display, TEXT("Performing Asset Registry search for: %s"), *NormalizedName);
    AssetRegistryModule.Get().GetAssets(Filter, AllBlueprintAssetData);
    UE_LOG(LogTemp, Display, TEXT("Found %d total blueprint assets"), AllBlueprintAssetData.Num());

    // First try exact name match
    FString SearchName = FPaths::GetBaseFilename(NormalizedName);
    for (const FAssetData& Asset : AllBlueprintAssetData)
    {
        if (Asset.AssetName.ToString() == SearchName)
        {
            UE_LOG(LogTemp, Display, TEXT("Found exact match: %s"), *Asset.GetObjectPathString());
            return Cast<UBlueprint>(Asset.GetAsset());
        }
    }

    // If exact match fails, try case-insensitive match
    for (const FAssetData& Asset : AllBlueprintAssetData)
    {
        if (Asset.AssetName.ToString().Equals(SearchName, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogTemp, Warning, TEXT("Found case-insensitive match: %s"), *Asset.GetObjectPathString());
            return Cast<UBlueprint>(Asset.GetAsset());
        }
    }

    UE_LOG(LogTemp, Error, TEXT("Blueprint '%s' not found after exhaustive search"), *BlueprintName);
    return nullptr;
}

UEdGraph* FUnrealMCPCommonUtils::FindOrCreateEventGraph(UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Try to find the event graph
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph->GetName().Contains(TEXT("EventGraph")))
        {
            return Graph;
        }
    }
    
    // Create a new event graph if none exists
    UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FName(TEXT("EventGraph")), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(Blueprint, NewGraph);
    return NewGraph;
}

// Blueprint node utilities
UK2Node_Event* FUnrealMCPCommonUtils::CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Check for existing event node with this exact name
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Using existing event node with name %s (ID: %s)"), 
                *EventName, *EventNode->NodeGuid.ToString());
            return EventNode;
        }
    }

    // No existing node found, create a new one
    UK2Node_Event* EventNode = nullptr;
    
    // Find the function to create the event
    UClass* BlueprintClass = Blueprint->GeneratedClass;
    UFunction* EventFunction = BlueprintClass->FindFunctionByName(FName(*EventName));
    
    if (EventFunction)
    {
        EventNode = NewObject<UK2Node_Event>(Graph);
        EventNode->EventReference.SetExternalMember(FName(*EventName), BlueprintClass);
        EventNode->NodePosX = Position.X;
        EventNode->NodePosY = Position.Y;
        Graph->AddNode(EventNode, true);
        EventNode->PostPlacedNewNode();
        EventNode->AllocateDefaultPins();
        UE_LOG(LogTemp, Display, TEXT("Created new event node with name %s (ID: %s)"), 
            *EventName, *EventNode->NodeGuid.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find function for event name: %s"), *EventName);
    }
    
    return EventNode;
}

UK2Node_CallFunction* FUnrealMCPCommonUtils::CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& Position)
{
    if (!Graph || !Function)
    {
        return nullptr;
    }
    
    UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
    FunctionNode->SetFromFunction(Function);
    FunctionNode->NodePosX = Position.X;
    FunctionNode->NodePosY = Position.Y;
    Graph->AddNode(FunctionNode, true);
    FunctionNode->CreateNewGuid();
    FunctionNode->PostPlacedNewNode();
    FunctionNode->AllocateDefaultPins();
    
    return FunctionNode;
}

UK2Node_VariableGet* FUnrealMCPCommonUtils::CreateVariableGetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableGet* VariableGetNode = NewObject<UK2Node_VariableGet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        VariableGetNode->VariableReference.SetFromField<FProperty>(Property, false);
        VariableGetNode->NodePosX = Position.X;
        VariableGetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableGetNode, true);
        VariableGetNode->PostPlacedNewNode();
        VariableGetNode->AllocateDefaultPins();
        
        return VariableGetNode;
    }
    
    return nullptr;
}

UK2Node_VariableSet* FUnrealMCPCommonUtils::CreateVariableSetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableSet* VariableSetNode = NewObject<UK2Node_VariableSet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        VariableSetNode->VariableReference.SetFromField<FProperty>(Property, false);
        VariableSetNode->NodePosX = Position.X;
        VariableSetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableSetNode, true);
        VariableSetNode->PostPlacedNewNode();
        VariableSetNode->AllocateDefaultPins();
        
        return VariableSetNode;
    }
    
    return nullptr;
}



UK2Node_Self* FUnrealMCPCommonUtils::CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
    SelfNode->NodePosX = Position.X;
    SelfNode->NodePosY = Position.Y;
    Graph->AddNode(SelfNode, true);
    SelfNode->CreateNewGuid();
    SelfNode->PostPlacedNewNode();
    SelfNode->AllocateDefaultPins();
    
    return SelfNode;
}

bool FUnrealMCPCommonUtils::ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, 
                                           UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    if (!Graph || !SourceNode || !TargetNode)
    {
        return false;
    }
    
    UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName, EGPD_Input);
    
    if (SourcePin && TargetPin)
    {
        SourcePin->MakeLinkTo(TargetPin);
        return true;
    }
    
    return false;
}

UEdGraphPin* FUnrealMCPCommonUtils::FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
    if (!Node)
    {
        return nullptr;
    }
    
    // Log all pins for debugging
    UE_LOG(LogTemp, Display, TEXT("FindPin: Looking for pin '%s' (Direction: %d) in node '%s'"), 
           *PinName, (int32)Direction, *Node->GetName());
    
    for (UEdGraphPin* Pin : Node->Pins)
    {
        UE_LOG(LogTemp, Display, TEXT("  - Available pin: '%s', Direction: %d, Category: %s"), 
               *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
    }
    
    // First try exact match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString() == PinName && (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found exact matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }
    
    // If no exact match and we're looking for a component reference, try case-insensitive match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase) && 
            (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }
    
    // If we're looking for a component output and didn't find it by name, try to find the first data output pin
    if (Direction == EGPD_Output && Cast<UK2Node_VariableGet>(Node) != nullptr)
    {
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                UE_LOG(LogTemp, Display, TEXT("  - Found fallback data output pin: '%s'"), *Pin->PinName.ToString());
                return Pin;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("  - No matching pin found for '%s'"), *PinName);
    return nullptr;
}

// Actor utilities
TSharedPtr<FJsonValue> FUnrealMCPCommonUtils::ActorToJson(AActor* Actor)
{
    if (!Actor)
    {
        return MakeShared<FJsonValueNull>();
    }
    
    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);
    
    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
    
    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);
    
    return MakeShared<FJsonValueObject>(ActorObject);
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::ActorToJsonObject(AActor* Actor, bool bDetailed)
{
    if (!Actor)
    {
        return nullptr;
    }
    
    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);
    
    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
    
    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);
    
    return ActorObject;
}

UK2Node_Event* FUnrealMCPCommonUtils::FindExistingEventNode(UEdGraph* Graph, const FString& EventName)
{
    if (!Graph)
    {
        return nullptr;
    }

    // Look for existing event nodes
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Found existing event node with name: %s"), *EventName);
            return EventNode;
        }
    }

    return nullptr;
}

bool FUnrealMCPCommonUtils::SetObjectProperty(UObject* Object, const FString& PropertyName, 
                                     const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage)
{
    if (!Object)
    {
        OutErrorMessage = TEXT("Invalid object");
        return false;
    }

    FProperty* Property = Object->GetClass()->FindPropertyByName(*PropertyName);
    if (!Property)
    {
        OutErrorMessage = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
        return false;
    }

    void* PropertyAddr = Property->ContainerPtrToValuePtr<void>(Object);
    
    // Handle different property types
    if (Property->IsA<FBoolProperty>())
    {
        ((FBoolProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsBool());
        return true;
    }
    else if (Property->IsA<FIntProperty>())
    {
        int32 IntValue = static_cast<int32>(Value->AsNumber());
        FIntProperty* IntProperty = CastField<FIntProperty>(Property);
        if (IntProperty)
        {
            IntProperty->SetPropertyValue_InContainer(Object, IntValue);
            return true;
        }
    }
    else if (Property->IsA<FFloatProperty>())
    {
        ((FFloatProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsNumber());
        return true;
    }
    else if (Property->IsA<FStrProperty>())
    {
        ((FStrProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsString());
        return true;
    }
    else if (Property->IsA<FByteProperty>())
    {
        FByteProperty* ByteProp = CastField<FByteProperty>(Property);
        UEnum* EnumDef = ByteProp ? ByteProp->GetIntPropertyEnum() : nullptr;
        
        // If this is a TEnumAsByte property (has associated enum)
        if (EnumDef)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
                ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
                
                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %d"), 
                      *PropertyName, ByteValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();
                
                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    uint8 ByteValue = FCString::Atoi(*EnumValueName);
                    ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %d"), 
                          *PropertyName, *EnumValueName, ByteValue);
                    return true;
                }
                
                // Handle qualified enum names (e.g., "Player0" or "EAutoReceiveInput::Player0")
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }
                
                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }
                
                if (EnumValue != INDEX_NONE)
                {
                    ByteProp->SetPropertyValue(PropertyAddr, static_cast<uint8>(EnumValue));
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"), 
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }
                    
                    OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
        else
        {
            // Regular byte property
            uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
            ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
            return true;
        }
    }
    else if (Property->IsA<FEnumProperty>())
    {
        FEnumProperty* EnumProp = CastField<FEnumProperty>(Property);
        UEnum* EnumDef = EnumProp ? EnumProp->GetEnum() : nullptr;
        FNumericProperty* UnderlyingNumericProp = EnumProp ? EnumProp->GetUnderlyingProperty() : nullptr;
        
        if (EnumDef && UnderlyingNumericProp)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                int64 EnumValue = static_cast<int64>(Value->AsNumber());
                UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                
                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %lld"), 
                      *PropertyName, EnumValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();
                
                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    int64 EnumValue = FCString::Atoi64(*EnumValueName);
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                
                // Handle qualified enum names
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }
                
                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }
                
                if (EnumValue != INDEX_NONE)
                {
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"), 
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }
                    
                    OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
    }
    else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        if (Value->Type == EJson::Array)
        {
            const TArray<TSharedPtr<FJsonValue>>& Arr = Value->AsArray();
            bool bStructHandled = false;

            // Handle FVector2D
            if (StructProp->Struct == TBaseStructure<FVector2D>::Get())
            {
                if (Arr.Num() == 2)
                {
                    FVector2D Vec2D(Arr[0]->AsNumber(), Arr[1]->AsNumber());
                    StructProp->CopySingleValue(PropertyAddr, &Vec2D);
                    UE_LOG(LogTemp, Display, TEXT("Setting FVector2D property %s to (%f, %f)"), 
                          *PropertyName, Vec2D.X, Vec2D.Y);
                    bStructHandled = true;
                }
                else
                {
                    OutErrorMessage = FString::Printf(TEXT("FVector2D property requires 2 values, got %d"), Arr.Num());
                }
            }
            // Handle FLinearColor
            else if (StructProp->Struct == TBaseStructure<FLinearColor>::Get())
            {
                if (Arr.Num() == 4) // RGBA
                {
                    FLinearColor Color(
                        Arr[0]->AsNumber(),
                        Arr[1]->AsNumber(),
                        Arr[2]->AsNumber(),
                        Arr[3]->AsNumber()
                    );
                    StructProp->CopySingleValue(PropertyAddr, &Color);
                    UE_LOG(LogTemp, Display, TEXT("Setting FLinearColor property %s to (R=%f, G=%f, B=%f, A=%f)"), 
                          *PropertyName, Color.R, Color.G, Color.B, Color.A);
                    bStructHandled = true;
                }
                 else if (Arr.Num() == 3) // RGB, assume A=1
                {
                    FLinearColor Color(
                        Arr[0]->AsNumber(),
                        Arr[1]->AsNumber(),
                        Arr[2]->AsNumber(),
                        1.0f // Default Alpha to 1
                    );
                    StructProp->CopySingleValue(PropertyAddr, &Color);
                    UE_LOG(LogTemp, Display, TEXT("Setting FLinearColor property %s to (R=%f, G=%f, B=%f, A=1.0)"), 
                          *PropertyName, Color.R, Color.G, Color.B);
                    bStructHandled = true;
                }
                else
                {
                    OutErrorMessage = FString::Printf(TEXT("FLinearColor property requires 3 (RGB) or 4 (RGBA) values, got %d"), Arr.Num());
                }
            }
            // Handle FRotator
            else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
            {
                if (Arr.Num() == 3) // Pitch, Yaw, Roll
                {
                    FRotator Rotator(
                        Arr[0]->AsNumber(), // Pitch
                        Arr[1]->AsNumber(), // Yaw
                        Arr[2]->AsNumber()  // Roll
                    );
                    StructProp->CopySingleValue(PropertyAddr, &Rotator);
                    UE_LOG(LogTemp, Display, TEXT("Setting FRotator property %s to (P=%f, Y=%f, R=%f)"), 
                          *PropertyName, Rotator.Pitch, Rotator.Yaw, Rotator.Roll);
                    bStructHandled = true;
                }
                else
                {
                    OutErrorMessage = FString::Printf(TEXT("FRotator property requires 3 values (Pitch, Yaw, Roll), got %d"), Arr.Num());
                }
            }
            // NOTE: FVector is handled specifically in HandleSetComponentProperty currently, 
            // but could be moved here for consistency if desired.
            
            if (bStructHandled)
            {
                return true; // Successfully handled the struct
            }
        }
        else
        {
             OutErrorMessage = FString::Printf(TEXT("Struct property %s requires a JSON array value"), *PropertyName);
        }
        // If we reach here, the struct type wasn't handled or input was wrong
        if (OutErrorMessage.IsEmpty())
        {
            OutErrorMessage = FString::Printf(TEXT("Unsupported struct type '%s' for property %s"), 
                StructProp->Struct ? *StructProp->Struct->GetName() : TEXT("Unknown"), *PropertyName);
        }
        return false;
    }
    else if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
    {
        // Handle object references (like DataTable, Blueprint classes, etc.)
        if (Value->Type == EJson::String)
        {
            FString AssetPath = Value->AsString();
            
            // Remove _C suffix if present (for blueprint classes)
            AssetPath.RemoveFromEnd(TEXT("_C"));
            
            UObject* LoadedObject = nullptr;
            
            // Use UEditorAssetLibrary to search and load the asset
            if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
            {
                LoadedObject = UEditorAssetLibrary::LoadAsset(AssetPath);
            }
            else
            {
                // If direct path doesn't exist, search for it
                TArray<FString> FoundAssets = UEditorAssetLibrary::ListAssets(TEXT("/Game"), true, false);
                
                for (const FString& AssetPathIter : FoundAssets)
                {
                    FString AssetName = FPaths::GetBaseFilename(AssetPathIter);
                    if (AssetName.Equals(AssetPath, ESearchCase::IgnoreCase))
                    {
                        LoadedObject = UEditorAssetLibrary::LoadAsset(AssetPathIter);
                        if (LoadedObject)
                        {
                            UE_LOG(LogTemp, Display, TEXT("Found asset by name search: %s at %s"), 
                                  *AssetPath, *AssetPathIter);
                            break;
                        }
                    }
                }
            }
            
            if (LoadedObject)
            {
                ObjectProp->SetObjectPropertyValue(PropertyAddr, LoadedObject);
                UE_LOG(LogTemp, Display, TEXT("Set object property %s to %s"), 
                      *PropertyName, *LoadedObject->GetPathName());
                return true;
            }
            else
            {
                OutErrorMessage = FString::Printf(TEXT("Failed to load object from path: %s"), *AssetPath);
                return false;
            }
        }
        else
        {
            OutErrorMessage = FString::Printf(TEXT("Object property %s requires a string path value"), *PropertyName);
            return false;
        }
    }
    
    OutErrorMessage = FString::Printf(TEXT("Unsupported property type: %s for property %s"), 
                                    *Property->GetClass()->GetName(), *PropertyName);
    return false;
}

// Implementation for the new helper function
bool FUnrealMCPCommonUtils::SetPropertyFromJson(FProperty* Property, void* ContainerPtr, const TSharedPtr<FJsonValue>& JsonValue)
{
    if (!Property || !ContainerPtr || !JsonValue.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson: Invalid input parameter(s)."));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Property Name: %s, Type: %s"), *Property->GetName(), *Property->GetCPPType());

    // Handle different property types
    if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
    {
        bool Value;
        if (JsonValue->TryGetBool(Value))
        {
            UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Setting Bool property to: %s"), Value ? TEXT("true") : TEXT("false"));
            BoolProperty->SetPropertyValue(ContainerPtr, Value);
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to set Bool property, incompatible value type"));
        }
    }
    else if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
    {
        int32 Value;
        if (JsonValue->TryGetNumber(Value))
        {
            UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Setting Int property to: %d"), Value);
            IntProperty->SetPropertyValue(ContainerPtr, Value);
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to set Int property, incompatible value type"));
        }
    }
    else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        double Value; // JSON numbers are doubles
        if (JsonValue->TryGetNumber(Value))
        {
            UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Setting Float property to: %f"), static_cast<float>(Value));
            FloatProperty->SetPropertyValue(ContainerPtr, static_cast<float>(Value));
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to set Float property, incompatible value type"));
        }
    }
    else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
    {
        double Value;
        if (JsonValue->TryGetNumber(Value))
        {
            DoubleProperty->SetPropertyValue(ContainerPtr, Value);
            return true;
        }
    }
    else if (FStrProperty* StrProperty = CastField<FStrProperty>(Property))
    {
        FString Value;
        if (JsonValue->TryGetString(Value))
        {
            StrProperty->SetPropertyValue(ContainerPtr, Value);
            return true;
        }
    }
    else if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
    {
        FString Value;
        if (JsonValue->TryGetString(Value))
        {
            NameProperty->SetPropertyValue(ContainerPtr, FName(*Value));
            return true;
        }
    }
    else if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
    {
        FString Value;
        if (JsonValue->TryGetString(Value))
        {
            TextProperty->SetPropertyValue(ContainerPtr, FText::FromString(Value));
            return true;
        }
    }
    else if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
    {
        UEnum* Enum = EnumProperty->GetEnum();
        if (!Enum) return false;

        FString StringValue;
        int64 IntValue;

        if (JsonValue->TryGetString(StringValue)) // Try setting by string name first
        {
            IntValue = Enum->GetValueByNameString(StringValue);
            if (IntValue != INDEX_NONE)
            {
                EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ContainerPtr, IntValue);
                return true;
            }
        }
        else if (JsonValue->TryGetNumber(IntValue)) // Try setting by integer index
        {
             if (Enum->IsValidEnumValue(IntValue))
             {
                 EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ContainerPtr, IntValue);
                 return true;
             }
        }
    }
    else if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
    {
        UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Found Struct property: %s"), 
            StructProperty->Struct ? *StructProperty->Struct->GetName() : TEXT("NULL"));
            
        const TSharedPtr<FJsonObject>* JsonObject;
        if (JsonValue->TryGetObject(JsonObject))
        {
            UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Processing JsonObject for struct"));
            // Use JsonObjectConverter to convert the JSON object to the struct
            if (FJsonObjectConverter::JsonObjectToUStruct(JsonObject->ToSharedRef(), StructProperty->Struct, ContainerPtr, 0, 0))
            {
                UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Successfully converted JsonObject to struct"));
                return true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to convert JsonObject to struct"));
            }
        }
        // Handle common structs specifically if needed (e.g., FVector, FLinearColor from array)
        else if (StructProperty->Struct == TBaseStructure<FVector>::Get()) 
        {
            UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Handling FVector struct"));
            const TArray<TSharedPtr<FJsonValue>>* JsonArray;
            if (JsonValue->TryGetArray(JsonArray)) 
            {
                UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Got array for FVector with %d elements"), JsonArray->Num());
                FVector VecValue;
                if (ParseVector(*JsonArray, VecValue))
                {
                    UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Setting FVector to (%f, %f, %f)"), 
                        VecValue.X, VecValue.Y, VecValue.Z);
                    *static_cast<FVector*>(ContainerPtr) = VecValue;
                    return true;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to parse Vector from array"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Expected array for Vector but got different type"));
            }
        }
        else if (StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
        {
            UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Handling FLinearColor struct"));
            const TArray<TSharedPtr<FJsonValue>>* JsonArray;
            if (JsonValue->TryGetArray(JsonArray))
            {
                UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Got array for FLinearColor with %d elements"), JsonArray->Num());
                FLinearColor ColorValue;
                if (ParseLinearColor(*JsonArray, ColorValue))
                {
                    UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Setting FLinearColor to (%f, %f, %f, %f)"), 
                        ColorValue.R, ColorValue.G, ColorValue.B, ColorValue.A);
                    *static_cast<FLinearColor*>(ContainerPtr) = ColorValue;
                    return true;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to parse LinearColor from array"));
                }
            }
            // Additional check for string format, like when a string representation of an array is passed
            else if (JsonValue->Type == EJson::String)
            {
                UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Got string for FLinearColor: %s"), *JsonValue->AsString());
                FString ColorString = JsonValue->AsString();
                
                // Check if the string looks like an array: "[r, g, b, a]"
                if (ColorString.StartsWith(TEXT("[")) && ColorString.EndsWith(TEXT("]")))
                {
                    // Remove brackets
                    ColorString = ColorString.Mid(1, ColorString.Len() - 2);
                    
                    // Split by commas
                    TArray<FString> ColorComponents;
                    ColorString.ParseIntoArray(ColorComponents, TEXT(","), true);
                    
                    UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Parsed %d color components from string"), ColorComponents.Num());
                    
                    if (ColorComponents.Num() >= 3)
                    {
                        float R = FCString::Atof(*ColorComponents[0].TrimStart());
                        float G = FCString::Atof(*ColorComponents[1].TrimStart());
                        float B = FCString::Atof(*ColorComponents[2].TrimStart());
                        float A = ColorComponents.Num() >= 4 ? FCString::Atof(*ColorComponents[3].TrimStart()) : 1.0f;
                        
                        FLinearColor ColorValue(R, G, B, A);
                        UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Setting FLinearColor from string to (%f, %f, %f, %f)"), 
                            ColorValue.R, ColorValue.G, ColorValue.B, ColorValue.A);
                        *static_cast<FLinearColor*>(ContainerPtr) = ColorValue;
                        return true;
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Not enough color components in string: %s"), *ColorString);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Color string is not in expected format: %s"), *ColorString);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Expected array or string for LinearColor but got different type"));
            }
        }
        // ... existing code ...
    }
    else if (FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
    {
        FString Value;
        if (JsonValue->TryGetString(Value))
        {
            UClass* LoadedClass = LoadObject<UClass>(nullptr, *Value);
            if (LoadedClass && LoadedClass->IsChildOf(ClassProperty->MetaClass))
            {
                ClassProperty->SetPropertyValue(ContainerPtr, LoadedClass);
                UE_LOG(LogTemp, Log, TEXT("SetPropertyFromJson - Set UClass* property '%s' to '%s'"), *Property->GetName(), *Value);
                return true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson - Failed to load or type mismatch for UClass* property '%s' with value '%s'"), *Property->GetName(), *Value);
            }
        }
    }
    // ... existing code ...

    // Log failure if no suitable type handler was found
    UE_LOG(LogTemp, Warning, TEXT("SetPropertyFromJson: Unsupported property type '%s' or invalid JSON value type."), *Property->GetClass()->GetName());
    return false;
}

// Example implementation for ParseVector (adjust as needed)
bool FUnrealMCPCommonUtils::ParseVector(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FVector& OutVector)
{
    if (JsonArray.Num() == 3) 
    {
        double X, Y, Z;
        if (JsonArray[0]->TryGetNumber(X) && JsonArray[1]->TryGetNumber(Y) && JsonArray[2]->TryGetNumber(Z))
        {
            OutVector.X = X;
            OutVector.Y = Y;
            OutVector.Z = Z;
            return true;
        }
    }
    return false;
}

// Example implementation for ParseLinearColor (adjust as needed)
bool FUnrealMCPCommonUtils::ParseLinearColor(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FLinearColor& OutColor)
{
    UE_LOG(LogTemp, Log, TEXT("ParseLinearColor - Array has %d elements"), JsonArray.Num());
    
    if (JsonArray.Num() < 3)
    {
        UE_LOG(LogTemp, Warning, TEXT("ParseLinearColor - Array has insufficient elements: %d (need at least 3)"), JsonArray.Num());
        return false;
    }

    // Check each element to ensure they are numbers
    for (int32 i = 0; i < JsonArray.Num() && i < 4; ++i)
    {
        if (JsonArray[i]->Type != EJson::Number)
        {
            UE_LOG(LogTemp, Warning, TEXT("ParseLinearColor - Element %d is not a number (type: %d)"), i, (int)JsonArray[i]->Type);
            return false;
        }
    }

    // Extract RGB values
    float R = JsonArray[0]->AsNumber();
    float G = JsonArray[1]->AsNumber();
    float B = JsonArray[2]->AsNumber();
    
    // Extract Alpha if available, otherwise default to 1.0
    float A = 1.0f;
    if (JsonArray.Num() >= 4)
    {
        A = JsonArray[3]->AsNumber();
    }
    
    UE_LOG(LogTemp, Log, TEXT("ParseLinearColor - Parsed color: R=%f, G=%f, B=%f, A=%f"), R, G, B, A);
    
    // Set the output color
    OutColor = FLinearColor(R, G, B, A);
    return true;
}

// Placeholder for ParseRotator if needed
bool FUnrealMCPCommonUtils::ParseRotator(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FRotator& OutRotator)
{
    if (JsonArray.Num() == 3) 
    {
        double P, Y, R;
        if (JsonArray[0]->TryGetNumber(P) && JsonArray[1]->TryGetNumber(Y) && JsonArray[2]->TryGetNumber(R))
        {
            OutRotator.Pitch = P;
            OutRotator.Yaw = Y;
            OutRotator.Roll = R;
            return true;
        }
    }
    return false;
}

// FindActorByName implementation might already exist, ensure it's suitable or adapt
AActor* FUnrealMCPCommonUtils::FindActorByName(const FString& ActorName)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return nullptr;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->GetName() == ActorName)
        {
            return Actor;
        }
    }
    return nullptr;
}

bool FUnrealMCPCommonUtils::CallFunctionByName(UObject* Target, const FString& FunctionName, const TArray<FString>& StringParams, FString& OutError)
{
    if (!Target) {
        OutError = TEXT("Target is null");
        return false;
    }
    UFunction* Function = Target->FindFunction(FName(*FunctionName));
    if (!Function) {
        OutError = FString::Printf(TEXT("Function not found: %s"), *FunctionName);
        return false;
    }
    uint8* Params = (uint8*)FMemory_Alloca(Function->ParmsSize);
    FMemory::Memzero(Params, Function->ParmsSize);

    int32 ParamIndex = 0;
    for (TFieldIterator<FProperty> It(Function); It && (It->PropertyFlags & CPF_Parm); ++It)
    {
        if (It->IsA<FStrProperty>() && ParamIndex < StringParams.Num())
        {
            FStrProperty* StrProp = CastField<FStrProperty>(*It);
            void* ValuePtr = It->ContainerPtrToValuePtr<void>(Params);
            StrProp->SetPropertyValue(ValuePtr, StringParams[ParamIndex]);
            ParamIndex++;
        }
        // Extend for other types as needed
    }
    Target->ProcessEvent(Function, Params);
    return true;
}

// ==================== Asset Discovery Implementations ====================

TArray<FString> FUnrealMCPCommonUtils::FindAssetsByType(const FString& AssetType, const FString& SearchPath)
{
    TArray<FString> FoundAssets;
    
    // Get the Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Create filter for asset type
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(FTopLevelAssetPath(*AssetType));
    
    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        FoundAssets.Add(AssetData.GetSoftObjectPath().ToString());
    }
    
    UE_LOG(LogTemp, Display, TEXT("Found %d assets of type '%s' in path '%s'"), FoundAssets.Num(), *AssetType, *SearchPath);
    return FoundAssets;
}

TArray<FString> FUnrealMCPCommonUtils::FindAssetsByName(const FString& AssetName, const FString& SearchPath)
{
    TArray<FString> FoundAssets;
    
    // Get the Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Create filter for search path
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    
    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        FString AssetBaseName = FPaths::GetBaseFilename(AssetData.AssetName.ToString());
        if (AssetBaseName.Contains(AssetName, ESearchCase::IgnoreCase) || 
            AssetData.AssetName.ToString().Contains(AssetName, ESearchCase::IgnoreCase))
        {
            FoundAssets.Add(AssetData.GetSoftObjectPath().ToString());
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("Found %d assets matching name '%s' in path '%s'"), FoundAssets.Num(), *AssetName, *SearchPath);
    return FoundAssets;
}

TArray<FString> FUnrealMCPCommonUtils::FindWidgetBlueprints(const FString& WidgetName, const FString& SearchPath)
{
    TArray<FString> FoundWidgets;
    
    // Get the Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Create filter for Widget Blueprints
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/UMGEditor"), TEXT("WidgetBlueprint")));
    
    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (WidgetName.IsEmpty() || 
            AssetData.AssetName.ToString().Contains(WidgetName, ESearchCase::IgnoreCase))
        {
            FoundWidgets.Add(AssetData.GetSoftObjectPath().ToString());
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("Found %d widget blueprints matching '%s' in path '%s'"), FoundWidgets.Num(), *WidgetName, *SearchPath);
    return FoundWidgets;
}

TArray<FString> FUnrealMCPCommonUtils::FindBlueprints(const FString& BlueprintName, const FString& SearchPath)
{
    TArray<FString> FoundBlueprints;
    
    // Get the Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Create filter for Blueprints
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("Blueprint")));
    
    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (BlueprintName.IsEmpty() || 
            AssetData.AssetName.ToString().Contains(BlueprintName, ESearchCase::IgnoreCase))
        {
            FoundBlueprints.Add(AssetData.GetSoftObjectPath().ToString());
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("Found %d blueprints matching '%s' in path '%s'"), FoundBlueprints.Num(), *BlueprintName, *SearchPath);
    return FoundBlueprints;
}

TArray<FString> FUnrealMCPCommonUtils::FindDataTables(const FString& TableName, const FString& SearchPath)
{
    TArray<FString> FoundTables;
    
    // Get the Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Create filter for Data Tables
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("DataTable")));
    
    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (TableName.IsEmpty() || 
            AssetData.AssetName.ToString().Contains(TableName, ESearchCase::IgnoreCase))
        {
            FoundTables.Add(AssetData.GetSoftObjectPath().ToString());
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("Found %d data tables matching '%s' in path '%s'"), FoundTables.Num(), *TableName, *SearchPath);
    return FoundTables;
}

UClass* FUnrealMCPCommonUtils::FindWidgetClass(const FString& WidgetPath)
{
    UE_LOG(LogTemp, Display, TEXT("FindWidgetClass: Searching for widget class: %s"), *WidgetPath);
    
    // Strategy 1: Direct class loading if path looks like a class path
    if (WidgetPath.Contains(TEXT("_C")) || WidgetPath.StartsWith(TEXT("/Script/")))
    {
        UClass* DirectClass = LoadObject<UClass>(nullptr, *WidgetPath);
        if (DirectClass && DirectClass->IsChildOf(UUserWidget::StaticClass()))
        {
            UE_LOG(LogTemp, Display, TEXT("FindWidgetClass: Found class via direct loading: %s"), *DirectClass->GetName());
            return DirectClass;
        }
    }
    
    // Strategy 2: Asset-based loading
    UBlueprint* WidgetBlueprint = FindWidgetBlueprint(WidgetPath);
    if (WidgetBlueprint && WidgetBlueprint->GeneratedClass)
    {
        UClass* GeneratedClass = WidgetBlueprint->GeneratedClass;
        if (GeneratedClass->IsChildOf(UUserWidget::StaticClass()))
        {
            UE_LOG(LogTemp, Display, TEXT("FindWidgetClass: Found class via blueprint: %s"), *GeneratedClass->GetName());
            return GeneratedClass;
        }
    }
    
    // Strategy 3: Search using asset discovery
    TArray<FString> CommonPaths = GetCommonAssetSearchPaths(WidgetPath);
    for (const FString& SearchPath : CommonPaths)
    {
        UE_LOG(LogTemp, Display, TEXT("FindWidgetClass: Trying search path: %s"), *SearchPath);
        
        // Try loading as blueprint asset first
        if (UEditorAssetLibrary::DoesAssetExist(SearchPath))
        {
            UObject* Asset = UEditorAssetLibrary::LoadAsset(SearchPath);
            if (UBlueprint* BP = Cast<UBlueprint>(Asset))
            {
                if (BP->GeneratedClass && BP->GeneratedClass->IsChildOf(UUserWidget::StaticClass()))
                {
                    UE_LOG(LogTemp, Display, TEXT("FindWidgetClass: Found widget class via asset search: %s"), *BP->GeneratedClass->GetName());
                    return BP->GeneratedClass;
                }
            }
        }
        
        // Try loading as class with _C suffix
        FString ClassPath = FString::Printf(TEXT("%s.%s_C"), *SearchPath, *FPaths::GetBaseFilename(SearchPath));
        UClass* Class = LoadObject<UClass>(nullptr, *ClassPath);
        if (Class && Class->IsChildOf(UUserWidget::StaticClass()))
        {
            UE_LOG(LogTemp, Display, TEXT("FindWidgetClass: Found widget class via class path: %s"), *Class->GetName());
            return Class;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("FindWidgetClass: Could not find widget class for: %s"), *WidgetPath);
    return nullptr;
}

UBlueprint* FUnrealMCPCommonUtils::FindWidgetBlueprint(const FString& WidgetPath)
{
    UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Searching for widget blueprint: %s"), *WidgetPath);
    
    // Strategy 1: Direct asset loading
    if (UEditorAssetLibrary::DoesAssetExist(WidgetPath))
    {
        UObject* Asset = UEditorAssetLibrary::LoadAsset(WidgetPath);
        if (UBlueprint* BP = Cast<UBlueprint>(Asset))
        {
            UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Found blueprint via direct loading: %s"), *BP->GetName());
            return BP;
        }
    }
    
    // Strategy 2: Search using common paths
    TArray<FString> CommonPaths = GetCommonAssetSearchPaths(WidgetPath);
    for (const FString& SearchPath : CommonPaths)
    {
        UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Trying search path: %s"), *SearchPath);
        
        if (UEditorAssetLibrary::DoesAssetExist(SearchPath))
        {
            UObject* Asset = UEditorAssetLibrary::LoadAsset(SearchPath);
            if (UBlueprint* BP = Cast<UBlueprint>(Asset))
            {
                UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Found blueprint via asset search: %s"), *BP->GetName());
                return BP;
            }
        }
    }
    
    // Strategy 3: Use asset registry search
    TArray<FString> FoundWidgets = FindWidgetBlueprints(FPaths::GetBaseFilename(WidgetPath));
    for (const FString& FoundPath : FoundWidgets)
    {
        UObject* Asset = UEditorAssetLibrary::LoadAsset(FoundPath);
        if (UBlueprint* BP = Cast<UBlueprint>(Asset))
        {
            UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Found blueprint via registry search: %s"), *BP->GetName());
            return BP;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("FindWidgetBlueprint: Could not find widget blueprint for: %s"), *WidgetPath);
    return nullptr;
}

UObject* FUnrealMCPCommonUtils::FindAssetByPath(const FString& AssetPath)
{
    UE_LOG(LogTemp, Display, TEXT("FindAssetByPath: Searching for asset: %s"), *AssetPath);
    
    if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
        if (Asset)
        {
            UE_LOG(LogTemp, Display, TEXT("FindAssetByPath: Found asset: %s"), *Asset->GetName());
            return Asset;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("FindAssetByPath: Could not find asset: %s"), *AssetPath);
    return nullptr;
}

UObject* FUnrealMCPCommonUtils::FindAssetByName(const FString& AssetName, const FString& AssetType)
{
    UE_LOG(LogTemp, Display, TEXT("FindAssetByName: Searching for asset '%s' of type '%s'"), *AssetName, *AssetType);
    
    TArray<FString> FoundAssets;
    if (!AssetType.IsEmpty())
    {
        FoundAssets = FindAssetsByType(AssetType);
    }
    else
    {
        FoundAssets = FindAssetsByName(AssetName);
    }
    
    for (const FString& AssetPath : FoundAssets)
    {
        if (FPaths::GetBaseFilename(AssetPath).Contains(AssetName, ESearchCase::IgnoreCase))
        {
            UObject* Asset = FindAssetByPath(AssetPath);
            if (Asset)
            {
                UE_LOG(LogTemp, Display, TEXT("FindAssetByName: Found matching asset: %s"), *Asset->GetName());
                return Asset;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("FindAssetByName: Could not find asset '%s'"), *AssetName);
    return nullptr;
}

UScriptStruct* FUnrealMCPCommonUtils::FindStructType(const FString& StructPath)
{
    UE_LOG(LogTemp, Display, TEXT("FindStructType: Searching for struct: %s"), *StructPath);
    
    // Strategy 1: Check built-in struct types first
    static const TMap<FString, UScriptStruct*> BuiltInStructs = {
        {TEXT("Vector"), TBaseStructure<FVector>::Get()},
        {TEXT("Rotator"), TBaseStructure<FRotator>::Get()},
        {TEXT("Transform"), TBaseStructure<FTransform>::Get()},
        {TEXT("Color"), TBaseStructure<FLinearColor>::Get()},
        {TEXT("LinearColor"), TBaseStructure<FLinearColor>::Get()},
        {TEXT("Vector2D"), TBaseStructure<FVector2D>::Get()},
        {TEXT("IntPoint"), TBaseStructure<FIntPoint>::Get()},
        {TEXT("IntVector"), TBaseStructure<FIntVector>::Get()},
        {TEXT("Guid"), TBaseStructure<FGuid>::Get()},
        {TEXT("DateTime"), TBaseStructure<FDateTime>::Get()}
    };
    
    // Check for exact built-in match
    if (const UScriptStruct* const* FoundBuiltIn = BuiltInStructs.Find(StructPath))
    {
        UE_LOG(LogTemp, Display, TEXT("FindStructType: Found built-in struct: %s"), *(*FoundBuiltIn)->GetName());
        return const_cast<UScriptStruct*>(*FoundBuiltIn);
    }
    
    // Strategy 2: Try direct struct loading with various naming conventions
    TArray<FString> StructNameVariations;
    StructNameVariations.Add(StructPath);
    StructNameVariations.Add(FString::Printf(TEXT("F%s"), *StructPath));
    
    // If it's already a path, try loading directly
    if (StructPath.StartsWith(TEXT("/")) || StructPath.Contains(TEXT(".")))
    {
        UScriptStruct* DirectStruct = LoadObject<UScriptStruct>(nullptr, *StructPath);
        if (DirectStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("FindStructType: Found struct via direct path: %s"), *DirectStruct->GetName());
            return DirectStruct;
        }
    }
    
    // Strategy 3: Search in common struct directories
    TArray<FString> StructDirectories = {
        TEXT("/Game/DataStructures/"),
        TEXT("/Game/Data/"),
        TEXT("/Game/Blueprints/DataStructures/"),
        TEXT("/Game/Blueprints/Structs/"),
        TEXT("/Game/Blueprints/"),
        TEXT("/Game/Structs/"),
        TEXT("/Game/")
    };
    
    for (const FString& StructDir : StructDirectories)
    {
        for (const FString& StructVariation : StructNameVariations)
        {
            // Try as asset path (e.g., /Game/DataStructures/MyStruct.MyStruct)
            FString AssetPath = FString::Printf(TEXT("%s%s.%s"), *StructDir, *StructVariation, *StructVariation);
            UScriptStruct* FoundStruct = LoadObject<UScriptStruct>(nullptr, *AssetPath);
            if (FoundStruct)
            {
                UE_LOG(LogTemp, Display, TEXT("FindStructType: Found struct via asset search: %s"), *FoundStruct->GetName());
                return FoundStruct;
            }
            
            // Try with base filename only
            FString BaseFilename = FPaths::GetBaseFilename(StructPath);
            if (BaseFilename != StructPath)
            {
                AssetPath = FString::Printf(TEXT("%s%s.%s"), *StructDir, *BaseFilename, *BaseFilename);
                FoundStruct = LoadObject<UScriptStruct>(nullptr, *AssetPath);
                if (FoundStruct)
                {
                    UE_LOG(LogTemp, Display, TEXT("FindStructType: Found struct via base filename search: %s"), *FoundStruct->GetName());
                    return FoundStruct;
                }
            }
        }
    }
    
    // Strategy 4: Try engine paths for built-in structs
    for (const FString& StructVariation : StructNameVariations)
    {
        FString EnginePath = BuildEnginePath(StructVariation);
        UScriptStruct* EngineStruct = LoadObject<UScriptStruct>(nullptr, *EnginePath);
        if (EngineStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("FindStructType: Found struct via engine path: %s"), *EngineStruct->GetName());
            return EngineStruct;
        }
        
        FString CorePath = BuildCorePath(StructVariation);
        UScriptStruct* CoreStruct = LoadObject<UScriptStruct>(nullptr, *CorePath);
        if (CoreStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("FindStructType: Found struct via core path: %s"), *CoreStruct->GetName());
            return CoreStruct;
        }
    }
    
    // Strategy 5: Use asset registry to find user-defined structs
    TArray<FString> FoundStructs = FindAssetsByType(TEXT("UserDefinedStruct"));
    for (const FString& FoundPath : FoundStructs)
    {
        if (FPaths::GetBaseFilename(FoundPath).Contains(StructPath, ESearchCase::IgnoreCase) ||
            FPaths::GetBaseFilename(FoundPath).Contains(FString::Printf(TEXT("F%s"), *StructPath), ESearchCase::IgnoreCase))
        {
            UScriptStruct* Struct = Cast<UScriptStruct>(FindAssetByPath(FoundPath));
            if (Struct)
            {
                UE_LOG(LogTemp, Display, TEXT("FindStructType: Found struct via registry search: %s"), *Struct->GetName());
                return Struct;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("FindStructType: Could not find struct for: %s"), *StructPath);
    return nullptr;
}

TArray<FString> FUnrealMCPCommonUtils::GetCommonAssetSearchPaths(const FString& AssetName)
{
    TArray<FString> SearchPaths;
    
    // If AssetName is already a full path (starts with /Game/ or /Script/), just return it
    if (AssetName.StartsWith(TEXT("/Game/")) || AssetName.StartsWith(TEXT("/Script/")))
    {
        SearchPaths.Add(AssetName);
        return SearchPaths;
    }
    
    // Clean the asset name
    FString CleanName = NormalizeAssetPath(AssetName);
    
    // Remove common prefixes and suffixes for search
    if (CleanName.StartsWith(TEXT("WBP_")))
    {
        CleanName = CleanName.RightChop(4);
    }
    if (CleanName.StartsWith(TEXT("BP_")))
    {
        CleanName = CleanName.RightChop(3);
    }
    
    // Common widget directories
    TArray<FString> CommonDirs = {
        TEXT("/Game/Widgets/"),
        TEXT("/Game/UI/"),
        TEXT("/Game/UMG/"),
        TEXT("/Game/Blueprints/Widgets/"),
        TEXT("/Game/Blueprints/UI/"),
        TEXT("/Game/Blueprints/"),
        TEXT("/Game/")
    };
    
    // Build search paths
    for (const FString& Dir : CommonDirs)
    {
        // Original name
        SearchPaths.Add(Dir + AssetName);
        SearchPaths.Add(Dir + CleanName);
        
        // With WBP_ prefix
        if (!AssetName.StartsWith(TEXT("WBP_")))
        {
            SearchPaths.Add(Dir + TEXT("WBP_") + AssetName);
            SearchPaths.Add(Dir + TEXT("WBP_") + CleanName);
        }
        
        // With BP_ prefix
        if (!AssetName.StartsWith(TEXT("BP_")))
        {
            SearchPaths.Add(Dir + TEXT("BP_") + AssetName);
            SearchPaths.Add(Dir + TEXT("BP_") + CleanName);
        }
    }
    
    return SearchPaths;
}

FString FUnrealMCPCommonUtils::NormalizeAssetPath(const FString& AssetPath)
{
    FString CleanPath = AssetPath;
    CleanPath.TrimStartAndEndInline();
    
    // Remove leading slashes and Game/ prefix for normalization
    if (CleanPath.StartsWith(TEXT("/")))
    {
        CleanPath.RemoveFromStart(TEXT("/"));
    }
    if (CleanPath.StartsWith(TEXT("Game/")))
    {
        CleanPath.RemoveFromStart(TEXT("Game/"));
    }
    
    // Get just the filename if it's a full path
    if (CleanPath.Contains(TEXT("/")))
    {
        CleanPath = FPaths::GetBaseFilename(CleanPath);
    }
    
    return CleanPath;
}

bool FUnrealMCPCommonUtils::IsValidAssetPath(const FString& AssetPath)
{
    return UEditorAssetLibrary::DoesAssetExist(AssetPath);
}

// Blueprint Node Creation Functions
UK2Node_InputAction* FUnrealMCPCommonUtils::CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& Position)
{
    if (!Graph)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateInputActionNode: Graph is null"));
        return nullptr;
    }

    if (ActionName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("CreateInputActionNode: ActionName is empty"));
        return nullptr;
    }

    // Create the proper UK2Node_InputAction node
    UK2Node_InputAction* InputActionNode = NewObject<UK2Node_InputAction>(Graph);
    if (!InputActionNode)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateInputActionNode: Failed to create UK2Node_InputAction"));
        return nullptr;
    }

    // Set the input action name
    InputActionNode->InputActionName = FName(*ActionName);
    
    // Set position
    InputActionNode->NodePosX = Position.X;
    InputActionNode->NodePosY = Position.Y;
    
    // Create new GUID for the node
    InputActionNode->CreateNewGuid();
    
    // Add the node to the graph
    Graph->AddNode(InputActionNode, true, true);
    
    // Post-placement setup
    InputActionNode->PostPlacedNewNode();
    InputActionNode->AllocateDefaultPins();
    
    UE_LOG(LogTemp, Display, TEXT("CreateInputActionNode: Successfully created input action node for '%s'"), *ActionName);
    return InputActionNode;
}

// Blueprint node inspection utilities
UEdGraphNode* FUnrealMCPCommonUtils::FindNodeInBlueprint(UBlueprint* Blueprint, const FString& NodeName, const FString& GraphName)
{
    if (!Blueprint)
    {
        return nullptr;
    }

    // Get all graphs from the Blueprint
    TArray<UEdGraph*> GraphsToSearch = GetAllGraphsFromBlueprint(Blueprint);
    
    // If a specific graph is requested, filter to only that graph
    if (!GraphName.IsEmpty())
    {
        GraphsToSearch = GraphsToSearch.FilterByPredicate([&GraphName](UEdGraph* Graph)
        {
            return Graph && (Graph->GetName().Contains(GraphName) || 
                           Graph->GetName().Equals(GraphName, ESearchCase::IgnoreCase));
        });
    }

    // Search for the node in all relevant graphs
    for (UEdGraph* Graph : GraphsToSearch)
    {
        if (UEdGraphNode* FoundNode = FindNodeInGraph(Graph, NodeName))
        {
            return FoundNode;
        }
    }

    return nullptr;
}

UEdGraphNode* FUnrealMCPCommonUtils::FindNodeInGraph(UEdGraph* Graph, const FString& NodeName)
{
    if (!Graph)
    {
        return nullptr;
    }

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node)
        {
            continue;
        }

        // Check node title/display name
        FText NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle);
        if (NodeTitle.ToString().Contains(NodeName) || NodeTitle.ToString().Equals(NodeName, ESearchCase::IgnoreCase))
        {
            return Node;
        }

        // Check node class name
        FString NodeClassName = Node->GetClass()->GetName();
        if (NodeClassName.Contains(NodeName) || NodeClassName.Equals(NodeName, ESearchCase::IgnoreCase))
        {
            return Node;
        }

        // For function call nodes, check the function name
        if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(Node))
        {
            if (UFunction* Function = FunctionNode->GetTargetFunction())
            {
                FString FunctionName = Function->GetName();
                if (FunctionName.Contains(NodeName) || FunctionName.Equals(NodeName, ESearchCase::IgnoreCase))
                {
                    return Node;
                }
            }
        }

        // For variable get/set nodes, check variable name
        if (UK2Node_Variable* VariableNode = Cast<UK2Node_Variable>(Node))
        {
            FString VariableName = VariableNode->GetVarName().ToString();
            if (VariableName.Contains(NodeName) || VariableName.Equals(NodeName, ESearchCase::IgnoreCase))
            {
                return Node;
            }
        }
    }

    return nullptr;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::GetNodePinInfoRuntime(UEdGraphNode* Node, const FString& PinName)
{
    TSharedPtr<FJsonObject> PinInfoObj = MakeShared<FJsonObject>();
    
    if (!Node)
    {
        return PinInfoObj;
    }

    // Find the requested pin
    UEdGraphPin* FoundPin = nullptr;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin)
        {
            continue;
        }

        // Check exact pin name match
        if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase))
        {
            FoundPin = Pin;
            break;
        }

        // Check pin display name if different from pin name
        FText DisplayName = Pin->PinFriendlyName;
        if (!DisplayName.IsEmpty() && DisplayName.ToString().Equals(PinName, ESearchCase::IgnoreCase))
        {
            FoundPin = Pin;
            break;
        }
    }

    if (FoundPin)
    {
        // Get pin type information
        TSharedPtr<FJsonObject> PinTypeInfo = GetPinTypeInfo(FoundPin->PinType);
        
        PinInfoObj->SetStringField(TEXT("pin_type"), GetPinCategoryDisplayName(FoundPin->PinType.PinCategory));
        PinInfoObj->SetStringField(TEXT("expected_type"), FoundPin->PinType.PinSubCategory.ToString());
        PinInfoObj->SetStringField(TEXT("description"), FoundPin->PinToolTip.IsEmpty() ? TEXT("No description available") : FoundPin->PinToolTip);
        PinInfoObj->SetBoolField(TEXT("is_required"), true); // Most pins are required by default in UE
        PinInfoObj->SetBoolField(TEXT("is_input"), FoundPin->Direction == EGPD_Input);
        PinInfoObj->SetBoolField(TEXT("is_reference"), FoundPin->PinType.bIsReference);
        PinInfoObj->SetBoolField(TEXT("is_array"), FoundPin->PinType.IsArray());
        PinInfoObj->SetObjectField(TEXT("pin_type_details"), PinTypeInfo);
        
        // Add connection information
        PinInfoObj->SetNumberField(TEXT("linked_to_count"), FoundPin->LinkedTo.Num());
        
        TArray<TSharedPtr<FJsonValue>> LinkedPins;
        for (UEdGraphPin* LinkedPin : FoundPin->LinkedTo)
        {
            if (LinkedPin && LinkedPin->GetOwningNode())
            {
                TSharedPtr<FJsonObject> LinkedPinInfo = MakeShared<FJsonObject>();
                LinkedPinInfo->SetStringField(TEXT("node_name"), LinkedPin->GetOwningNode()->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
                LinkedPinInfo->SetStringField(TEXT("pin_name"), LinkedPin->PinName.ToString());
                LinkedPins.Add(MakeShared<FJsonValueObject>(LinkedPinInfo));
            }
        }
        PinInfoObj->SetArrayField(TEXT("linked_to"), LinkedPins);
    }

    return PinInfoObj;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::GetPinTypeInfo(const FEdGraphPinType& PinType)
{
    TSharedPtr<FJsonObject> TypeInfo = MakeShared<FJsonObject>();
    
    TypeInfo->SetStringField(TEXT("category"), PinType.PinCategory.ToString());
    TypeInfo->SetStringField(TEXT("subcategory"), PinType.PinSubCategory.ToString());
    TypeInfo->SetBoolField(TEXT("is_array"), PinType.IsArray());
    TypeInfo->SetBoolField(TEXT("is_reference"), PinType.bIsReference);
    TypeInfo->SetBoolField(TEXT("is_const"), PinType.bIsConst);
    TypeInfo->SetBoolField(TEXT("is_weak_pointer"), PinType.bIsWeakPointer);
    
    // Add container type information if it's a container
    if (PinType.IsContainer())
    {
        TypeInfo->SetStringField(TEXT("container_type"), TEXT("Container")); // Simplified for UE 5.7
        TypeInfo->SetStringField(TEXT("value_category"), TEXT("Unknown"));
        TypeInfo->SetStringField(TEXT("value_subcategory"), TEXT("Unknown"));
    }

    // Add object information if it's an object type
    if (PinType.PinSubCategoryObject.IsValid())
    {
        if (UClass* Class = Cast<UClass>(PinType.PinSubCategoryObject.Get()))
        {
            TypeInfo->SetStringField(TEXT("object_class"), Class->GetName());
            TypeInfo->SetStringField(TEXT("object_class_path"), Class->GetPathName());
        }
        else if (UScriptStruct* Struct = Cast<UScriptStruct>(PinType.PinSubCategoryObject.Get()))
        {
            TypeInfo->SetStringField(TEXT("struct_name"), Struct->GetName());
            TypeInfo->SetStringField(TEXT("struct_path"), Struct->GetPathName());
        }
    }

    return TypeInfo;
}

FString FUnrealMCPCommonUtils::GetPinCategoryDisplayName(const FName& Category)
{
    if (Category == UEdGraphSchema_K2::PC_Boolean)
    {
        return TEXT("bool");
    }
    else if (Category == UEdGraphSchema_K2::PC_Byte)
    {
        return TEXT("byte");
    }
    else if (Category == UEdGraphSchema_K2::PC_Int)
    {
        return TEXT("int");
    }
    else if (Category == UEdGraphSchema_K2::PC_Int64)
    {
        return TEXT("int64");
    }
    else if (Category == UEdGraphSchema_K2::PC_Real)
    {
        return TEXT("real");
    }
    else if (Category == UEdGraphSchema_K2::PC_Double)
    {
        return TEXT("double");
    }
    else if (Category == UEdGraphSchema_K2::PC_String)
    {
        return TEXT("string");
    }
    else if (Category == UEdGraphSchema_K2::PC_Text)
    {
        return TEXT("text");
    }
    else if (Category == UEdGraphSchema_K2::PC_Name)
    {
        return TEXT("name");
    }
    else if (Category == UEdGraphSchema_K2::PC_Object)
    {
        return TEXT("object");
    }
    else if (Category == UEdGraphSchema_K2::PC_Class)
    {
        return TEXT("class");
    }
    else if (Category == UEdGraphSchema_K2::PC_Struct)
    {
        return TEXT("struct");
    }
    else if (Category == UEdGraphSchema_K2::PC_Exec)
    {
        return TEXT("exec");
    }
    else if (Category == UEdGraphSchema_K2::PC_Wildcard)
    {
        return TEXT("wildcard");
    }
    
    return Category.ToString();
}

TArray<UEdGraph*> FUnrealMCPCommonUtils::GetAllGraphsFromBlueprint(UBlueprint* Blueprint)
{
    TArray<UEdGraph*> AllGraphs;
    
    if (!Blueprint)
    {
        return AllGraphs;
    }

    // Add all UbergraphPages (event graphs)
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph)
        {
            AllGraphs.Add(Graph);
        }
    }

    // Add all function graphs
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (Graph)
        {
            AllGraphs.Add(Graph);
        }
    }

    // Add macro graphs if any
    for (UEdGraph* Graph : Blueprint->MacroGraphs)
    {
        if (Graph)
        {
            AllGraphs.Add(Graph);
        }
    }

    return AllGraphs;
}