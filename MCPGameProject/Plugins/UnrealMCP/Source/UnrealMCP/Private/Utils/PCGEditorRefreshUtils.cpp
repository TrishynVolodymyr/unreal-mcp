// PCG Editor Graph Refresh Utility
//
// Problem: PCG dual-graph architecture (UPCGGraph data + UPCGEditorGraph visual).
// When we modify UPCGGraph programmatically, the visual editor doesn't update.
// Close+reopen doesn't help because FPCGEditor::Initialize() skips
// InitFromNodeGraph() if the old UPCGEditorGraph pointer is still non-null.
//
// Solution: Null the private PCGEditorGraph member before reopening.
// We use a compile-time access hack since it's not a UPROPERTY (no reflection).

#include "Utils/PCGEditorRefreshUtils.h"

// Access the private PCGEditorGraph member of UPCGGraph
// This is necessary because:
// 1. PCGEditorGraph is NOT a UPROPERTY (no reflection access)
// 2. FPCGEditor is the only friend class with access
// 3. UPCGEditorGraph::ReconstructGraph() is not DLL-exported
// 4. Close+reopen skips rebuild if old EditorGraph pointer exists
// HACK: Access protected PCGEditorGraph member
// Required because it's not a UPROPERTY and there's no public API to reset it
#define protected public
#define private public
#include "PCGGraph.h"
#undef private
#undef protected

#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"

void PCGEditorRefreshUtils::RefreshEditorGraph(UPCGGraph* PCGGraph)
{
	if (!PCGGraph || !GEditor)
	{
		return;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
	{
		return;
	}

	// Check if this graph has an editor open
	TArray<IAssetEditorInstance*> Editors = AssetEditorSubsystem->FindEditorsForAsset(PCGGraph);
	if (Editors.Num() == 0)
	{
		return;
	}

	// Close all editors
	AssetEditorSubsystem->CloseAllEditorsForAsset(PCGGraph);

	// Null out the cached UPCGEditorGraph so FPCGEditor::Initialize()
	// creates a fresh one via InitFromNodeGraph() on reopen
	PCGGraph->PCGEditorGraph = nullptr;

	// Reopen the editor — now it will rebuild the visual graph from current data
	AssetEditorSubsystem->OpenEditorForAsset(PCGGraph);

	UE_LOG(LogTemp, Display, TEXT("PCGEditorRefreshUtils: Refreshed editor for %s"), *PCGGraph->GetName());
}
