#include "Commands/Editor/GetRenderingStatsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "DynamicRHI.h"
#include "RHIStats.h"
#include "RHI.h"
#include "Engine/Engine.h"

#if STATS
#include "Stats/StatsData.h"
#include "Stats/Stats.h"
#endif

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
#if RHI_NEW_GPU_PROFILER
	// UE 5.7+ new GPU profiler: per-category draw call breakdown not available via legacy FRHIDrawStatsCategory API
	Result->SetArrayField(TEXT("draw_call_categories"), TArray<TSharedPtr<FJsonValue>>());
	Result->SetStringField(TEXT("draw_call_categories_note"),
		TEXT("Per-category breakdown unavailable in UE 5.7+ (RHI_NEW_GPU_PROFILER). Use get_gpu_stats for per-pass GPU timing."));
#elif HAS_GPU_STATS
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

	// === Visibility / InitViews Stats ===
#if STATS
	{
		TSharedPtr<FJsonObject> VisObj = MakeShared<FJsonObject>();
		FGameThreadStatsData* ViewData = FLatestGameThreadStatsData::Get().Latest;
		if (!ViewData)
		{
			// Auto-enable stat initviews if not active
			if (GEngine)
			{
				UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
				GEngine->Exec(World, TEXT("stat initviews"));
			}
			VisObj->SetBoolField(TEXT("just_enabled"), true);
			VisObj->SetStringField(TEXT("note"), TEXT("InitViews stats enabled. Call again for visibility data."));
		}
		else
		{
			// Use short FNames (GetShortName() returns these) — NOT GET_STATFNAME() which returns long encoded paths
			struct FStatMapping
			{
				FName StatName;
				const TCHAR* JsonKey;
			};
			const FStatMapping Mappings[] = {
				{ FName(TEXT("STAT_ProcessedPrimitives")),          TEXT("processed_primitives") },
				{ FName(TEXT("STAT_CulledPrimitives")),             TEXT("frustum_culled_primitives") },
				{ FName(TEXT("STAT_OccludedPrimitives")),           TEXT("occluded_primitives") },
				{ FName(TEXT("STAT_StaticallyOccludedPrimitives")), TEXT("statically_occluded") },
				{ FName(TEXT("STAT_VisibleStaticMeshElements")),    TEXT("visible_static_mesh_elements") },
				{ FName(TEXT("STAT_VisibleDynamicPrimitives")),     TEXT("visible_dynamic_primitives") },
				{ FName(TEXT("STAT_OcclusionQueries")),             TEXT("occlusion_queries") },
			};

			// Collect counter values — stored as double in stats system, read via GetValue_double
			TMap<FName, double> StatValues;
			for (int32 GroupIdx = 0; GroupIdx < ViewData->ActiveStatGroups.Num(); ++GroupIdx)
			{
				const FActiveStatGroupInfo& StatGroup = ViewData->ActiveStatGroups[GroupIdx];
				for (const FComplexStatMessage& Message : StatGroup.CountersAggregate)
				{
					FName ShortName = Message.GetShortName();
					ShortName.SetNumber(0);
					double Value = Message.GetValue_double(EComplexStatField::IncAve);
					StatValues.Add(ShortName, Value);
				}
			}

			// Extract the specific stats we care about
			for (const FStatMapping& M : Mappings)
			{
				double* Found = StatValues.Find(M.StatName);
				VisObj->SetNumberField(M.JsonKey, Found ? FMath::RoundToInt(*Found) : 0.0);
			}
		}
		Result->SetObjectField(TEXT("visibility"), VisObj);
	}
#endif

	// === Summary ===
	FString Summary = FString::Printf(
		TEXT("Draw calls: %d, Tris: %d, GPU: %.1fms"),
		GNumDrawCallsRHI[0], GNumPrimitivesDrawnRHI[0],
		FPlatformTime::ToMilliseconds(GPUCycles));

#if RHI_NEW_GPU_PROFILER
	// No per-category breakdown available in UE 5.7+
#elif HAS_GPU_STATS
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
