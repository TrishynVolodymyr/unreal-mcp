#pragma once

#include "CoreMinimal.h"

class UPCGGraph;

/**
 * Utility for refreshing the PCG Editor Graph after programmatic modifications.
 * 
 * PCG uses a dual-graph architecture:
 *   - UPCGGraph (runtime data, what MCP modifies)
 *   - UPCGEditorGraph (UEdGraph mirror for Slate, what the user sees)
 * 
 * When UPCGGraph is modified directly, UPCGEditorGraph doesn't update automatically.
 * This utility closes and reopens the editor tab to force a full visual refresh.
 */
namespace PCGEditorRefreshUtils
{
	/**
	 * Refresh the PCG editor for the given UPCGGraph by closing and reopening the tab.
	 * Safe to call even if the editor tab isn't open (no-op in that case).
	 * Must be called on the Game Thread.
	 */
	UNREALMCP_API void RefreshEditorGraph(UPCGGraph* PCGGraph);
}
