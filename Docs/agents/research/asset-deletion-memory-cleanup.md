# Research: Proper UAsset Deletion from Memory in UE 5.7

## Summary

Our `delete_asset` command calls `UEditorAssetLibrary::DeleteAsset()`, which internally calls `ObjectTools::ForceDeleteObjects()` -- the most thorough deletion API in UE. This API replaces all references with nullptr, clears object flags, unloads packages, runs GC, and deletes from disk. Our code IS already calling the correct API. The bug is more subtle: either `ForceDeleteObjects` is silently failing/returning 0, or the import side is hitting stale in-memory state that survived the deletion.

## Call Chain (Our Code)

```
delete_asset TCP command
  -> FDeleteAssetCommand::Execute()                     [DeleteAssetCommand.cpp]
  -> FProjectService::DeleteAsset()                     [ProjectService.cpp:335]
  -> FProjectAssetOperations::DeleteAsset()             [ProjectAssetOperations.cpp:61]
  -> UEditorAssetLibrary::DeleteAsset()                 [EditorAssetLibrary.cpp:339]
  -> UEditorAssetSubsystem::DeleteAsset()               [EditorAssetSubsystem.cpp:795]
  -> ObjectTools::ForceDeleteObjects(objects, false)    [ObjectTools.cpp:3553]
```

All dispatched via `AsyncTask(ENamedThreads::GameThread, ...)` from `UnrealMCPBridge.cpp:456`, so it runs on the game thread.

## Root Cause Analysis

### The Exact Bug Path

1. `delete_asset` calls `UEditorAssetLibrary::DeleteAsset(path)` which calls `ObjectTools::ForceDeleteObjects`
2. `ForceDeleteObjects` should do full cleanup (details below)
3. Later, `import_static_mesh` calls `CreatePackage(*PackagePath)` (line 65 of ImportStaticMeshCommand.cpp)
4. `CreatePackage` does `FindObject<UPackage>(nullptr, *InName)` (UObjectGlobals.cpp:1040) -- finds OLD in-memory package if still alive
5. `FbxFactory::ImportObject` does `StaticFindObject(UObject::StaticClass(), InParent, *(Name.ToString()))` (FbxFactory.cpp:216) -- finds OLD UStaticMesh
6. Factory treats it as REIMPORT instead of fresh import -- updates old object rather than creating new one

### What ForceDeleteObjects Does (ObjectTools.cpp:3553-3981)

Full pipeline when working correctly:
1. `AddExtraObjectsToDelete` -- adds map build data, etc.
2. `RecursiveRetrieveReferencers` -- finds ALL objects referencing the deleted objects
3. Closes all editors for referencing assets
4. Deselects objects from editor selection
5. For Blueprints: destroys instances, SCS nodes, components, actors
6. **`ForceReplaceReferences(nullptr, ObjectsToReplace, ...)`** -- replaces ALL references with nullptr (line 3885)
7. `CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS)` -- first GC pass (line 3893)
8. `FAssetRegistryModule::AssetDeleted(ObjectToDelete)` -- unregisters from registry (line 3464)
9. `ObjectToDelete->ClearFlags(RF_Standalone | RF_Public)` -- makes GC-eligible (line 3467)
10. `FAssetRegistryModule::PackageDeleted(Package)` -- unregisters package (line 3597 in CleanupAfterSuccessfulDelete)
11. `CleanupAfterSuccessfulDelete(PotentialPackagesToDelete)` -- package unload + disk delete (line 3949)
    - Calls `GatherObjectReferencersForDeletion` on the package (`bPerformReferenceCheck=true` default)
    - **If package is still referenced: REMOVES it from unload list** (line 2549)
    - If not referenced: `UPackageTools::UnloadPackages()` + `CollectGarbage()` + disk file delete

### Three Likely Failure Points

**Failure Point 1: `OnAssetsCanDelete` delegate blocks deletion (line 3561-3567)**
Any delegate registered on `FEditorDelegates::OnAssetsCanDelete` can veto the deletion. If vetoed, `ForceDeleteObjects` returns 0 without logging (just shows a dialog that gets suppressed by `GIsRunningUnattendedScript`). Our code checks the return of `UEditorAssetLibrary::DeleteAsset` (bool) but the actual error may be swallowed.

**Failure Point 2: Package survives CleanupAfterSuccessfulDelete (line 2535-2549)**
`CleanupAfterSuccessfulDelete` is called with default `bPerformReferenceCheck=true`. It runs `GatherObjectReferencersForDeletion` on the package. If ANY external object still references the package (not the asset inside it), the package is removed from the unload list. Possible culprits:
- FLinkerLoad (package loader) still attached
- Transaction buffer holding a reference
- Async loading system has a pending reference
- Rendering resources still referencing the mesh

