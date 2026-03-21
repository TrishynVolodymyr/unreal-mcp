# Research: Per-Pass GPU Timing Breakdown in UE 5.7

## Summary

In UE 5.7, `RHI_NEW_GPU_PROFILER=1` by default, which means `GPUPROFILERTRACE_ENABLED=0`. The old `FRealtimeGPUProfiler::FetchPerfByDescription()` API is dead code. The new GPU profiler uses a completely different architecture based on `FEventSink` and breadcrumb events. Per-pass GPU timing data IS available in Development Editor builds through the UE stat system (`STATGROUP_GPU` equivalent), which the new `FGPUProfilerSink_StatSystem` populates via `FEndOfPipeStats`. There are two viable approaches to retrieve this data programmatically.

## Key Findings: Why Current Approach Fails

The define chain in `RHIDefinitions.h`:
```cpp
// RHIDefinitions.h line 68-77
#ifndef RHI_NEW_GPU_PROFILER
#define RHI_NEW_GPU_PROFILER 1           // <-- DEFAULT ON in 5.7
#endif

#ifndef GPUPROFILERTRACE_ENABLED
#if UE_TRACE_ENABLED && !UE_BUILD_SHIPPING
#define GPUPROFILERTRACE_ENABLED !RHI_NEW_GPU_PROFILER  // <-- evaluates to 0
#else
#define GPUPROFILERTRACE_ENABLED 0
#endif
#endif
```

With `RHI_NEW_GPU_PROFILER=1`:
- `GPUPROFILERTRACE_ENABLED = 0` -- old trace API disabled
- `FRealtimeGPUProfiler::FetchPerfByDescription()` -- does not exist (guarded by `#if GPUPROFILERTRACE_ENABLED`)
- `FRealtimeGPUProfiler` class itself -- does not exist (guarded by `#if RHI_NEW_GPU_PROFILER` at top of RealtimeGPUProfiler.h, excluded in the `#else` branch)
- `SCOPED_GPU_STAT()` -- empty macro (line 204-206 of RealtimeGPUProfiler.h)
- `GPU_STATS_BEGINFRAME/ENDFRAME` -- empty macros

The entire old `FRealtimeGPUProfilerFrame` / `FRealtimeGPUProfilerEvent` architecture is compiled out.

## UE 5.7 New GPU Profiler Architecture

### How It Works

The new profiler is in `RHI/Private/GPUProfiler.cpp` and `RHI/Public/GPUProfiler.h`, under `namespace UE::RHI::GPUProfiler`.

**Data flow:**
1. GPU work is tagged with `RHI_BREADCRUMB_EVENT_STAT(RHICmdList, StatName, "EventName")` macros
2. Each `DECLARE_GPU_STAT(StatName)` creates a `UE::RHI::GPUProfiler::TGPUStat<>` instance (not the old `DECLARE_FLOAT_COUNTER_STAT`)
3. Platform RHI (D3D12/Vulkan) writes GPU timestamps into `FEventStream` objects
4. `FEventSink` implementations consume these streams:
   - `FGPUProfilerSink_StatSystem` -- computes "stat unit" GPU time, "stat gpu" per-pass stats, and "profilegpu"
   - Trace sink (if `UE_TRACE_GPU_PROFILER_ENABLED`) -- sends to Unreal Insights

**Stat emission path (GPUProfiler.cpp line 1659-1666):**
```cpp
// Inside FGPUProfilerSink_StatSystem::FStatState::EmitResults()
#if STATS
Stats->AddMessage(GPUStat.GetStatId(Queue, FGPUStat::EType::Busy).GetName(),
    EStatOperation::Set, FPlatformTime::ToMilliseconds64(Inclusive.BusyCycles));
Stats->AddMessage(GPUStat.GetStatId(Queue, FGPUStat::EType::Idle).GetName(),
    EStatOperation::Set, FPlatformTime::ToMilliseconds64(Inclusive.IdleCycles));
Stats->AddMessage(GPUStat.GetStatId(Queue, FGPUStat::EType::Wait).GetName(),
    EStatOperation::Set, FPlatformTime::ToMilliseconds64(Inclusive.WaitCycles));
#endif
```

