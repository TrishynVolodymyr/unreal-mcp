# Research: UE 5.7 Profiling & Debugging Tools for MCP Integration

## Summary

Comprehensive catalog of all profiling and debugging tools available in Unreal Engine 5.7, analyzed for MCP automation potential. UE provides ~40+ stat groups, Unreal Insights tracing, CSV profiler, GPU profiler (new in 5.7), DumpGPU, memory reports, object introspection, and RenderDoc/PIX integration. Most are activatable via console commands (`GEngine->Exec`), making them automatable through our existing C++ command infrastructure.

---

## 1. Unreal Insights (utrace)

### Console Commands (from `TraceAuxiliary.cpp`)

| Command | Usage | Notes |
|---------|-------|-------|
| `Trace.File [Path] [ChannelSet]` | Start tracing to `.utrace` file | Primary command (replaces deprecated `Trace.Start`) |
| `Trace.Send <Host> [ChannelSet]` | Stream to Unreal Insights app | Host = IP of trace store |
| `Trace.Stop` | Stop active trace | |
| `Trace.Status` | Print current trace state | |
| `Trace.Enable <Channels>` | Enable specific channels | |
| `Trace.Disable <Channels>` | Disable specific channels | |
| `Trace.SnapshotFile [Path]` | Write snapshot of current trace | |
| `Trace.SnapshotSend <Host> <Port>` | Send snapshot to remote | |
| `Trace.Bookmark` | Insert bookmark in trace | |

### Channel Presets (from source)

- **Default**: `cpu,gpu,frame,log,bookmark,screenshot,region`
- **Memory**: `memtag,memalloc,callstack,module` (tail mode)
- **Memory_Light**: `memtag,memalloc` (tail mode)

### What It Shows

- Per-frame CPU timings (game thread, render thread, RHI thread)
- GPU pass timings (via `gpu` channel)
- Memory allocations and tags
- Bookmarks and regions
- Screenshots at capture points
- Log messages

### MCP Automation

**Highly automatable.** All commands are console commands executable via `GEngine->Exec()`. An MCP tool could:
1. `Trace.File /path/to/capture.utrace cpu,gpu,frame` -- start capture
2. Wait N frames or seconds
3. `Trace.Stop` -- stop capture
4. Return path to `.utrace` file for analysis
5. Parse the file programmatically (complex but possible)

### Usefulness: HIGH for timeline analysis, MEDIUM for automated AI use (binary format)

---

## 2. Stat Commands -- Complete Catalog

### How Stats Work in UE

Stats are declared via `DECLARE_STATS_GROUP` (group name for `stat X` command) and individual stats via `DECLARE_CYCLE_STAT`, `DECLARE_DWORD_COUNTER_STAT`, `DECLARE_MEMORY_STAT`, etc.

All stat groups can be toggled via `stat <GroupName>` console command. Data is accessible programmatically via `FLatestGameThreadStatsData::Get().Latest` (as our existing `GetGPUStatsCommand.cpp` already does).

### Global Stat Groups (from `GlobalStats.inl`)