**Failure Point 3: AsyncTask(GameThread) context issues (line 456 of UnrealMCPBridge.cpp)**
`AsyncTask(GameThread)` runs inside the TaskGraph system. `ForceDeleteObjects` internally calls `FlushAsyncLoading()`, `FlushRenderingCommands()`, `CollectGarbage()`, and shows slow task UI. Some of these may have issues when called from within a TaskGraph task vs. a normal game thread tick. Our own code already works around this for FBX imports by using `FTSTicker` instead of `AsyncTask(GameThread)` -- the delete command may need the same treatment.

## UE 5.7 Available APIs

### Primary APIs for Asset Deletion

| Function | Header | Purpose |
|----------|--------|---------|
| `ObjectTools::ForceDeleteObjects(objects, showConfirm)` | `ObjectTools.h` | Nuclear option: replaces ALL refs with nullptr, then deletes objects + packages + disk. **Already used by our code.** |
| `ObjectTools::DeleteObjects(objects, showConfirm)` | `ObjectTools.h` | Safe delete: uses `FAssetDeleteModel` to scan refs first, shows UI if refs exist. |
| `ObjectTools::DeleteAssets(assetDataArray, showConfirm)` | `ObjectTools.h` | Takes FAssetData instead of UObject*. |
| `ObjectTools::DeleteSingleObject(obj, refCheck)` | `ObjectTools.h` | Low-level: clears flags, notifies registry. Does NOT delete from disk or unload package. |
| `ObjectTools::DeleteObjectsUnchecked(objects)` | `ObjectTools.h` | Calls DeleteSingleObject + CleanupAfterSuccessfulDelete. No reference check. |
| `ObjectTools::ForceReplaceReferences(nullptr, objects)` | `ObjectTools.h` | Replaces all refs to objects with nullptr across ALL loaded objects. |
| `ObjectTools::CleanupAfterSuccessfulDelete(packages)` | `ObjectTools.h` | Unloads packages, runs GC, deletes .uasset from disk. |
| `UPackageTools::UnloadPackages(packages)` | `PackageTools.h` | Unloads packages from memory, clears RF_Standalone, runs GC. |
| `UEditorAssetLibrary::DeleteAsset(path)` | `EditorAssetLibrary.h` | Thin wrapper around `ForceDeleteObjects`. |

### Supporting APIs

| Function | Purpose |
|----------|---------|
| `FAssetRegistryModule::AssetDeleted(obj)` | Remove asset from registry |
| `FAssetRegistryModule::PackageDeleted(pkg)` | Remove package from registry |
| `obj->ClearFlags(RF_Standalone \| RF_Public)` | Make object GC-eligible |
| `CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS)` | Run garbage collection |
| `GEditor->ResetTransaction(reason)` | Clear undo buffer (holds strong refs) |
| `FlushAsyncLoading()` | Complete pending async loads |
| `ResetLoaders(packages)` | Detach package linkers |
| `StaticFindObject(class, outer, name)` | Check if object exists in memory |
| `FindObject<UPackage>(nullptr, name)` | Check if package exists in memory |

## Recommended Fix Strategy

### Step 1: Add Diagnostics First

Before changing the deletion logic, add logging to understand what is actually happening:

```cpp
bool FProjectAssetOperations::DeleteAsset(const FString& AssetPath, FString& OutError)
{
    // Check pre-state
    FString ObjectPath = AssetPath + TEXT(".") + FPaths::GetBaseFilename(AssetPath);
    UObject* PreCheck = StaticFindObject(UObject::StaticClass(), nullptr, *ObjectPath);
    UE_LOG(LogTemp, Warning, TEXT("MCP DeleteAsset PRE: Object in memory = %s"),
        PreCheck ? TEXT("YES") : TEXT("NO"));

    if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        OutError = FString::Printf(TEXT("Asset does not exist: %s"), *AssetPath);
        return false;
    }

    bool bResult = UEditorAssetLibrary::DeleteAsset(AssetPath);
    UE_LOG(LogTemp, Warning, TEXT("MCP DeleteAsset: UEditorAssetLibrary::DeleteAsset returned %s"),
        bResult ? TEXT("true") : TEXT("false"));

    // Check post-state
    UObject* PostCheck = StaticFindObject(UObject::StaticClass(), nullptr, *ObjectPath);
    UPackage* PostPkg = FindObject<UPackage>(nullptr, *AssetPath);
    UE_LOG(LogTemp, Warning, TEXT("MCP DeleteAsset POST: Object in memory = %s, Package in memory = %s"),
        PostCheck ? TEXT("YES") : TEXT("NO"),
        PostPkg ? TEXT("YES") : TEXT("NO"));

    if (PostCheck)
    {
        UE_LOG(LogTemp, Warning, TEXT("MCP DeleteAsset POST: Object flags = %d, RF_Standalone=%s, RF_Public=%s"),
            PostCheck->GetFlags(),
            PostCheck->HasAnyFlags(RF_Standalone) ? TEXT("yes") : TEXT("no"),
            PostCheck->HasAnyFlags(RF_Public) ? TEXT("yes") : TEXT("no"));
    }

    return bResult;
}
```

