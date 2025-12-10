#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Dom/JsonObject.h"

/**
 * Service class for creating control flow Blueprint nodes
 * Handles: Literals, Branch, Sequence, CustomEvent, Cast nodes
 */
class UNREALMCP_API FControlFlowNodeCreator
{
public:
	/**
	 * Get the singleton instance
	 */
	static FControlFlowNodeCreator& Get();

	/**
	 * Try to create a literal/constant node
	 * @return true if this function handled the node creation
	 */
	bool TryCreateLiteralNode(const FString& FunctionName, TSharedPtr<FJsonObject> ParamsObject,
		UEdGraph* EventGraph, int32 PositionX, int32 PositionY,
		UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType);

	/**
	 * Try to create a Branch (IfThenElse) node
	 * @return true if this function handled the node creation
	 */
	bool TryCreateBranchNode(const FString& FunctionName, UEdGraph* EventGraph,
		int32 PositionX, int32 PositionY,
		UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType);

	/**
	 * Try to create a Sequence (ExecutionSequence) node
	 * @return true if this function handled the node creation
	 */
	bool TryCreateSequenceNode(const FString& FunctionName, UEdGraph* EventGraph,
		int32 PositionX, int32 PositionY,
		UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType);

	/**
	 * Try to create a CustomEvent node
	 * @return true if this function handled the node creation
	 */
	bool TryCreateCustomEventNode(const FString& FunctionName, TSharedPtr<FJsonObject> ParamsObject,
		UEdGraph* EventGraph, int32 PositionX, int32 PositionY,
		UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType);

	/**
	 * Try to create a Cast (DynamicCast) node
	 * @return true if this function handled the node creation
	 */
	bool TryCreateCastNode(const FString& FunctionName, TSharedPtr<FJsonObject> ParamsObject,
		UEdGraph* EventGraph, int32 PositionX, int32 PositionY,
		UEdGraphNode*& OutNode, FString& OutNodeTitle, FString& OutNodeType);

private:
	/** Private constructor for singleton pattern */
	FControlFlowNodeCreator() = default;
};
