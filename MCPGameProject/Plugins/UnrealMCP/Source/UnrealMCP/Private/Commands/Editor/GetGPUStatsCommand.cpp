#include "Commands/Editor/GetGPUStatsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Engine.h"
#include "DynamicRHI.h"

#if STATS
#include "Stats/StatsData.h"
#endif

#if RHI_NEW_GPU_PROFILER
#include "GPUProfiler.h"
#elif GPUPROFILERTRACE_ENABLED
#include "ProfilingDebugging/RealtimeGPUProfiler.h"
#endif

FString FGetGPUStatsCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

	// Total GPU frame time (always available)
	uint32 GPUCycles = RHIGetGPUFrameCycles(0);
	float TotalGPUMs = FPlatformTime::ToMilliseconds(GPUCycles);
	Result->SetNumberField(TEXT("total_gpu_ms"), TotalGPUMs);

#if STATS && RHI_NEW_GPU_PROFILER
	// UE 5.7 new GPU profiler: data flows through the stat system.
	// Requires "stat gpu" to be enabled via console command first.
	FGameThreadStatsData* ViewData = FLatestGameThreadStatsData::Get().Latest;
	if (!ViewData)
	{
		// Auto-enable stat gpu if not active
		if (GEngine)
		{
			UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			GEngine->Exec(World, TEXT("stat gpu"));
		}

		Result->SetBoolField(TEXT("success"), true);
		Result->SetBoolField(TEXT("gpu_stats_just_enabled"), true);
		Result->SetStringField(TEXT("message"), TEXT("GPU stats enabled. Call again for per-pass breakdown."));

		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
		return OutputString;
	}

	// Collect GPU stats from all active stat groups
	using EType = UE::RHI::GPUProfiler::FGPUStat::EType;

	struct FGpuPassData
	{
		double BusyAvg = 0;
		double BusyMax = 0;
		double BusyMin = 0;
		double WaitAvg = 0;
		double IdleAvg = 0;
	};

	TMap<FName, FGpuPassData> GroupedPasses;

	for (int32 GroupIdx = 0; GroupIdx < ViewData->ActiveStatGroups.Num(); ++GroupIdx)
	{
		const FActiveStatGroupInfo& StatGroup = ViewData->ActiveStatGroups[GroupIdx];
		for (const FComplexStatMessage& Message : StatGroup.GpuStatsAggregate)
		{
			FName ShortName = Message.GetShortName();
			int32 Number = ShortName.GetNumber();
			ShortName.SetNumber(0);

			FGpuPassData& Pass = GroupedPasses.FindOrAdd(ShortName);

			if (Message.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double)
			{
				double AvgVal = Message.GetValue_double(EComplexStatField::IncAve);
				double MaxVal = Message.GetValue_double(EComplexStatField::IncMax);
				double MinVal = Message.GetValue_double(EComplexStatField::IncMin);

				switch (EType(Number))
				{
				case EType::Busy:
					Pass.BusyAvg = AvgVal;
					Pass.BusyMax = MaxVal;
					Pass.BusyMin = MinVal;
					break;
				case EType::Wait:
					Pass.WaitAvg = AvgVal;
					break;
				case EType::Idle:
					Pass.IdleAvg = AvgVal;
					break;
				}
			}
			else if (Message.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64)
			{
				// Duration stats stored as int64 cycles
				double AvgMs = FPlatformTime::ToMilliseconds64(Message.GetValue_Duration(EComplexStatField::IncAve));
				double MaxMs = FPlatformTime::ToMilliseconds64(Message.GetValue_Duration(EComplexStatField::IncMax));
				double MinMs = FPlatformTime::ToMilliseconds64(Message.GetValue_Duration(EComplexStatField::IncMin));

				switch (EType(Number))
				{
				case EType::Busy:
					Pass.BusyAvg = AvgMs;
					Pass.BusyMax = MaxMs;
					Pass.BusyMin = MinMs;
					break;
				case EType::Wait:
					Pass.WaitAvg = AvgMs;
					break;
				case EType::Idle:
					Pass.IdleAvg = AvgMs;
					break;
				}
			}
		}
	}

	// Sort by busy time (highest first)
	TArray<TPair<FName, FGpuPassData>> SortedPasses;
	for (auto& Pair : GroupedPasses)
	{
		SortedPasses.Add(TPair<FName, FGpuPassData>(Pair.Key, Pair.Value));
	}
	SortedPasses.Sort([](const TPair<FName, FGpuPassData>& A, const TPair<FName, FGpuPassData>& B)
	{
		return A.Value.BusyAvg > B.Value.BusyAvg;
	});

	TArray<TSharedPtr<FJsonValue>> PassArray;
	float TotalBusyMs = 0;

	for (const auto& Pair : SortedPasses)
	{
		const FGpuPassData& Pass = Pair.Value;
		if (Pass.BusyAvg < 0.001) continue; // Skip negligible

		TSharedPtr<FJsonObject> PassObj = MakeShared<FJsonObject>();
		PassObj->SetStringField(TEXT("name"), Pair.Key.ToString());
		PassObj->SetNumberField(TEXT("busy_avg_ms"), Pass.BusyAvg);
		PassObj->SetNumberField(TEXT("busy_max_ms"), Pass.BusyMax);
		PassObj->SetNumberField(TEXT("busy_min_ms"), Pass.BusyMin);
		PassObj->SetNumberField(TEXT("wait_avg_ms"), Pass.WaitAvg);
		PassObj->SetNumberField(TEXT("idle_avg_ms"), Pass.IdleAvg);
		PassArray.Add(MakeShared<FJsonValueObject>(PassObj));
		TotalBusyMs += Pass.BusyAvg;
	}

	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("passes"), PassArray);
	Result->SetNumberField(TEXT("pass_count"), PassArray.Num());
	Result->SetNumberField(TEXT("total_busy_ms"), TotalBusyMs);

	// Summary of top 5
	FString Summary = FString::Printf(TEXT("GPU: %.1fms total, %.1fms busy. Top passes: "), TotalGPUMs, TotalBusyMs);
	for (int32 i = 0; i < FMath::Min(5, PassArray.Num()); ++i)
	{
		const TSharedPtr<FJsonObject>* PassObj;
		if (PassArray[i]->TryGetObject(PassObj))
		{
			Summary += FString::Printf(TEXT("%s=%.2fms"),
				*(*PassObj)->GetStringField(TEXT("name")),
				(*PassObj)->GetNumberField(TEXT("busy_avg_ms")));
			if (i < FMath::Min(5, PassArray.Num()) - 1) Summary += TEXT(", ");
		}
	}
	Result->SetStringField(TEXT("message"), Summary);