### Step 2: Fix Based on Diagnostics

**If DeleteAsset returns false:** The `OnAssetsCanDelete` delegate or `bClosedAllEditors` check is vetoing. Need to investigate what's blocking.

**If DeleteAsset returns true BUT object still in memory:** The object survived GC. Most likely cause:
- Transaction buffer holding a reference
- Rendering thread still has a reference
- `CleanupAfterSuccessfulDelete` skipped the package unload due to reference check

**Fix for "returns true but object survives"** -- use `ObjectTools::ForceDeleteObjects` directly with additional cleanup:

```cpp
bool FProjectAssetOperations::DeleteAsset(const FString& AssetPath, FString& OutError)
{
    // 1. Load the asset
    FString ObjectPath = AssetPath;
    if (!ObjectPath.Contains(TEXT(".")))
    {
        ObjectPath = AssetPath + TEXT(".") + FPaths::GetBaseFilename(AssetPath);
    }

    UObject* Asset = StaticFindObject(UObject::StaticClass(), nullptr, *ObjectPath);
    if (!Asset)
    {
        Asset = LoadObject<UObject>(nullptr, *ObjectPath);
    }
    if (!Asset)
    {
        OutError = FString::Printf(TEXT("Asset not found: %s"), *AssetPath);
        return false;
    }

    UPackage* Package = Asset->GetOutermost();

    // 2. Close editors
    if (GEditor)
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(Asset);
        GEditor->GetSelectedObjects()->Deselect(Asset);
    }

    // 3. Clear transaction buffer first (common blocker)
    if (GEditor && GEditor->Trans)
    {
        GEditor->ResetTransaction(NSLOCTEXT("MCP", "DeleteAsset", "MCP Delete Asset"));
    }

    // 4. Force delete
    TArray<UObject*> ObjectsToDelete;
    ObjectsToDelete.Add(Asset);
    int32 NumDeleted = ObjectTools::ForceDeleteObjects(ObjectsToDelete, /*bShowConfirmation=*/false);

    if (NumDeleted == 0)
    {
        OutError = FString::Printf(TEXT("ForceDeleteObjects returned 0 for: %s"), *AssetPath);
        return false;
    }

    // 5. Verify cleanup -- force additional GC if object survived
    UObject* SurvivedCheck = StaticFindObject(UObject::StaticClass(), nullptr, *ObjectPath);
    if (SurvivedCheck)
    {
        UE_LOG(LogTemp, Warning, TEXT("MCP DeleteAsset: Object survived ForceDeleteObjects, forcing manual cleanup"));

        SurvivedCheck->ClearFlags(RF_Standalone | RF_Public);
        FAssetRegistryModule::AssetDeleted(SurvivedCheck);

        if (UPackage* SurvivedPkg = FindObject<UPackage>(nullptr, *AssetPath))
        {
            FAssetRegistryModule::PackageDeleted(SurvivedPkg);
            TArray<UPackage*> PkgsToUnload;
            PkgsToUnload.Add(SurvivedPkg);
            UPackageTools::UnloadPackages(PkgsToUnload);
        }

        CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    }

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully deleted asset: %s"), *AssetPath);
    return true;
}
```

### Step 3: Also Fix Import Side (Defense in Depth)

Add stale object cleanup in `ImportStaticMeshCommand.cpp` before creating the package:

```cpp
// Before CreatePackage -- purge any stale in-memory objects at this path
FString FullObjectPath = PackagePath + TEXT(".") + AssetName;
UObject* StaleObject = StaticFindObject(UObject::StaticClass(), nullptr, *FullObjectPath);
if (StaleObject)
{
    UE_LOG(LogTemp, Warning, TEXT("MCP Import: Found stale in-memory object at %s, purging before import"), *FullObjectPath);

    UPackage* StalePackage = StaleObject->GetOutermost();
    StaleObject->ClearFlags(RF_Standalone | RF_Public);
    FAssetRegistryModule::AssetDeleted(StaleObject);

    if (StalePackage)
    {
        FAssetRegistryModule::PackageDeleted(StalePackage);
        TArray<UPackage*> PkgsToUnload;
        PkgsToUnload.Add(StalePackage);
        UPackageTools::UnloadPackages(PkgsToUnload);
    }

    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
}

// Now safe to create package and import
UPackage* Package = CreatePackage(*PackagePath);
```