This means: with `STATS=1` (true in Development Editor), per-pass GPU timing data IS being pushed into the UE stat system under dynamic stat groups (not the old `STATGROUP_GPU`). The stats are categorized under `STATCAT_GPU`.

### What's Available in Development Editor

| Define | Value | Meaning |
|--------|-------|---------|
| `RHI_NEW_GPU_PROFILER` | 1 | New profiler active |
| `GPUPROFILERTRACE_ENABLED` | 0 | Old trace API disabled |
| `HAS_GPU_STATS` | 1 | `(STATS \|\| CSV_PROFILER_STATS \|\| GPUPROFILERTRACE_ENABLED) && !SHIPPING` |
| `STATS` | 1 | UE stat system active |
| `WITH_PROFILEGPU` | 1 | ProfileGPU command available |
| `WITH_RHI_BREADCRUMBS` | 1 | Breadcrumb events available |
| `CSV_PROFILER_STATS` | 1 | CSV profiler available |

## Approach 1: Console Command + Log Parsing (Simplest)

Execute `stat gpu` console command, which in the new profiler toggles all `STATCAT_GPU` stat groups. The stats are rendered on-screen by `StatsRender2.cpp` which reads from `FActiveStatGroupInfo::GpuStatsAggregate`.

Alternatively, execute `ProfileGPU` console command which triggers a one-shot profile capture that dumps results to the log.

```cpp
// Trigger a one-shot GPU profile dump to log
GEngine->Exec(World, TEXT("ProfileGPU"));
// Then parse MCPGameProject/Saved/Logs/MCPGameProject.log for the results
```

**Limitation:** Not programmatic. Data ends up in log, not in a structured format you can return as JSON.

## Approach 2: Read Stats via FStatsThreadState (Recommended)

The stat system aggregates GPU timing data. You can read it programmatically on the game thread:

```cpp
#include "Stats/Stats.h"
#include "Stats/StatsData.h"

// First, ensure GPU stat groups are enabled (equivalent to "stat gpu")
// This needs to be done once.
void EnableGPUStats()
{
    if (GEngine)
    {
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        GEngine->Exec(World, TEXT("stat gpu"));
    }
}

// Then, read the aggregated stats:
// The HUD stat rendering system already collects these into FActiveStatGroupInfo.
// We can use similar logic via FStatsThreadState.
```

The problem is that `FStatsThreadState` operates on the stats thread and the data is complex to extract. The more practical route is through the stat group delegate system.

## Approach 3: RHIGetGPUFrameCycles + CSV Profiler (Best for MCP)

### Total GPU Time (Already Working)

```cpp
uint32 GPUCycles = RHIGetGPUFrameCycles(0);
float TotalGPUMs = FPlatformTime::ToMilliseconds(GPUCycles);
```

This works in all builds. Updated via `GRHIGPUFrameTimeHistory.PushFrameCycles()` in `ProcessFrame()`.

### Per-Pass Breakdown via CSV Profiler

The new GPU profiler emits per-pass data to CSV profiler (GPUProfiler.cpp line 1669-1675):
```cpp
#if CSV_PROFILER_STATS
if (CsvProfiler && Queue.Type == FQueue::EType::Graphics && Queue.Index == 0)
{
    CsvProfiler->RecordCustomStat(
        GPUStat.DisplayName, CSV_CATEGORY_INDEX(GPU),
        FPlatformTime::ToMilliseconds64(BusyCycles), ECsvCustomStatOp::Set);
}
#endif
```

You can capture a CSV profile programmatically:
```cpp
// Start CSV capture
GEngine->Exec(World, TEXT("csvprofile start"));
// ... wait some frames ...
// Stop and read the CSV file
GEngine->Exec(World, TEXT("csvprofile stop"));
```

**Limitation:** Asynchronous, file-based. Not ideal for real-time queries.

## Approach 4: Direct Stat System Query (Most Practical for MCP)

The stat HUD system already reads GPU stats. The key is `FHUDGroupManager` which manages active stat groups and their data. When `stat gpu` is active, GPU timing data flows into `FActiveStatGroupInfo::GpuStatsAggregate` as `FComplexStatMessage` objects.

However, `FHUDGroupManager` is tightly coupled to the viewport rendering system and not easily accessible from an editor subsystem.

