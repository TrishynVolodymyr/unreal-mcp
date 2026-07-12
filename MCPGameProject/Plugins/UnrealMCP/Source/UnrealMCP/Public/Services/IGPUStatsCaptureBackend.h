#pragma once

#include "CoreMinimal.h"
#include "Services/IEditorStatCapture.h"

struct FGPUStatsPassSample
{
	FString Name;
	double BusyAverageMilliseconds = 0.0;
	double BusyMaximumMilliseconds = 0.0;
	double BusyMinimumMilliseconds = 0.0;
	double WaitAverageMilliseconds = 0.0;
	double IdleAverageMilliseconds = 0.0;
};

struct FGPUStatsSnapshot
{
	TArray<FGPUStatsPassSample> Passes;
	double TotalBusyMilliseconds = 0.0;
};

class UNREALMCP_API IGPUStatsCaptureBackend : public IEditorStatCaptureBackend
{
public:
	virtual ~IGPUStatsCaptureBackend() = default;
	virtual double GetTotalGPUMilliseconds() const = 0;
	virtual bool TryReadDetailedSnapshot(FGPUStatsSnapshot& OutSnapshot) const = 0;
};

UNREALMCP_API TSharedRef<IGPUStatsCaptureBackend> CreateGPUStatsCaptureBackend();