| Console Command | STATGROUP | What It Shows |
|-----------------|-----------|---------------|
| `stat AI` | STATGROUP_AI | AI tick, pathfinding, behavior tree costs |
| `stat Anim` | STATGROUP_Anim | AnimInstance init, montage advance, blend space eval, bone pose extraction, curve eval |
| `stat Audio` | STATGROUP_Audio | Audio processing, source management |
| `stat Canvas` | STATGROUP_Canvas | Canvas/HUD draw calls |
| `stat Character` | STATGROUP_Character | Character movement |
| `stat Collision` | STATGROUP_Collision | Collision detection costs |
| `stat Engine` | STATGROUP_Engine | **KEY**: FrameTime, GameEngineTick, GameViewportTick, RedrawViewports, UpdateLevelStreaming, RHITickTime, StaticMeshTriangles, SkelMeshTriangles, SkelMeshDrawCalls, SkinningTime, InputLatency |
| `stat Game` | STATGROUP_Game | **KEY**: PhysicsTime, SpawnActorTime, ActorBeginPlay, MoveComponentTime, UpdateOverlaps, TickTime, WorldTickTime, UpdateCameraTime, CharacterMovement, PlayerControllerTick, PostTickComponentUpdate, GCMarkTime, GCSweepTime |
| `stat InitViews` | STATGROUP_InitViews | **CRITICAL FOR CULLING**: ViewVisibility, OcclusionCull, ViewRelevance, StaticRelevance, UpdateStaticMeshes, GetDynamicMeshElements, SetupMeshPass, InitDynamicShadows, GatherShadowPrimitives, BuildCSMVisibilityState, ProcessedPrimitives (count), CulledPrimitives (count) |
| `stat LightRendering` | STATGROUP_LightRendering | **CRITICAL FOR LIGHTS**: RenderLightsAndShadows, DirectLightRenderingTime, TranslucentInjection, NumLightsUsingStandardDeferred, NumShadowedLights, NumBatchedLights, NumLightFunctionOnlyLights |
| `stat SceneRendering` | STATGROUP_SceneRendering | **CRITICAL FOR GPU PASSES**: Render Init, AllocGBufferTargets, Lighting, RenderAtmosphere, RenderSkyAtmosphere, RenderFog, RenderLocalFogVolume, LightShaftOcclusion, LightShaftBloom, RenderFinish, FXSystem PreRender/PostRenderOpaque, DBuffer, ResolveDepth, ViewExtensions |
| `stat ShadowRendering` | STATGROUP_ShadowRendering | **CRITICAL FOR SHADOWS**: ActiveLightCount, VSM metrics (Nanite views, single page count, full count, raster bins, shading bins, directional/local projections), DistantLightCount, DistantCachedCount |
| `stat Nanite` | STATGROUP_Nanite | **CRITICAL FOR NANITE**: CullingContexts, BasePass Total/Visible Raster Bins, BasePass Total/Visible Shading Bins, CustomDepthInstances |
| `stat NaniteStreaming` | STATGROUP_NaniteStreaming | NaniteResources count, HierarchyNodes, RootPages, StreamingPoolPages, pool sizes (MB), page requests (GPU/Explicit/Prefetch), quality scale |
| `stat Landscape` | STATGROUP_Landscape | Landscape rendering |
| `stat Memory` | STATGROUP_Memory | Memory allocations |
| `stat MemoryStaticMesh` | STATGROUP_MemoryStaticMesh | Static mesh memory |
| `stat Navigation` | STATGROUP_Navigation | NavMesh costs |
| `stat Net` | STATGROUP_Net | Network stats |
| `stat Object` | STATGROUP_Object | Object creation/destruction |
| `stat Particles` | STATGROUP_Particles | Particle emitter init, component init, dynamic data creation, send render data |
| `stat Physics` | STATGROUP_Physics | Physics simulation costs |
| `stat RHI` | STATGROUP_RHI | **CRITICAL FOR GPU MEMORY**: DrawPrimitiveCalls, Triangles drawn, Lines drawn, RenderTarget2D/3D/Cube memory, Texture2D/3D/Cube memory, UniformBufferMemory, IndexBufferMemory, VertexBufferMemory, StructuredBufferMemory, SamplerDescriptorsAllocated, ResourceDescriptorsAllocated |
| `stat RHICMDLIST` | STATGROUP_RHICMDLIST | RHI command list execution, memory, parallel translate |
| `stat RDG` | STATGROUP_RDG | Render Dependency Graph stats |
| `stat RenderThread` | STATGROUP_RenderThreadProcessing | Render thread processing |
| `stat RenderTargetPool` | STATGROUP_RenderTargetPool | Render target pool usage |
| `stat SceneMemory` | STATGROUP_SceneMemory | Scene memory usage |
| `stat SceneUpdate` | STATGROUP_SceneUpdate | Scene update costs |
| `stat Streaming` | STATGROUP_Streaming | Asset streaming |
| `stat StreamingOverview` | STATGROUP_StreamingOverview | Streaming overview |
| `stat StreamingDetails` | STATGROUP_StreamingDetails | Streaming details |
| `stat Threading` | STATGROUP_Threading | Thread stats |
| `stat Threads` | STATGROUP_Threads | Thread info |
| `stat Tickables` | STATGROUP_Tickables | Tickable object costs |
| `stat UI` | STATGROUP_UI | UMG/Slate costs |
| `stat UObjects` | STATGROUP_UObjects | UObject count tracking |
| `stat ShaderCompiling` | STATGROUP_ShaderCompiling | Shader compilation |
| `stat Niagara` | STATGROUP_Niagara | Niagara VFX: scene proxy creation, skeleton sampling, UV mapping, async compile |

