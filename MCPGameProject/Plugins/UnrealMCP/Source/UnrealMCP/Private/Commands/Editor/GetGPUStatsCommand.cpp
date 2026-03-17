#include "Commands/Editor/GetGPUStatsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Engine.h"
#include "DynamicRHI.h"

#if GPUPROFILERTRACE_ENABLED
#include "ProfilingDebugging/RealtimeGPUProfiler.h"
#endif

FString FGetGPUStatsCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

	// Total GPU frame time
	uint32 GPUCycles = RHIGetGPUFrameCycles(0);
	float TotalGPUMs = FPlatformTime::ToMilliseconds(GPUCycles);
	Result->SetNumberField(TEXT("total_gpu_ms"), TotalGPUMs);

#if GPUPROFILERTRACE_ENABLED
	// Enable GPU stats collection if not already active
	static bool bGPUStatsEnabled = false;
	if (!bGPUStatsEnabled)
	{
		if (GEngine)
		{
			UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			GEngine->Exec(World, TEXT("stat gpu"));
		}
		bGPUStatsEnabled = true;

		// First call won't have data yet — return total only
		Result->SetBoolField(TEXT("success"), true);
		Result->SetBoolField(TEXT("gpu_stats_just_enabled"), true);
		Result->SetStringField(TEXT("message"), TEXT("GPU stats enabled. Call again for per-pass breakdown."));

		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
		return OutputString;
	}

	// Fetch per-pass GPU timing
	TArray<FRealtimeGPUProfilerDescriptionResult> GPUResults;
	FRealtimeGPUProfiler::Get()->FetchPerfByDescription(GPUResults);

	// Sort by average time (highest first)
	GPUResults.Sort([](const FRealtimeGPUProfilerDescriptionResult& A, const FRealtimeGPUProfilerDescriptionResult& B)
	{
		return A.AverageTime > B.AverageTime;
	});

	TArray<TSharedPtr<FJsonValue>> PassArray;
	float TotalPassMs = 0;

	for (const auto& R : GPUResults)
	{
		float AvgMs = R.AverageTime / 1000.0f;
		if (AvgMs < 0.001f) continue; // Skip negligible passes

		TSharedPtr<FJsonObject> PassObj = MakeShared<FJsonObject>();
		PassObj->SetStringField(TEXT("name"), R.Description);
		PassObj->SetNumberField(TEXT("avg_ms"), AvgMs);
		PassObj->SetNumberField(TEXT("min_ms"), R.MinTime / 1000.0f);
		PassObj->SetNumberField(TEXT("max_ms"), R.MaxTime / 1000.0f);
		PassArray.Add(MakeShared<FJsonValueObject>(PassObj));
		TotalPassMs += AvgMs;
	}

	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("passes"), PassArray);
	Result->SetNumberField(TEXT("pass_count"), PassArray.Num());
	Result->SetNumberField(TEXT("total_pass_ms"), TotalPassMs);

	// Build summary of top 5 passes
	FString Summary = FString::Printf(TEXT("GPU: %.1fms total. Top passes: "), TotalGPUMs);
	for (int32 i = 0; i < FMath::Min(5, PassArray.Num()); ++i)
	{
		const TSharedPtr<FJsonObject>* PassObj;
		if (PassArray[i]->TryGetObject(PassObj))
		{
			Summary += FString::Printf(TEXT("%s=%.2fms"),
				*(*PassObj)->GetStringField(TEXT("name")),
				(*PassObj)->GetNumberField(TEXT("avg_ms")));
			if (i < FMath::Min(5, PassArray.Num()) - 1) Summary += TEXT(", ");
		}
	}
	Result->SetStringField(TEXT("message"), Summary);

#else
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("message"), TEXT("GPU profiling not available in this build (GPUPROFILERTRACE_ENABLED=0)"));
#endif

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return OutputString;
}

FString FGetGPUStatsCommand::GetCommandName() const { return TEXT("get_gpu_stats"); }
bool FGetGPUStatsCommand::ValidateParams(const FString& Parameters) const { return true; }
