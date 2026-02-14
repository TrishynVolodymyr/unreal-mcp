#pragma once

#include "CoreMinimal.h"
#include "PCGGraph.h"
#include "PCGNode.h"

namespace PCGNodeUtils
{
	inline UPCGNode* FindNodeByName(UPCGGraph* Graph, const FString& NodeName)
	{
		if (!Graph)
		{
			return nullptr;
		}
		if (Graph->GetInputNode() && Graph->GetInputNode()->GetName() == NodeName)
		{
			return Graph->GetInputNode();
		}
		if (Graph->GetOutputNode() && Graph->GetOutputNode()->GetName() == NodeName)
		{
			return Graph->GetOutputNode();
		}
		for (UPCGNode* Node : Graph->GetNodes())
		{
			if (Node && Node->GetName() == NodeName)
			{
				return Node;
			}
		}
		return nullptr;
	}
}