### Non-Global Stat Groups (declared in specific modules)

| Console Command | Where Declared | What It Shows |
|-----------------|----------------|---------------|
| `stat NaniteRayTracing` | `NaniteRayTracing.cpp` | In-flight updates, stream out requests, scheduled builds, auxiliary data buffer memory |
| `stat NiagaraDataChannels` | `NiagaraDataChannel.h` | Niagara data channel overhead |
| `stat AudioMixer` | `AudioMixer.h` | Audio mixer processing |
| `stat Crowd` | `CrowdManager.cpp` | AI crowd management |

### MCP Automation

**Highly automatable.** Stats can be:
1. Enabled via `GEngine->Exec(World, TEXT("stat InitViews"))`
2. Read via `FLatestGameThreadStatsData::Get().Latest` (see existing `GetGPUStatsCommand.cpp`)
3. Each stat group provides `FActiveStatGroupInfo` with `CountersAggregate`, `GpuStatsAggregate`, and named cycle counters

### Usefulness: VERY HIGH -- this is the primary tool for automated profiling

---

## 3. ProfileGPU -- One-Shot GPU Capture

### Console Command

```
ProfileGPU
```

Keyboard shortcut: `Ctrl+Shift+,`

### How It Works (from `UnrealEngine.cpp`)

In UE 5.7 with `RHI_NEW_GPU_PROFILER`, the old `ProfileGPU` path is compiled out (guarded by `#if WITH_PROFILEGPU && (RHI_NEW_GPU_PROFILER == 0)`). The new GPU profiler feeds data through the stat system instead.

For UE 5.7, the equivalent is `stat gpu` which uses `UE::RHI::GPUProfiler` -- the new breadcrumb-based profiler that reports:
- Per-breadcrumb GPU busy time (TOP/BOP timestamps)
- Per-breadcrumb draw/dispatch/primitive/vertex counts
- Queue types: Graphics, Compute, Copy, SwapChain
- Frame boundaries with CPU/GPU correlation

### What It Shows

Hierarchical breakdown of GPU work:
- Each render pass (BasePass, Lighting, Shadows, PostProcess, etc.)
- Time in milliseconds per pass
- Nested passes (e.g., Shadows > DirectionalLight > CascadedShadowMap)

### MCP Automation

**Already partially implemented** in `GetGPUStatsCommand.cpp`. Our existing command:
1. Enables `stat gpu` on first call
2. Reads `FLatestGameThreadStatsData::Get().Latest`
3. Extracts per-pass BusyAvg/BusyMax/BusyMin/WaitAvg/IdleAvg

### Usefulness: VERY HIGH -- already integrated, shows per-pass GPU breakdown

---

## 4. CSV Profiler

### Console Commands

The CSV profiler uses the `FCsvProfiler` API. Console commands:

```
csv.Start          -- Not a direct console command; uses FCsvProfiler::BeginCapture()
csv.Stop           -- Not a direct console command; uses FCsvProfiler::EndCapture()
```

