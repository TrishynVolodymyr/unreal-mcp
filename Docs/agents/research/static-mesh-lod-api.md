# Research: StaticMesh LOD API (UE 5.7)

## Summary

UE 5.7 provides comprehensive LOD management through `UStaticMesh` directly and the `UStaticMeshEditorSubsystem` (editor scripting). Key APIs: `SetNumSourceModels()` for LOD count, `FbxMeshUtils::ImportStaticMeshLOD()` for FBX LOD import, `FStaticMeshRenderData::ScreenSize[]` for transition thresholds, and `FMeshReductionSettings` for auto-reduction. The `UStaticMeshEditorSubsystem` wraps all of this with validated, script-friendly methods.

## 1. Get Mesh Metadata

### LOD Count
```cpp
#include "Engine/StaticMesh.h"

UStaticMesh* Mesh = ...;
int32 NumSourceModels = Mesh->GetNumSourceModels();  // Source/editable LOD count
int32 NumRenderLODs = Mesh->GetNumLODs();             // Render data LOD count (after build)
```

### Verts/Tris Per LOD
```cpp
// Per-LOD stats (from render data)
for (int32 LODIdx = 0; LODIdx < Mesh->GetNumLODs(); ++LODIdx)
{
    int32 Verts = Mesh->GetNumVertices(LODIdx);    // BlueprintPure
    int32 Tris = Mesh->GetNumTriangles(LODIdx);     // BlueprintPure
    int32 TexCoords = Mesh->GetNumTexCoords(LODIdx);
    int32 Sections = Mesh->GetNumSections(LODIdx);
}

// Direct LODResources access (what our ImportStaticMesh already uses)
if (Mesh->GetRenderData() && Mesh->GetRenderData()->LODResources.Num() > 0)
{
    const FStaticMeshLODResources& LOD0 = Mesh->GetRenderData()->LODResources[0];
    int32 Verts = LOD0.GetNumVertices();
    int32 Tris = LOD0.GetNumTriangles();
}
```

### Bounds
```cpp
FBoxSphereBounds Bounds = Mesh->GetBounds();    // BlueprintPure
FBox BBox = Mesh->GetBoundingBox();              // BlueprintPure, includes bounds extension
```

### Material Slots
```cpp
const TArray<FStaticMaterial>& Materials = Mesh->GetStaticMaterials();
int32 NumMats = Materials.Num();
UMaterialInterface* Mat = Mesh->GetMaterial(MaterialIndex); // BlueprintPure
```

### Screen Sizes
```cpp
// From RenderData
FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
for (int32 i = 0; i < Mesh->GetNumLODs(); ++i)
{
    float ScreenSize = RenderData->ScreenSize[i].Default;
}

// From SourceModel
for (int32 i = 0; i < Mesh->GetNumSourceModels(); ++i)
{
    float ScreenSize = Mesh->GetSourceModel(i).ScreenSize.Default;
}

// Or via subsystem
UStaticMeshEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UStaticMeshEditorSubsystem>();
TArray<float> ScreenSizes = Subsystem->GetLodScreenSizes(Mesh);
```

### LOD Group
```cpp
FName LODGroup = Mesh->GetLODGroup();
bool bAutoScreenSize = Mesh->GetAutoComputeLODScreenSize();
```

## 2. Set LOD Count

```cpp
// Set exact number of source models (LODs)
Mesh->SetNumSourceModels(3);  // Now has LOD 0, 1, 2

// Add a single new LOD
FStaticMeshSourceModel& NewModel = Mesh->AddSourceModel();

// Remove a specific LOD
Mesh->RemoveSourceModel(2);  // Remove LOD 2

// Access source model for config
FStaticMeshSourceModel& SrcModel = Mesh->GetSourceModel(LODIndex);
SrcModel.BuildSettings = ...;
SrcModel.ReductionSettings = ...;
SrcModel.ScreenSize = 0.5f;

// MAX_STATIC_MESH_LODS = 8
```

## 3. Import Mesh Into LOD Slot

### Method A: FbxMeshUtils (direct, used by the engine)
```cpp
#include "FbxMeshUtils.h"

// Import FBX file as a specific LOD level
bool bSuccess = FbxMeshUtils::ImportStaticMeshLOD(
    BaseStaticMesh,    // UStaticMesh* - target mesh
    Filename,          // const FString& - FBX file path
    LODLevel,          // int32 - which LOD slot (0-based)
    false              // bool bAsync - false for synchronous
);
```