### Step 4: Consider Ticker Dispatch

If the above fixes still have issues, `delete_asset` may need to be dispatched via `FTSTicker` instead of `AsyncTask(GameThread)`, similar to how `import_static_mesh` and `import_lod` are handled. `ForceDeleteObjects` calls `FlushAsyncLoading()`, `CollectGarbage()`, and `FlushRenderingCommands()` which may cause issues inside TaskGraph context.

Add `delete_asset` to the `TickerCommands` array in `UnrealMCPBridge.cpp`:

```cpp
static const TArray<FString> TickerCommands = {
    TEXT("import_static_mesh"),
    TEXT("import_lod"),
    TEXT("delete_asset"),  // ForceDeleteObjects calls FlushAsyncLoading + CollectGarbage
};
```

## ForceDelete vs Normal Delete

| Aspect | `DeleteObjects` (Normal) | `ForceDeleteObjects` (Force) |
|--------|-------------------------|------------------------------|
| Reference check | Yes, scans all refs first | No, replaces all refs with nullptr |
| User confirmation | Optional dialog | Optional dialog |
| In-memory refs | Aborts if still referenced | Nulls them out forcefully |
| On-disk refs | Shows dialog listing referencers | Does not check on-disk refs |
| Blueprint instances | N/A (aborts if referenced) | Destroys all instances |
| Reference replacement | N/A | `ForceReplaceReferences(nullptr, ...)` |
| Use case | User-initiated "safe" delete | Programmatic/automated delete |

**For MCP: ForceDeleteObjects is correct.** We need silent, non-interactive deletion.

## Handling References from Other Assets (PCG Spawner)

When a PCG spawner references the mesh being deleted:

1. `ForceDeleteObjects` calls `RecursiveRetrieveReferencers` which finds the PCG component
2. Closes any editors open for referencing assets
3. `ForceReplaceReferences(nullptr, ObjectsToReplace)` replaces the mesh reference in PCG spawner with nullptr
4. PCG spawner Blueprint is dirtied (unsaved change)
5. After deletion, the PCG spawner's mesh reference is nullptr

After reimport at the same path:
- **Hard references** (`UStaticMesh*` properties): Will be nullptr. Need explicit re-assignment.
- **Soft references** (`FSoftObjectPath`): Will still point to the old path string. When resolved after reimport, they will find the new asset automatically since it has the same path.
- **PCG StaticMeshSpawner**: Uses `FSoftObjectPath` internally. After reimport to same path, the PCG component should pick up the new mesh on next evaluation/refresh.

## Key UE Source Files

| File | Key Content |
|------|-------------|
| `Engine/Source/Editor/UnrealEd/Private/ObjectTools.cpp` | `ForceDeleteObjects` (3553), `DeleteSingleObject` (3387), `CleanupAfterSuccessfulDelete` (2498), `DeleteObjects` (3064), `ForceReplaceReferences` (1129), `GatherObjectReferencersForDeletion` (305) |
| `Engine/Source/Editor/UnrealEd/Public/ObjectTools.h` | All API declarations |
| `Engine/Source/Editor/UnrealEd/Private/Subsystems/EditorAssetSubsystem.cpp` | `DeleteAsset` (795), `DeleteLoadedAssets` (772) |
| `Engine/Plugins/Editor/EditorScriptingUtilities/.../EditorAssetLibrary.cpp` | Thin wrapper (339) |
| `Engine/Source/Editor/UnrealEd/Private/PackageTools.cpp` | `UnloadPackages` (361) |
| `Engine/Source/Editor/UnrealEd/Private/Fbx/FbxFactory.cpp` | `StaticFindObject` reimport detection (216) |
| `Engine/Source/Editor/UnrealEd/Private/AssetDeleteModel.cpp` | `FAssetDeleteModel` reference scanning |
| `Engine/Source/Runtime/CoreUObject/Private/UObject/UObjectGlobals.cpp` | `CreatePackage` with `FindObject<UPackage>` (1040) |

## Implementation Notes

- `ObjectTools.h` and `PackageTools.h` are in `UnrealEd` module (already a dependency)
- `CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS)` MUST run on game thread
- `ForceReplaceReferences` creates `FGlobalComponentRecreateRenderStateContext` (expensive, detaches/reattaches all components globally)
- After `ForceDeleteObjects`, raw pointers to deleted object are dangling
- Transaction buffer (`GEditor->Trans`) is the most common blocker for "object won't die"
- `GIsRunningUnattendedScript` suppresses dialogs but does NOT prevent `ForceDeleteObjects` from returning 0 on failure