**Programmatic API** (from `CsvProfiler.h`):
```cpp
FCsvProfiler::Get()->BeginCapture(
    NumFramesToCapture,    // -1 for unlimited
    DestinationFolder,     // output folder
    Filename,              // output filename
    ECsvProfilerFlags::None
);

TSharedFuture<FString> Result = FCsvProfiler::Get()->EndCapture();
// Result contains the path to the .csv file
```

### What It Captures

Per-frame CSV data with columns for:
- All `CSV_SCOPED_TIMING_STAT` instrumented sections
- Custom stats via `CSV_CUSTOM_STAT`
- Exclusive timing (non-overlapping)
- Wait tracking
- Events and bookmarks
- Categories: CsvProfiler-defined categories (render, game, physics, etc.)

### Console Variables

| CVar | Purpose |
|------|---------|
| `csv.NamedEventsExclusive` | Emit named events for exclusive stats |
| `csv.NamedEventsTiming` | Emit named events for non-exclusive timing stats |

### MCP Automation

**Highly automatable.** Can:
1. Call `FCsvProfiler::Get()->BeginCapture(100)` to capture 100 frames
2. Wait for completion via the returned `TSharedFuture`
3. Read the resulting `.csv` file
4. Parse CSV data to identify hotspots

Output path: `<Project>/Saved/Profiling/CSV/`

### Usefulness: HIGH -- structured CSV is easy for AI to parse

---

## 5. stat startfile / stat stopfile

### Console Commands

| Command | Purpose |
|---------|---------|
| `stat startfile [Filename]` | Start dumping stats to `.ue4stats` file |
| `stat startfileraw` | Start dumping raw stats (enables raw stat tracking first) |
| `stat stopfile` | Stop dumping and finalize file |

### What It Captures

Full stat system dump -- all cycle counters, dword counters, memory stats across all active stat groups. Binary format viewable in Unreal Insights or the legacy UE4 profiler.

Output path: `<Project>/Saved/Profiling/`

### MCP Automation

**Automatable via console command.** However, output is binary (`.ue4stats`), harder for AI to parse than CSV.

### Usefulness: MEDIUM -- binary format limits AI parsing, but captures everything

---

## 6. DumpGPU

### Console Command

```
DumpGPU
```

### What It Shows (from `DumpGPU.h`)

Dumps the entire Render Dependency Graph (RDG) frame to disk:
- All render passes with their inputs/outputs
- Texture resources and states
- Buffer resources
- Service file for optional upload

Output path: `<Project>/Saved/Screenshots/` (or configurable)

### MCP Automation

**Automatable via console command.** Output is structured files on disk that could be parsed.

### Usefulness: HIGH for deep render pipeline debugging, but very detailed/verbose

---

## 7. FreezeRendering

### Console Command

```
FreezeRendering
```

Also accessible via Debug Camera (F key when in debug camera mode).

### What It Does

Freezes the renderer's view of the scene -- culling, LOD selection, occlusion queries all stop updating. The camera can then be moved freely to inspect what the renderer "sees" without it updating. Toggle command (call again to unfreeze).

### MCP Automation

**Automatable via console command.** Useful for debugging culling issues.

### Usefulness: MEDIUM -- visual inspection tool, less useful for automated analysis

---

## 8. ToggleAllScreenMessages

### Console Command

```
ToggleAllScreenMessages
```

### What It Does

Shows/hides all on-screen debug messages (stat displays, log messages, etc.).

### MCP Automation

Automatable but low value for AI profiling.

### Usefulness: LOW for automated profiling

---

## 9. obj list

### Console Command

```
obj list
obj list class=StaticMeshComponent
obj list class=PointLightComponent
```

### What It Shows

Lists all UObjects currently in memory, optionally filtered by class. Shows:
- Object count per class
- Memory usage per class
- Object names

### Implementation (from `UObjectGlobals.cpp`, `UObjectHash.cpp`)

The command iterates `FUObjectArray` and groups by class. This is handled in `CoreUObject` module.

### MCP Automation

