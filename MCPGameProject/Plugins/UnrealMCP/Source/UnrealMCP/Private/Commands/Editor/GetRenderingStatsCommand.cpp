#include "Commands/Editor/GetRenderingStatsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "DynamicRHI.h"
#include "RHIStats.h"
#include "RHI.h"
#include "Engine/Engine.h"

FString FGetRenderingStatsCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);

	// === RHI Global Counters ===
	Result->SetNumberField(TEXT("draw_calls"), (double)GNumDrawCallsRHI[0]);
	Result->SetNumberField(TEXT("primitives_drawn"), (double)GNumPrimitivesDrawnRHI[0]);

	// === GPU Frame Time ===
	uint32 GPUCycles = RHIGetGPUFrameCycles(0);
	Result->SetNumberField(TEXT("gpu_time_ms"), FPlatformTime::ToMilliseconds(GPUCycles));

	// === Draw Call Categories ===
#if HAS_GPU_STATS
	{
		TArray<TSharedPtr<FJsonValue>> CategoriesArray;
		FRHIDrawStatsCategory::FManager& Manager = FRHIDrawStatsCategory::GetManager();
		for (int32 i = 0; i < Manager.NumCategory; ++i)
		{
			int32 DrawCount = Manager.DisplayCounts[i][0];
			if (DrawCount <= 0) continue;

			TSharedPtr<FJsonObject> CatObj = MakeShared<FJsonObject>();
			CatObj->SetStringField(TEXT("name"), Manager.Array[i]->Name.ToString());
			CatObj->SetNumberField(TEXT("draw_calls"), DrawCount);
			CategoriesArray.Add(MakeShared<FJsonValueObject>(CatObj));
		}

		// Sort by draw calls descending
		CategoriesArray.Sort([](const TSharedPtr<FJsonValue>& A, const TSharedPtr<FJsonValue>& B)
		{
			return A->AsObject()->GetNumberField(TEXT("draw_calls")) > B->AsObject()->GetNumberField(TEXT("draw_calls"));
		});

		Result->SetArrayField(TEXT("draw_call_categories"), CategoriesArray);
	}
#endif

	// === VRAM Stats ===
	{
		FTextureMemoryStats TexMemStats;
		RHIGetTextureMemoryStats(TexMemStats);

		TSharedPtr<FJsonObject> VramObj = MakeShared<FJsonObject>();
		if (TexMemStats.DedicatedVideoMemory > 0)
		{
			VramObj->SetNumberField(TEXT("dedicated_vram_mb"), (double)(TexMemStats.DedicatedVideoMemory / (1024 * 1024)));
			VramObj->SetNumberField(TEXT("budget_mb"), (double)(TexMemStats.TotalGraphicsMemory / (1024 * 1024)));
		}
		if (TexMemStats.StreamingMemorySize > 0)
		{
			VramObj->SetNumberField(TEXT("streaming_mb"), (double)(TexMemStats.StreamingMemorySize / (1024 * 1024)));
			VramObj->SetNumberField(TEXT("non_streaming_mb"), (double)(TexMemStats.NonStreamingMemorySize / (1024 * 1024)));
		}
		Result->SetObjectField(TEXT("vram"), VramObj);
	}

	// === Summary ===
	FString Summary = FString::Printf(
		TEXT("Draw calls: %d, Tris: %d, GPU: %.1fms"),
		GNumDrawCallsRHI[0], GNumPrimitivesDrawnRHI[0],
		FPlatformTime::ToMilliseconds(GPUCycles));

#if HAS_GPU_STATS
	{
		FRHIDrawStatsCategory::FManager& Manager = FRHIDrawStatsCategory::GetManager();
		Summary += TEXT(" | Categories: ");
		int32 Shown = 0;
		for (int32 i = 0; i < Manager.NumCategory && Shown < 5; ++i)
		{
			int32 DrawCount = Manager.DisplayCounts[i][0];
			if (DrawCount <= 0) continue;
			if (Shown > 0) Summary += TEXT(", ");
			Summary += FString::Printf(TEXT("%s=%d"), *Manager.Array[i]->Name.ToString(), DrawCount);
			++Shown;
		}
	}
#endif

	Result->SetStringField(TEXT("message"), Summary);

	FString Out;
	TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Result.ToSharedRef(), W);
	return Out;
}

FString FGetRenderingStatsCommand::GetCommandName() const { return TEXT("get_rendering_stats"); }
bool FGetRenderingStatsCommand::ValidateParams(const FString& Parameters) const { return true; }