### Method B: UStaticMeshEditorSubsystem (recommended for scripting)
```cpp
#include "StaticMeshEditorSubsystem.h"

UStaticMeshEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UStaticMeshEditorSubsystem>();

// Import or re-import a LOD from FBX file
// LODIndex can be == NumSourceModels to add a new LOD
// Returns LODIndex on success, INDEX_NONE on failure
int32 Result = Subsystem->ImportLOD(BaseStaticMesh, LODIndex, SourceFilename);
```

### Method C: SetCustomLOD (copy from another mesh)
```cpp
// Copy LOD data from another static mesh
bool bSuccess = DestMesh->SetCustomLOD(SourceStaticMesh, LodIndex, SourceDataFilename);
```

### Method D: SetLodFromStaticMesh (subsystem, mesh-to-mesh)
```cpp
// Copy LOD from one mesh to another
int32 Result = Subsystem->SetLodFromStaticMesh(
    DestinationStaticMesh, DestinationLodIndex,
    SourceStaticMesh, SourceLodIndex,
    bReuseExistingMaterialSlots
);
```

### Note on ImportStaticMeshLOD internals:
In UE 5.7, `ImportStaticMeshLOD` first tries Interchange (`UInterchangeMeshUtilities::ImportCustomLod`), and falls back to legacy FBX importer if Interchange can't handle it. The Interchange path is async-capable.

## 4. Set LOD Screen Sizes

### Direct approach
```cpp
Mesh->SetAutoComputeLODScreenSize(false);  // Disable auto-compute first

FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
RenderData->ScreenSize[0].Default = 1.0f;   // LOD 0 always visible
RenderData->ScreenSize[1].Default = 0.5f;   // LOD 1 at 50% screen
RenderData->ScreenSize[2].Default = 0.25f;  // LOD 2 at 25% screen

// Also set on source models for persistence
Mesh->GetSourceModel(0).ScreenSize = 1.0f;
Mesh->GetSourceModel(1).ScreenSize = 0.5f;
Mesh->GetSourceModel(2).ScreenSize = 0.25f;
```

### Via subsystem (recommended - handles validation/monotonic clamping)
```cpp
UStaticMeshEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UStaticMeshEditorSubsystem>();

TArray<float> ScreenSizes = { 1.0f, 0.5f, 0.25f };
bool bSuccess = Subsystem->SetLodScreenSizes(Mesh, ScreenSizes);
// Automatically:
// - Disables bAutoComputeLODScreenSize
// - Clamps values to be monotonically decreasing (delta 0.0001f)
// - Sets both RenderData->ScreenSize[i] and SourceModel.ScreenSize
```

## 5. Auto LOD Reduction

### Via UStaticMeshEditorSubsystem::SetLods (simplest)
```cpp
#include "StaticMeshEditorSubsystem.h"
#include "StaticMeshEditorSubsystemHelpers.h"

UStaticMeshEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UStaticMeshEditorSubsystem>();

FStaticMeshReductionOptions Options;
Options.bAutoComputeLODScreenSize = true;

// Each entry = one LOD (LOD 0 is original, LOD 1+ are reduced)
Options.ReductionSettings.Add({ 1.0f, 1.0f });    // LOD 0: 100% tris, screen 1.0
Options.ReductionSettings.Add({ 0.5f, 0.5f });    // LOD 1: 50% tris, screen 0.5
Options.ReductionSettings.Add({ 0.25f, 0.25f });  // LOD 2: 25% tris, screen 0.25

int32 NumLODs = Subsystem->SetLods(Mesh, Options);  // Returns num LODs created
// Internally: resets to LOD 0, adds source models, sets ReductionSettings, calls PostEditChange()
```