**Automatable via console command.** Output goes to the log, which can be captured by redirecting to `FOutputDevice`.

### Usefulness: HIGH -- tells you exactly how many of each object type exist (e.g., "18000 InstancedStaticMeshComponent")

---

## 10. memreport

### Console Command

```
memreport
memreport -full
```

### What It Shows (from `UnrealEngine.cpp` lines 7907-8055)

Executes a configurable set of memory diagnostic commands and writes results to a `.memreport` file. The commands to run are configurable in `Engine.ini` under `[MemReport<Type>Commands]` sections.

Typical output includes:
- `obj list` output
- Memory allocator stats
- Platform memory stats
- Texture memory breakdown
- RHI resource memory

Output path: `<Project>/Saved/Profiling/MemReports/`

### MCP Automation

**Highly automatable.** Command generates a text file that can be read and parsed.

### Usefulness: HIGH for memory profiling, the file is text-based and parseable

---

## 11. GPU Visualizer (stat gpu)

### Console Command

```
stat gpu
```

In UE 5.7 with `RHI_NEW_GPU_PROFILER`, this is the primary GPU profiling mechanism. The old `ProfileGPU` UI visualizer is replaced.

### What It Shows

The new GPU profiler (from `GPUProfiler.h`) provides:
- **Per-breadcrumb timing**: TOP (Top of Pipe) and BOP (Bottom of Pipe) timestamps
- **Queue breakdown**: Graphics, Compute, Copy, SwapChain queues independently
- **Per-pass stats**: NumDraws, NumDispatches, NumPrimitives, NumVertices
- **Fence tracking**: Signal/Wait fence events for cross-queue synchronization
- **Frame time override**: For platforms without accurate GPU timing

### MCP Automation

**Already integrated.** `GetGPUStatsCommand.cpp` reads this data.

### Usefulness: VERY HIGH

---

## 12. RenderDoc / PIX Integration

### API (from `IRenderCaptureProvider.h`)

```cpp
// Check availability
if (IRenderCaptureProvider::IsAvailable())
{
    // Capture a frame
    IRenderCaptureProvider::Get().CaptureFrame(Viewport, Flags, FilePath);

    // Or use scoped capture
    IRenderCaptureProvider::Get().BeginCapture(&RHICmdList, ECaptureFlags_Launch, FileName);
    // ... rendering happens ...
    IRenderCaptureProvider::Get().EndCapture(&RHICmdList);
}
```

### Activation

- RenderDoc: Launch editor with `-RenderDoc` command line flag, or load the RenderDoc plugin
- PIX: Launch with `-AttachPIX` or use PIX's attach feature

### What It Shows

Full GPU frame capture: every draw call, resource state, shader, texture binding. The gold standard for GPU debugging.

### MCP Automation

**Partially automatable.** Can programmatically trigger a capture via `IRenderCaptureProvider`, but the resulting file requires RenderDoc/PIX to analyze. An MCP tool could:
1. Check `IRenderCaptureProvider::IsAvailable()`
2. Trigger capture to a known path
3. Return the path for manual inspection

### Usefulness: MEDIUM for AI (requires external tool to analyze), HIGH for developer workflow

---

## Render Debugging CVars

| CVar/Command | Purpose |
|--------------|---------|
| `r.RDG.Debug` | Enable RDG validation and debugging |
| `r.RDG.Dump` | Dump RDG graph |
| `stat RDG` | RDG performance stats |
| `ShowFlag.X 0/1` | Toggle individual rendering features |
| `viewmode <mode>` | Switch view mode (Lit, Unlit, Wireframe, ShaderComplexity, etc.) |
| `ShaderComplexity` | Show shader complexity visualization |
| `ProfileGPUHitches` | Detect and log GPU hitches |

---

## Recommended MCP Tools to Build

### Priority 1: Deep Stat Reader (expand existing `get_gpu_stats`)

**Command: `get_perf_stats`**

Accept a stat group name, enable it, read the data, return structured JSON.

