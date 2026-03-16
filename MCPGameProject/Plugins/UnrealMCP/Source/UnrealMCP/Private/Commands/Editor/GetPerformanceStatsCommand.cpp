#include "Commands/Editor/GetPerformanceStatsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "HAL/PlatformMemory.h"
#include "Misc/App.h"
#include "RHIStats.h"
#include "DynamicRHI.h"
#include "Engine/Engine.h"

// GAverageFPS is declared in Engine module but not in a public header
extern ENGINE_API float GAverageFPS;
extern ENGINE_API float GAverageMS;

FString FGetPerformanceStatsCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);

	// Frame timing
	double DeltaTime = FApp::GetDeltaTime();
	Result->SetNumberField(TEXT("fps_current"), DeltaTime > 0 ? 1.0 / DeltaTime : 0);
	Result->SetNumberField(TEXT("fps_average"), (double)GAverageFPS);
	Result->SetNumberField(TEXT("frame_time_ms"), DeltaTime * 1000.0);
	Result->SetNumberField(TEXT("average_frame_time_ms"), (double)GAverageMS);

	// GPU time
	uint32 GPUCycles = RHIGetGPUFrameCycles(0);
	Result->SetNumberField(TEXT("gpu_time_ms"), FPlatformTime::ToMilliseconds(GPUCycles));

	// Render stats
	Result->SetNumberField(TEXT("draw_calls"), (double)GNumDrawCallsRHI[0]);
	Result->SetNumberField(TEXT("primitives_drawn"), (double)GNumPrimitivesDrawnRHI[0]);

	// Memory stats
	FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
	TSharedPtr<FJsonObject> MemObj = MakeShared<FJsonObject>();
	MemObj->SetNumberField(TEXT("used_physical_mb"), (double)(MemStats.UsedPhysical / (1024 * 1024)));
	MemObj->SetNumberField(TEXT("available_physical_mb"), (double)(MemStats.AvailablePhysical / (1024 * 1024)));
	MemObj->SetNumberField(TEXT("peak_used_physical_mb"), (double)(MemStats.PeakUsedPhysical / (1024 * 1024)));
	MemObj->SetNumberField(TEXT("used_virtual_mb"), (double)(MemStats.UsedVirtual / (1024 * 1024)));
	MemObj->SetNumberField(TEXT("available_virtual_mb"), (double)(MemStats.AvailableVirtual / (1024 * 1024)));
	Result->SetObjectField(TEXT("memory"), MemObj);

	// Frame counter
	Result->SetNumberField(TEXT("frame_number"), (double)GFrameCounter);

	Result->SetStringField(TEXT("message"), FString::Printf(
		TEXT("FPS: %.1f avg, %.1f current | GPU: %.2fms | Draw calls: %d | Tris: %d | RAM: %lldMB"),
		GAverageFPS, DeltaTime > 0 ? 1.0f / DeltaTime : 0.f,
		FPlatformTime::ToMilliseconds(GPUCycles),
		GNumDrawCallsRHI[0], GNumPrimitivesDrawnRHI[0],
		MemStats.UsedPhysical / (1024 * 1024)));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return OutputString;
}

FString FGetPerformanceStatsCommand::GetCommandName() const
{
	return TEXT("get_performance_stats");
}

bool FGetPerformanceStatsCommand::ValidateParams(const FString& Parameters) const
{
	return true; // No required params
}