### Per-LOD reduction settings (fine-grained)
```cpp
FStaticMeshSourceModel& SrcModel = Mesh->GetSourceModel(LODIndex);
SrcModel.ReductionSettings.PercentTriangles = 0.5f;      // 50% triangles
SrcModel.ReductionSettings.PercentVertices = 1.0f;        // No vertex limit
SrcModel.ReductionSettings.MaxDeviation = 0.0f;           // Max distance deviation
SrcModel.ReductionSettings.PixelError = 8.0f;             // Pixel error allowance
SrcModel.ReductionSettings.WeldingThreshold = 0.0f;
SrcModel.ReductionSettings.HardAngleThreshold = 45.0f;
SrcModel.ReductionSettings.BaseLODModel = 0;               // Reduce from LOD 0
SrcModel.ReductionSettings.SilhouetteImportance = EMeshFeatureImportance::Normal;
SrcModel.ReductionSettings.TextureImportance = EMeshFeatureImportance::Normal;
SrcModel.ReductionSettings.ShadingImportance = EMeshFeatureImportance::Normal;
SrcModel.ReductionSettings.TerminationCriterion = EStaticMeshReductionTerimationCriterion::Triangles;

// Or via subsystem
Subsystem->SetLodReductionSettings(Mesh, LODIndex, ReductionSettings);
Subsystem->GetLodReductionSettings(Mesh, LODIndex, OutReductionSettings);
```

## 6. Rebuild Mesh After Modifications

```cpp
// Full rebuild (triggers mesh compilation, may be async)
Mesh->Build(/*bSilent=*/true);

// Notify of property changes (triggers rebuild + UI refresh)
Mesh->PostEditChange();

// Mark for save
Mesh->MarkPackageDirty();

// IMPORTANT from our existing code: Build() can trigger TaskGraph recursion
// when called from MCP thread. PostEditChange() is safer as it handles async.
// Or use the subsystem methods which handle this internally.

// Batch build for multiple meshes
TArray<UStaticMesh*> Meshes = { Mesh1, Mesh2 };
UStaticMesh::BatchBuild(Meshes, /*bSilent=*/true);
```

## 7. Module Dependencies for Build.cs

Current `UnrealMCP.Build.cs` already has:
- `UnrealEd` (has `FbxMeshUtils`, `FbxFactory`)
- `Engine` (has `UStaticMesh`)
- `AssetTools`, `AssetRegistry`

**Needed additions:**
```csharp
// In the bBuildEditor block:
"StaticMeshEditor",    // UStaticMeshEditorSubsystem, FStaticMeshReductionOptions
```

**Headers needed:**
```cpp
#include "Engine/StaticMesh.h"                    // UStaticMesh (already used)
#include "StaticMeshResources.h"                  // FStaticMeshRenderData, FStaticMeshLODResources
#include "StaticMeshEditorSubsystem.h"            // UStaticMeshEditorSubsystem
#include "StaticMeshEditorSubsystemHelpers.h"     // FStaticMeshReductionOptions
#include "FbxMeshUtils.h"                         // FbxMeshUtils::ImportStaticMeshLOD
#include "MeshReductionSettings.h"                // FMeshReductionSettings
```

## Key Constants

```cpp
#define MAX_STATIC_MESH_LODS 8  // Maximum LOD levels allowed
```

## Thread Safety Warning

From our existing `ImportStaticMeshCommand.cpp` (line 119):
> Don't call Build() here -- it triggers TaskGraph recursion when called from MCP thread.

For LOD operations, prefer using `UStaticMeshEditorSubsystem` methods which internally handle `PostEditChange()` correctly. If calling directly, wrap in `AsyncTask(ENamedThreads::GameThread, [=]{ ... })` or use the `ExecuteOnGameThread` pattern our plugin already uses.

## Reference Files

- UE Source: `Engine/Source/Runtime/Engine/Classes/Engine/StaticMesh.h` (lines 1939-2155)
- UE Source: `Engine/Source/Runtime/Engine/Classes/Engine/StaticMeshSourceData.h` (FStaticMeshSourceModel)
- UE Source: `Engine/Source/Runtime/Engine/Public/StaticMeshResources.h` (FStaticMeshRenderData, line 773+)
- UE Source: `Engine/Source/Runtime/Engine/Public/MeshReductionSettings.h` (FMeshReductionSettings)
- UE Source: `Engine/Source/Editor/StaticMeshEditor/Public/StaticMeshEditorSubsystem.h` (full scripting API)
- UE Source: `Engine/Source/Editor/StaticMeshEditor/Public/StaticMeshEditorSubsystemHelpers.h` (FStaticMeshReductionOptions)
- UE Source: `Engine/Source/Editor/UnrealEd/Public/FbxMeshUtils.h` (ImportStaticMeshLOD)
- Project: `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/Editor/ImportStaticMeshCommand.cpp`
