#include "Services/IGPUStatsCaptureBackend.h"

#include "DynamicRHI.h"

#if STATS
#include "Stats/StatsData.h"
#endif

#include "GPUProfiler.h"

namespace
{
class FGPUStatsCaptureBackend final : public IGPUStatsCaptureBackend
{
public:
	FGPUStatsCaptureBackend()
		: StatCapture(CreateEditorStatCaptureBackend(TEXT("GPU"), true))
	{
	}

	virtual double GetTotalGPUMilliseconds() const override
	{
		return FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0));
	}

	virtual bool IsCaptureEnabled() const override
	{
		return StatCapture->IsCaptureEnabled();
	}

	virtual bool IsStatsRenderingEnabled() const override
	{
		return StatCapture->IsStatsRenderingEnabled();
	}

	virtual void SetCaptureEnabled(bool bEnabled, bool bRenderStats) override
	{
		StatCapture->SetCaptureEnabled(bEnabled, bRenderStats);
	}

	virtual bool TryReadDetailedSnapshot(FGPUStatsSnapshot& OutSnapshot) const override
	{
#if STATS
		if (!IsCaptureEnabled())
		{
			return false;
		}

		const FGameThreadStatsData* ViewData = FLatestGameThreadStatsData::Get().Latest;
		if (!ViewData)
		{
			return false;
		}

		using EType = UE::RHI::GPUProfiler::FGPUStat::EType;
		struct FAggregatedPass
		{
			double BusyAverage = 0.0;
			double BusyMaximum = 0.0;
			double BusyMinimum = 0.0;
			double WaitAverage = 0.0;
			double IdleAverage = 0.0;
		};

		TMap<FName, FAggregatedPass> GroupedPasses;
		for (const FActiveStatGroupInfo& StatGroup : ViewData->ActiveStatGroups)
		{
			for (const FComplexStatMessage& Message : StatGroup.GpuStatsAggregate)
			{
				FName ShortName = Message.GetShortName();
				const int32 Number = ShortName.GetNumber();
				ShortName.SetNumber(0);
				FAggregatedPass& Pass = GroupedPasses.FindOrAdd(ShortName);

				double Average = 0.0;
				double Maximum = 0.0;
				double Minimum = 0.0;
				if (Message.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double)
				{
					Average = Message.GetValue_double(EComplexStatField::IncAve);
					Maximum = Message.GetValue_double(EComplexStatField::IncMax);
					Minimum = Message.GetValue_double(EComplexStatField::IncMin);
				}
				else if (Message.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64)
				{
					Average = FPlatformTime::ToMilliseconds64(Message.GetValue_Duration(EComplexStatField::IncAve));
					Maximum = FPlatformTime::ToMilliseconds64(Message.GetValue_Duration(EComplexStatField::IncMax));
					Minimum = FPlatformTime::ToMilliseconds64(Message.GetValue_Duration(EComplexStatField::IncMin));
				}
				else
				{
					continue;
				}

				switch (EType(Number))
				{
				case EType::Busy:
					Pass.BusyAverage = Average;
					Pass.BusyMaximum = Maximum;
					Pass.BusyMinimum = Minimum;
					break;
				case EType::Wait:
					Pass.WaitAverage = Average;
					break;
				case EType::Idle:
					Pass.IdleAverage = Average;
					break;
				}
			}
		}

		if (GroupedPasses.IsEmpty())
		{
			return false;
		}

		OutSnapshot = FGPUStatsSnapshot{};
		for (const TPair<FName, FAggregatedPass>& Pair : GroupedPasses)
		{
			if (Pair.Value.BusyAverage < 0.001)
			{
				continue;
			}
			FGPUStatsPassSample& Sample = OutSnapshot.Passes.AddDefaulted_GetRef();
			Sample.Name = Pair.Key.ToString();
			Sample.BusyAverageMilliseconds = Pair.Value.BusyAverage;
			Sample.BusyMaximumMilliseconds = Pair.Value.BusyMaximum;
			Sample.BusyMinimumMilliseconds = Pair.Value.BusyMinimum;
			Sample.WaitAverageMilliseconds = Pair.Value.WaitAverage;
			Sample.IdleAverageMilliseconds = Pair.Value.IdleAverage;
			OutSnapshot.TotalBusyMilliseconds += Pair.Value.BusyAverage;
		}

		OutSnapshot.Passes.Sort([](const FGPUStatsPassSample& A, const FGPUStatsPassSample& B)
		{
			return A.BusyAverageMilliseconds > B.BusyAverageMilliseconds;
		});
		return !OutSnapshot.Passes.IsEmpty();
#else
		return false;
#endif
	}

private:
	TSharedRef<IEditorStatCaptureBackend> StatCapture;
};
}

TSharedRef<IGPUStatsCaptureBackend> CreateGPUStatsCaptureBackend()
{
	return MakeShared<FGPUStatsCaptureBackend>();
}
