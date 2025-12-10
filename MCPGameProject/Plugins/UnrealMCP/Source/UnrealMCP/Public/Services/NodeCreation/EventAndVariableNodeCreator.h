#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Dom/JsonObject.h"

/**
 * Service class for creating event and variable Blueprint nodes
 * Handles: Component bound events, standard events, macros, variable getters/setters, struct nodes
 */
class UNREALMCP_API FEventAndVariableNodeCreator
{
public:
	/**
	 * Get the singleton instance
	 */
	static FEventAndVariableNodeCreator& Get();

	/**
	 * Try to create a component bound event node
	 * @return true if this function handled the node creation
	 */
	bool TryCreateComponentBoundEventNode(TSharedPtr<FJsonObject> ParamsObject, UBlueprint* Blueprint,
		const FString& BlueprintName, UEdGraph* EventGraph, int32 PositionX, int32 PositionY,
		UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType, FString& OutErrorMessage);

	/**
	 * Try to create a standard event node (BeginPlay, Tick, etc.)
	 * @return true if this function handled the node creation
	 */
	bool TryCreateStandardEventNode(const FString& FunctionName, UEdGraph* EventGraph,
		int32 PositionX, int32 PositionY,
		UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType);

	/**
	 * Try to create a macro instance node
	 * @return true if this function handled the node creation
	 */
	bool TryCreateMacroNode(const FString& FunctionName, UEdGraph* EventGraph,
		int32 PositionX, int32 PositionY,
		UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType, FString& OutErrorMessage);

	/**
	 * Try to create a variable getter/setter node
	 * @return true if this function handled the node creation
	 */
	bool TryCreateVariableNode(const FString& FunctionName, TSharedPtr<FJsonObject> ParamsObject,
		UBlueprint* Blueprint, const FString& BlueprintName, UEdGraph* EventGraph,
		int32 PositionX, int32 PositionY,
		UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType, FString& OutErrorMessage);

	/**
	 * Try to create a struct node (BreakStruct/MakeStruct)
	 * @return true if this function handled the node creation
	 */
	bool TryCreateStructNode(const FString& FunctionName, TSharedPtr<FJsonObject> ParamsObject,
		UEdGraph* EventGraph, int32 PositionX, int32 PositionY,
		UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType, FString& OutErrorMessage);

private:
	/** Private constructor for singleton pattern */
	FEventAndVariableNodeCreator() = default;
};