```json
{
    "stat_group": "InitViews",
    "stats": {
        "ViewVisibility": {"avg_ms": 0.5, "max_ms": 1.2},
        "OcclusionCull": {"avg_ms": 0.3, "max_ms": 0.8},
        "ProcessedPrimitives": 18432,
        "CulledPrimitives": 15200
    }
}
```

Key stat groups to support:
- `Engine` -- frame-level overview
- `Game` -- game thread breakdown
- `SceneRendering` -- render pass breakdown
- `InitViews` -- culling costs
- `LightRendering` -- light costs
- `ShadowRendering` -- shadow/VSM costs
- `Nanite` -- Nanite raster/shading bins
- `NaniteStreaming` -- Nanite memory/streaming
- `RHI` -- GPU memory and draw calls
- `Anim` -- animation costs
- `Particles` -- particle system costs
- `Physics` -- physics costs

### Priority 2: CSV Profiler Capture

**Command: `capture_csv_profile`**

```cpp
FCsvProfiler::Get()->BeginCapture(FrameCount, OutputFolder, Filename);
// ... wait ...
TSharedFuture<FString> Path = FCsvProfiler::Get()->EndCapture();
// Return path to CSV file
```

Returns path to CSV file that AI can read and analyze.

### Priority 3: Object Count Report

**Command: `get_object_counts`**

Execute `obj list` and parse output. Return:
```json
{
    "StaticMeshComponent": 18432,
    "InstancedStaticMeshComponent": 256,
    "PointLightComponent": 42,
    "DirectionalLightComponent": 1,
    "NiagaraComponent": 15
}
```

### Priority 4: Memory Report

**Command: `get_memory_report`**

Trigger `memreport`, read the output file, return structured data.

### Priority 5: Trace Capture

**Command: `capture_trace`**

Start/stop Unreal Insights trace, return file path.

### Priority 6: RenderDoc/PIX Capture

**Command: `capture_render_frame`**

Check `IRenderCaptureProvider::IsAvailable()`, trigger capture, return path.

---

## Key Source Files Referenced

| File | Content |
|------|---------|
| `Runtime/Core/Public/Stats/GlobalStats.inl` | All global stat group declarations |
| `Runtime/Core/Private/ProfilingDebugging/TraceAuxiliary.cpp` | Unreal Insights trace commands |
| `Runtime/Core/Public/ProfilingDebugging/CsvProfiler.h` | CSV profiler API |
| `Runtime/Core/Private/Stats/StatsCommand.cpp` | stat startfile/stopfile |
| `Runtime/RHI/Public/GPUProfiler.h` | New GPU profiler (5.7) |
| `Runtime/RHI/Public/RHIStats.h` | RHI stat declarations |
| `Runtime/RenderCore/Public/RenderCore.h` | InitViews, LightRendering, ShadowRendering stat declarations |
| `Runtime/RenderCore/Public/DumpGPU.h` | DumpGPU API |
| `Runtime/RenderCore/Public/IRenderCaptureProvider.h` | RenderDoc/PIX capture API |
| `Runtime/Engine/Public/EngineStats.h` | Engine and Game stat declarations |
| `Runtime/Engine/Private/UnrealEngine.cpp` | Console command handlers (FreezeRendering, memreport, ProfileGPU, DumpGPU) |
| `Runtime/Engine/Public/Rendering/NaniteResources.h` | Nanite stat group declaration |
| `Runtime/Engine/Private/Rendering/NaniteStreamingManager.cpp` | NaniteStreaming stat declarations |
| `Runtime/Renderer/Private/DeferredShadingRenderer.cpp` | SceneRendering stat declarations |
| `Runtime/Renderer/Private/Shadows/ShadowSceneRenderer.cpp` | ShadowRendering stat declarations |
| `Runtime/Renderer/Private/Nanite/NaniteCullRaster.cpp` | Nanite CullingContexts stat |
| `Plugins/FX/Niagara/Source/Niagara/Private/` | Niagara stat declarations |