#elif GPUPROFILERTRACE_ENABLED
	// Legacy GPU profiler (UE 5.6 and earlier)
	static bool bGPUStatsEnabled = false;
	if (!bGPUStatsEnabled)
	{
		if (GEngine)
		{
			UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			GEngine->Exec(World, TEXT("stat gpu"));
		}
		bGPUStatsEnabled = true;

		Result->SetBoolField(TEXT("success"), true);
		Result->SetBoolField(TEXT("gpu_stats_just_enabled"), true);
		Result->SetStringField(TEXT("message"), TEXT("GPU stats enabled. Call again for per-pass breakdown."));

		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
		return OutputString;
	}

	TArray<FRealtimeGPUProfilerDescriptionResult> GPUResults;
	FRealtimeGPUProfiler::Get()->FetchPerfByDescription(GPUResults);

	GPUResults.Sort([](const FRealtimeGPUProfilerDescriptionResult& A, const FRealtimeGPUProfilerDescriptionResult& B)
	{
		return A.AverageTime > B.AverageTime;
	});

	TArray<TSharedPtr<FJsonValue>> PassArray;
	float TotalPassMs = 0;

	for (const auto& R : GPUResults)
	{
		float AvgMs = R.AverageTime / 1000.0f;
		if (AvgMs < 0.001f) continue;

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
	Result->SetStringField(TEXT("message"), TEXT("GPU profiling not available in this build (no STATS and no GPUPROFILERTRACE)"));
#endif

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return OutputString;
}

FString FGetGPUStatsCommand::GetCommandName() const { return TEXT("get_gpu_stats"); }
bool FGetGPUStatsCommand::ValidateParams(const FString& Parameters) const { return true; }
