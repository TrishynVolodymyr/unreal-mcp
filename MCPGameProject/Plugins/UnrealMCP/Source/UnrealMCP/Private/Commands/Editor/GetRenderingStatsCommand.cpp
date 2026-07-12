#include "Commands/Editor/GetRenderingStatsCommand.h"

#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "RHI.h"
#include "RHIStats.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
FString SerializeRenderingResponse(const TSharedRef<FJsonObject>& Result)
{
	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Result, Writer);
	return Output;
}

TSharedPtr<FJsonObject> BuildUnavailableVisibilityPayload()
{
	TSharedPtr<FJsonObject> Visibility = MakeShared<FJsonObject>();
	Visibility->SetBoolField(TEXT("detailed_available"), false);
	Visibility->SetStringField(
		TEXT("note"),
		TEXT("Fresh InitViews counters are unavailable through the UE 5.8 editor stats snapshot. Use Unreal Insights for visibility profiling."));
	return Visibility;
}
}

FString FGetRenderingStatsCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> Request;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters.IsEmpty() ? TEXT("{}") : Parameters);
	if (!FJsonSerializer::Deserialize(Reader, Request) || !Request.IsValid())
	{
		const TSharedRef<FJsonObject> ErrorResult = MakeShared<FJsonObject>();
		ErrorResult->SetBoolField(TEXT("success"), false);
		ErrorResult->SetStringField(TEXT("error"), TEXT("Parameters must be a JSON object"));
		return SerializeRenderingResponse(ErrorResult);
	}

	FString Action = TEXT("snapshot");
	Request->TryGetStringField(TEXT("action"), Action);
	Action.ToLowerInline();
	if (Action != TEXT("snapshot"))
	{
		const TSharedRef<FJsonObject> ErrorResult = MakeShared<FJsonObject>();
		ErrorResult->SetBoolField(TEXT("success"), false);
		ErrorResult->SetStringField(TEXT("error"), FString::Printf(TEXT("Unknown action '%s'"), *Action));
		return SerializeRenderingResponse(ErrorResult);
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetNumberField(TEXT("draw_calls"), static_cast<double>(GNumDrawCallsRHI[0]));
	Result->SetNumberField(TEXT("primitives_drawn"), static_cast<double>(GNumPrimitivesDrawnRHI[0]));

	const uint32 GPUCycles = RHIGetGPUFrameCycles(0);
	Result->SetNumberField(TEXT("gpu_time_ms"), FPlatformTime::ToMilliseconds(GPUCycles));

	Result->SetArrayField(TEXT("draw_call_categories"), TArray<TSharedPtr<FJsonValue>>());
	Result->SetStringField(
		TEXT("draw_call_categories_note"),
		TEXT("Per-category breakdown is unavailable in UE 5.8. Use get_gpu_stats for per-pass GPU timing."));

	FTextureMemoryStats TexMemStats;
	RHIGetTextureMemoryStats(TexMemStats);
	TSharedPtr<FJsonObject> Vram = MakeShared<FJsonObject>();
	if (TexMemStats.DedicatedVideoMemory > 0)
	{
		Vram->SetNumberField(TEXT("dedicated_vram_mb"), static_cast<double>(TexMemStats.DedicatedVideoMemory / (1024 * 1024)));
		Vram->SetNumberField(TEXT("budget_mb"), static_cast<double>(TexMemStats.TotalGraphicsMemory / (1024 * 1024)));
	}
	if (TexMemStats.StreamingMemorySize > 0)
	{
		Vram->SetNumberField(TEXT("streaming_mb"), static_cast<double>(TexMemStats.StreamingMemorySize / (1024 * 1024)));
		Vram->SetNumberField(TEXT("non_streaming_mb"), static_cast<double>(TexMemStats.NonStreamingMemorySize / (1024 * 1024)));
	}
	Result->SetObjectField(TEXT("vram"), Vram);

	// The UE 5.8 editor snapshot has no freshness signal for InitViews counters.
	// Never return cached or zero counters as though they were a fresh sample.
	Result->SetObjectField(TEXT("visibility"), BuildUnavailableVisibilityPayload());
	Result->SetStringField(
		TEXT("message"),
		FString::Printf(
			TEXT("Draw calls: %d, Tris: %d, GPU: %.1fms"),
			GNumDrawCallsRHI[0],
			GNumPrimitivesDrawnRHI[0],
			FPlatformTime::ToMilliseconds(GPUCycles)));
	return SerializeRenderingResponse(Result.ToSharedRef());
}

FString FGetRenderingStatsCommand::GetCommandName() const
{
	return TEXT("get_rendering_stats");
}

bool FGetRenderingStatsCommand::ValidateParams(const FString& Parameters) const
{
	return true;
}