**The most practical approach** is to use `FCoreDelegates::StatCheckEnabled` and the stat group toggle mechanism to enable GPU stats, then hook into the stat thread's frame delegate to read the aggregated data.

## Recommended Implementation for MCP

### Strategy: Execute Console Command + Parse Stat Data

The cleanest approach for the MCP plugin:

1. **For total GPU time:** Use `RHIGetGPUFrameCycles(0)` -- already working
2. **For per-pass breakdown:** Execute `ProfileGPU` console command and capture its output

The `ProfileGPU` command (registered in GPUProfiler.cpp line 2650-2657) triggers `FGPUProfilerSink_StatSystem` to capture one frame of detailed GPU timing. The results are logged.

```cpp
// Execute ProfileGPU and capture output
GEngine->Exec(World, TEXT("ProfileGPU"));
// Read from log: "LogRHI: Display: ..." output
```

### Alternative: Hook into FStatsThreadState::NewFrameDelegate

For real-time per-frame data without log parsing:

```cpp
#if STATS
FStatsThreadState& StatsState = FStatsThreadState::GetLocalState();

// Register for new frame callbacks
StatsState.NewFrameDelegate.AddLambda([](int64 Frame)
{
    // Access the latest frame's condensed stats
    // Filter for GPU stats (EStatMetaFlags::IsGPU flag)
    // ... extract timing data
});
#endif
```

This is the most architecturally clean approach but requires careful thread synchronization (the delegate fires on the stats thread, not the game thread).

### Simplest Working Approach

Execute `stat gpu` once to enable GPU stat collection. Then on subsequent frames, read the stat values directly:

```cpp
#if STATS
// Read a specific GPU stat value
// The stats are named like "Stat_GPU_<QueueType>_<StatName>_<Type>"
// where Type is Busy/Idle/Wait

// Use the stat system's internal query mechanism
const FComplexStatMessage* StatData = /* from active stat group info */;
if (StatData)
{
    double ValueMs = StatData->GetValue_double(EComplexStatField::IncAve);
}
#endif
```

## Gap Analysis

| Need | Have | Gap |
|------|------|-----|
| Total GPU frame time | `RHIGetGPUFrameCycles()` | None -- works |
| Per-pass GPU timing | Old: `FetchPerfByDescription()` | Completely unavailable. New profiler uses stat system |
| Programmatic stat read | `FStatsThreadState` + delegates | Need to implement stat reading logic |
| Console-triggered capture | `ProfileGPU` command | Need to capture output programmatically |

## Reference Files

- **New GPU profiler sink:** `ues/.../Engine/Source/Runtime/RHI/Private/GPUProfiler.cpp` (lines 1517-2668)
- **New GPU profiler types:** `ues/.../Engine/Source/Runtime/RHI/Public/GPUProfiler.h`
- **Define chain:** `ues/.../Engine/Source/Runtime/RHI/Public/RHIDefinitions.h` (lines 63-84)
- **Old profiler (dead code):** `ues/.../Engine/Source/Runtime/RenderCore/Public/ProfilingDebugging/RealtimeGPUProfiler.h`
- **Old profiler impl (dead code):** `ues/.../Engine/Source/Runtime/RenderCore/Private/ProfilingDebugging/RealtimeGPUProfiler.cpp`
- **Stat rendering (shows how stats are read):** `ues/.../Engine/Source/Runtime/Engine/Private/StatsRender2.cpp` (line 1028+)
- **Stat data structures:** `ues/.../Engine/Source/Runtime/Core/Public/Stats/StatsData.h`
- **EndOfPipeStats:** `ues/.../Engine/Source/Runtime/Core/Public/Stats/StatsSystemTypes.h` (line 1471)
- **GPU frame time history:** `ues/.../Engine/Source/Runtime/RHI/Public/GPUProfiler.h` (line 938, `FRHIGPUFrameTimeHistory`)
- **RHI breadcrumb stat macros:** `ues/.../Engine/Source/Runtime/RHI/Public/RHIBreadcrumbs.h` (line 1348+)
- **Existing MCP command:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/Editor/GetGPUStatsCommand.cpp`
- **stat gpu command handler:** `ues/.../Engine/Source/Runtime/Core/Private/Stats/StatsCommand.cpp` (line 2148)
