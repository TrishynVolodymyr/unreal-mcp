# Research: Programmatic Access to UE 5.7 Stat Counters

## Summary

UE stat counters (STAT_ProcessedPrimitives, etc.) are **fire-and-forget messages** sent via `FThreadStats::AddMessage` -- there is no simple "read counter value" API. However, there are multiple **direct global variables and scene members** that provide the same data without going through the stat system at all. The best approach is to read these globals and FScene members directly.

## Approach 1: Direct Stat System Access -- NOT VIABLE

The UE stat system works via message passing:
- `INC_DWORD_STAT_BY(STAT_X, Value)` expands to `FThreadStats::AddMessage(GET_STATFNAME(STAT_X), EStatOperation::Add, Value)`
- These messages are accumulated per-frame on the **stats thread**
- `FLatestGameThreadStatsData::Get().Latest` holds the last data sent to game thread, but requires stat groups to be **enabled** (same as HUD display)
- `FStatsThreadState::GetLocalState()` only available on the stats thread

**Verdict: Skip the stat system entirely. Read the source data directly.**

## Approach 2: Global RHI Counters -- BEST for Draw Calls/Primitives

Already in use. Full list from `RHIStats.h`:

```cpp
#include "RHIStats.h"

// Already used:
extern RHI_API int32 GNumDrawCallsRHI[MAX_NUM_GPUS];      // Total draw calls per GPU
extern RHI_API int32 GNumPrimitivesDrawnRHI[MAX_NUM_GPUS]; // Total primitives per GPU

// Category-level draw stats (HAS_GPU_STATS required, true in non-shipping):
FRHIDrawStatsCategory::FManager& Manager = FRHIDrawStatsCategory::GetManager();
// Manager.DisplayCounts[CategoryIndex][GPUIndex] -- draw calls per category
// Manager.NumCategory -- number of registered categories
// Manager.Array[i]->Name -- FName of each category
```

### Additional RHI externs available:

```cpp
#include "RHIGlobals.h"
extern RHI_API bool GRHIDeviceIsIntegrated;
// GPU frame cycles already used:
uint32 GPUCycles = RHIGetGPUFrameCycles(0);
```

### FRHIDrawStats Detail (from ProcessAsFrameStats):
The `GRHICommandList.FrameDrawStats` contains per-category breakdowns:
- `Draws`, `Triangles`, `Lines`, `Quads`, `Points`, `Rectangles` per category per GPU
- But `GRHICommandList` is render-thread only. The globals `GNumDrawCallsRHI`/`GNumPrimitivesDrawnRHI` are the game-thread-safe versions.

### RHI Stat Counters (require HAS_GPU_STATS):
```cpp
// Declared in RHIStats.h, STATGROUP_RHI:
STAT_RHIDrawPrimitiveCalls  // same as GNumDrawCallsRHI[0] essentially
STAT_RHITriangles           // triangles only (not all primitives)
STAT_RHILines               // lines drawn
```

### VRAM Memory:
```cpp
#include "RHIStats.h"
#include "DynamicRHI.h"

FTextureMemoryStats TexMemStats;
RHIGetTextureMemoryStats(TexMemStats);
// TexMemStats.DedicatedVideoMemory
// TexMemStats.TotalGraphicsMemory
// TexMemStats.StreamingMemorySize
// TexMemStats.NonStreamingMemorySize
```

### D3D Memory (Windows only, PLATFORM_MICROSOFT):
```cpp
#if PLATFORM_MICROSOFT
// FD3DMemoryStats has BudgetLocal, UsedLocal, AvailableLocal etc.
// But there's no public getter -- it's populated internally by D3D12 adapter
// and pushed via UpdateD3DMemoryStatsAndCSV(). Not directly readable.
// Use stat system or RHIGetTextureMemoryStats instead.
#endif
```

## Approach 3: FScene Members -- Primitive/Light Counts

From `ScenePrivate.h` (Renderer module, private header):

```cpp
// FScene members (game-thread safe):
int32 NumVisibleLights_GameThread;     // Number of visible lights
int32 NumEnabledSkylights_GameThread;  // Number of active skylights

// Render-thread only:
TArray<FPrimitiveSceneProxy*> PrimitiveSceneProxies;  // .Num() = total primitives in scene
```

### Access pattern from editor subsystem:
```cpp
#include "Engine/World.h"

UWorld* World = GEditor->GetEditorWorldContext().World();
FSceneInterface* SceneInterface = World->Scene;

// FScene is in Renderer PRIVATE headers -- you cannot #include "ScenePrivate.h"
// from a plugin without adding Renderer to PrivateDependencyModuleNames.
// This is intentionally restricted.

// ALTERNATIVE: Count primitives via UWorld iteration
int32 PrimitiveCount = 0;
for (TActorIterator<AActor> It(World); It; ++It)
{
    TInlineComponentArray<UPrimitiveComponent*> Prims;
    (*It)->GetComponents<UPrimitiveComponent>(Prims);
    PrimitiveCount += Prims.Num();
}
// This counts components, not scene proxies, so it's approximate.
```

