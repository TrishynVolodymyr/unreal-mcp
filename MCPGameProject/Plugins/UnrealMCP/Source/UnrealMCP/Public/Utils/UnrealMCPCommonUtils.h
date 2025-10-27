#pragma once

#include "CoreMinimal.h"
#include "Json.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "UObject/Object.h"
#include "UObject/UnrealType.h"

// Forward declarations
class AActor;
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UK2Node_Event;
class UK2Node_CallFunction;
class UK2Node_VariableGet;
class UK2Node_VariableSet;
class UK2Node_InputAction;
class UK2Node_Self;
class UFunction;

/**
 * Common utilities for UnrealMCP commands
 */
class UNREALMCP_API FUnrealMCPCommonUtils
{
public:
    // JSON utilities
    static TSharedPtr<FJsonObject> CreateErrorResponse(const FString& Message);
    static TSharedPtr<FJsonObject> CreateSuccessResponse(const FString& Message = TEXT(""));
    static void GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray);
    static void GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray);
    static FVector2D GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
    static FVector GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
    static FRotator GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
    
    // Actor utilities
    static TSharedPtr<FJsonValue> ActorToJson(AActor* Actor);
    static TSharedPtr<FJsonObject> ActorToJsonObject(AActor* Actor, bool bDetailed = false);
    
    // Blueprint utilities
    static UBlueprint* FindBlueprint(const FString& BlueprintName);
    static UBlueprint* FindBlueprintByName(const FString& BlueprintName);
    static UEdGraph* FindOrCreateEventGraph(UBlueprint* Blueprint);
    
    // Blueprint node utilities
    static UK2Node_Event* CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& Position);
    static UK2Node_CallFunction* CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& Position);
    static UK2Node_VariableGet* CreateVariableGetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position);
    static UK2Node_VariableSet* CreateVariableSetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position);
    static UK2Node_InputAction* CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& Position);
    static UK2Node_Self* CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& Position);
    static bool ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, 
                                UEdGraphNode* TargetNode, const FString& TargetPinName);
    static UEdGraphPin* FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction = EGPD_MAX);
    static UK2Node_Event* FindExistingEventNode(UEdGraph* Graph, const FString& EventName);

    // Property utilities
    static bool SetObjectProperty(UObject* Object, const FString& PropertyName, 
                                 const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage);

    // Helper to parse JSON array to FVector
    static bool ParseVector(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FVector& OutVector);

    // Helper to parse JSON array to FRotator
    static bool ParseRotator(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FRotator& OutRotator);

    // Helper to parse JSON array to FLinearColor (accepts [R,G,B] or [R,G,B,A], assumes 0-1 range)
    static bool ParseLinearColor(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FLinearColor& OutColor);

    // Helper to find an actor by name
    static AActor* FindActorByName(const FString& ActorName);

    // Helper to set an FProperty value from a JsonValue
    static bool SetPropertyFromJson(FProperty* Property, void* ContainerPtr, const TSharedPtr<FJsonValue>& JsonValue);

    // Helper to call a BlueprintCallable function by name with FString parameters
    static bool CallFunctionByName(UObject* Target, const FString& FunctionName, const TArray<FString>& StringParams, FString& OutError);

    // Helper to set an FProperty value from a JsonValue
    static bool SetPropertyValueFromJsonValue(FProperty* Property, void* PropertyValue, const TSharedPtr<FJsonValue>& JsonValue);

    // Package path utilities
    static FString GetGameContentPath() { return TEXT("/Game/"); }
    static FString GetEngineScriptPath() { return TEXT("/Script/Engine"); }
    static FString GetCoreScriptPath() { return TEXT("/Script/CoreUObject"); }
    static FString GetUMGScriptPath() { return TEXT("/Script/UMG"); }
    
    static FString BuildGamePath(const FString& Path) 
    { 
        FString CleanPath = Path;
        if (CleanPath.StartsWith(TEXT("/")))
        {
            CleanPath = CleanPath.RightChop(1);
        }
        return FString::Printf(TEXT("%s%s"), *GetGameContentPath(), *CleanPath);
    }
    
    static FString BuildEnginePath(const FString& Path)
    {
        return FString::Printf(TEXT("%s.%s"), *GetEngineScriptPath(), *Path);
    }
    
    static FString BuildCorePath(const FString& Path)
    {
        return FString::Printf(TEXT("%s.%s"), *GetCoreScriptPath(), *Path);
    }
    
    static FString BuildUMGPath(const FString& Path)
    {
        return FString::Printf(TEXT("%s.%s"), *GetUMGScriptPath(), *Path);
    }

    // Asset Discovery utilities
    static TArray<FString> FindAssetsByType(const FString& AssetType, const FString& SearchPath = TEXT("/Game"));
    static TArray<FString> FindAssetsByName(const FString& AssetName, const FString& SearchPath = TEXT("/Game"));
    static TArray<FString> FindWidgetBlueprints(const FString& WidgetName = TEXT(""), const FString& SearchPath = TEXT("/Game"));
    static TArray<FString> FindBlueprints(const FString& BlueprintName = TEXT(""), const FString& SearchPath = TEXT("/Game"));
    static TArray<FString> FindDataTables(const FString& TableName = TEXT(""), const FString& SearchPath = TEXT("/Game"));
    
    // Enhanced asset loading with discovery
    static UClass* FindWidgetClass(const FString& WidgetPath);
    static UBlueprint* FindWidgetBlueprint(const FString& WidgetPath);
    static UObject* FindAssetByPath(const FString& AssetPath);
    static UObject* FindAssetByName(const FString& AssetName, const FString& AssetType = TEXT(""));
    static UScriptStruct* FindStructType(const FString& StructPath);
    
    // Asset path utilities
    static TArray<FString> GetCommonAssetSearchPaths(const FString& AssetName);
    static FString NormalizeAssetPath(const FString& AssetPath);
    static bool IsValidAssetPath(const FString& AssetPath);

    // Blueprint node inspection utilities
    static UEdGraphNode* FindNodeInBlueprint(UBlueprint* Blueprint, const FString& NodeName, const FString& GraphName = TEXT(""));
    static UEdGraphNode* FindNodeInGraph(UEdGraph* Graph, const FString& NodeName);
    static TSharedPtr<FJsonObject> GetNodePinInfoRuntime(UEdGraphNode* Node, const FString& PinName);
    static TSharedPtr<FJsonObject> GetPinTypeInfo(const FEdGraphPinType& PinType);
    static FString GetPinCategoryDisplayName(const FName& Category);
    static TArray<UEdGraph*> GetAllGraphsFromBlueprint(UBlueprint* Blueprint);

private:
    // ... existing code ...
}; 