### FScene::HasVisibleLights pattern:
```cpp
// FSceneInterface does NOT expose NumVisibleLights publicly.
// FScene stores NumVisibleLights_GameThread but FScene is private to Renderer.
// To get light count from editor:
int32 LightCount = 0;
for (TActorIterator<ALight> It(World); It; ++It)
{
    if ((*It)->GetLightComponent() && (*It)->GetLightComponent()->IsVisible())
    {
        LightCount++;
    }
}
```

## Approach 4: Renderer Stat Counters -- What They Track

All declared in `RenderCore.h` (public header, RENDERCORE_API):

### STATGROUP_InitViews (visibility/culling):
| Stat Name | What It Counts |
|-----------|---------------|
| `STAT_ProcessedPrimitives` | Total primitives tested for visibility (per view) |
| `STAT_CulledPrimitives` | Primitives rejected by frustum culling |
| `STAT_OccludedPrimitives` | Primitives rejected by occlusion culling |
| `STAT_StaticallyOccludedPrimitives` | Primitives rejected by precomputed occlusion |
| `STAT_VisibleStaticMeshElements` | Visible static mesh elements after culling |
| `STAT_VisibleDynamicPrimitives` | Visible dynamic primitives after culling |
| `STAT_OcclusionQueries` | Number of occlusion queries issued |
| `STAT_CSMSubjects` | Cascade shadow map subject primitives |

### STATGROUP_SceneRendering:
| Stat Name | What It Counts |
|-----------|---------------|
| `STAT_MeshDrawCalls` | Mesh draw calls (from MeshPassProcessor) |
| `STAT_SceneLights` | Lights in scene (accumulator) |
| `STAT_SceneDecals` | Decals in scene (accumulator) |
| `STAT_Decals` | Decals in view (counter) |
| `STAT_LightShaftsLights` | Lights using light shafts |

### STATGROUP_Engine (from EngineStats.h):
| Stat Name | What It Counts |
|-----------|---------------|
| `STAT_StaticMeshTriangles` | Static mesh triangles rendered |
| `STAT_SkelMeshTriangles` | Skeletal mesh triangles |
| `STAT_SkelMeshDrawCalls` | Skeletal mesh draw calls |

**The problem: These are all counter stats (reset each frame). Their values are only accumulated via `INC_DWORD_STAT_BY` on the render thread and consumed by the stat system. There is no "current value" variable to read -- the value only exists transiently during the render frame.**

### Reading Stat Values Anyway:
The only way to read these without HUD display is:
1. Enable the stat group programmatically: `FThreadStats::EnableStatGroup(TEXT("STATGROUP_InitViews"))`
2. Read from `FLatestGameThreadStatsData::Get().Latest`
3. Parse the `FComplexStatMessage` entries

This is fragile, requires the stat thread to be active, and the data is one frame behind. **Not recommended for a reliable tool.**

## Approach 5: Nanite Stats -- GPU Readback Only

Nanite stats (`FNaniteStats`) are GPU-side structured buffers:
- Populated by compute shaders during cull/raster passes
- Stored in `Nanite::GGlobalResources.GetStatsBufferRef()` (GPU buffer)
- Only read back when `GNaniteShowStats != 0` (console variable `r.Nanite.ShowStats`)
- The readback is GPU async, displayed via `Nanite::PrintStats` -- not available as simple counters

**Verdict: Nanite stats require GPU readback. Not practical for a synchronous MCP tool. Could enable `r.Nanite.ShowStats` and parse console output, but that's hacky.**

## Approach 6: Console Command Output Capture

UE console commands can be captured:

```cpp
FString Output;
FStringOutputDevice OutputDevice;
GEngine->Exec(World, TEXT("stat InitViews"), OutputDevice);
Output = OutputDevice;
// BUT: "stat InitViews" toggles HUD display, it does NOT dump values to output.
// stat commands use FStatGroupEnableManager to toggle groups on/off.
```

### Better console commands:
```cpp
// "stat dumpframe" writes a single frame's stats to log -- but async and unreliable timing
// "stat dumpave" dumps averaged stats -- same issue
// "stat dumpcpu" / "stat dumpgpu" -- profiling dumps, not real-time values
```

**Verdict: Console command capture does not provide structured stat counter values.**

## Recommended Implementation

For the MCP `get_performance_stats` command, the most reliable approach combines:

### Tier 1 -- Already Working (keep as-is):
```cpp
GNumDrawCallsRHI[0]           // Draw calls
GNumPrimitivesDrawnRHI[0]     // Primitives drawn (tris + lines + etc)
GAverageFPS / GAverageMS       // Frame timing
RHIGetGPUFrameCycles(0)        // GPU time
FPlatformMemory::GetStats()    // System RAM
```

### Tier 2 -- Easy to Add:
```cpp
// Texture/VRAM memory
FTextureMemoryStats TexMemStats;
RHIGetTextureMemoryStats(TexMemStats);
// -> DedicatedVideoMemory, TotalGraphicsMemory, StreamingMemorySize

// RHI triangle count (if HAS_GPU_STATS)
#if HAS_GPU_STATS
STAT_RHITriangles  // via stat system, or just use GNumPrimitivesDrawnRHI which is the total
#endif

// Draw call categories
#if HAS_GPU_STATS
FRHIDrawStatsCategory::FManager& Manager = FRHIDrawStatsCategory::GetManager();
for (int32 i = 0; i < Manager.NumCategory; ++i)
{
    FName CategoryName = Manager.Array[i]->Name;
    int32 DrawCount = Manager.DisplayCounts[i][0]; // GPU 0
}
#endif
```

### Tier 3 -- Scene Counts (via world iteration, approximate):
```cpp
// Count primitives by iterating actors
UWorld* World = GEditor->GetEditorWorldContext().World();
int32 TotalPrimitives = 0;
int32 TotalLights = 0;
int32 TotalStaticMeshes = 0;
for (TActorIterator<AActor> It(World); It; ++It)
{
    TInlineComponentArray<UPrimitiveComponent*> Prims;
    (*It)->GetComponents(Prims);
    TotalPrimitives += Prims.Num();

    TInlineComponentArray<ULightComponent*> Lights;
    (*It)->GetComponents(Lights);
    TotalLights += Lights.Num();

    TInlineComponentArray<UStaticMeshComponent*> SMComps;
    (*It)->GetComponents(SMComps);
    TotalStaticMeshes += SMComps.Num();
}
```

### NOT Recommended:
- Direct FScene access (private Renderer headers, can't use from plugin)
- Stat system message reading (fragile, requires enabled groups)
- Nanite GPU stats readback (async, complex)
- Console command output parsing (commands toggle display, don't output values)

## Key Files Referenced

- `E:\code\unreal-mcp\ues\UnrealEngine-5.7\UnrealEngine-5.7\Engine\Source\Runtime\RHI\Public\RHIStats.h` -- GNumDrawCallsRHI, FRHIDrawStats, FRHIDrawStatsCategory, FTextureMemoryStats, FD3DMemoryStats
- `E:\code\unreal-mcp\ues\UnrealEngine-5.7\UnrealEngine-5.7\Engine\Source\Runtime\RHI\Public\RHICommandList.h` -- GRHICommandList.FrameDrawStats
- `E:\code\unreal-mcp\ues\UnrealEngine-5.7\UnrealEngine-5.7\Engine\Source\Runtime\RHI\Private\RHI.cpp:1264` -- ProcessAsFrameStats implementation (how GNumDrawCallsRHI gets populated)
- `E:\code\unreal-mcp\ues\UnrealEngine-5.7\UnrealEngine-5.7\Engine\Source\Runtime\RenderCore\Public\RenderCore.h` -- All STAT_* declarations for InitViews and SceneRendering groups
- `E:\code\unreal-mcp\ues\UnrealEngine-5.7\UnrealEngine-5.7\Engine\Source\Runtime\Engine\Public\EngineStats.h` -- STAT_StaticMeshTriangles etc.
- `E:\code\unreal-mcp\ues\UnrealEngine-5.7\UnrealEngine-5.7\Engine\Source\Runtime\Renderer\Private\ScenePrivate.h:2874` -- FScene class with NumVisibleLights_GameThread, PrimitiveSceneProxies
- `E:\code\unreal-mcp\ues\UnrealEngine-5.7\UnrealEngine-5.7\Engine\Source\Runtime\Renderer\Private\SceneVisibility.cpp:4869-4871` -- Where ProcessedPrimitives/CulledPrimitives/OccludedPrimitives are incremented
- `E:\code\unreal-mcp\ues\UnrealEngine-5.7\UnrealEngine-5.7\Engine\Source\Runtime\Core\Public\Stats\Stats.h` -- GET_STATFNAME, INC_DWORD_STAT_BY macros
- `E:\code\unreal-mcp\ues\UnrealEngine-5.7\UnrealEngine-5.7\Engine\Source\Runtime\Core\Public\Stats\StatsData.h:903` -- FLatestGameThreadStatsData
- `E:\code\unreal-mcp\MCPGameProject\Plugins\UnrealMCP\Source\UnrealMCP\Private\Commands\Editor\GetPerformanceStatsCommand.cpp` -- Current implementation
