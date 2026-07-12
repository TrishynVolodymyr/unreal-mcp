#include "Commands/Editor/GetGPUStatsCommand.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Services/IEditorStatCapture.h"
#include "Services/IGPUStatsCaptureBackend.h"

namespace
{
FString SerializeResponse(const TSharedRef<FJsonObject>& Result)
{
	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Result, Writer);
	return Output;
}

TSharedRef<FJsonObject> MakeBaseResponse(const IGPUStatsCaptureBackend& Backend)
{
	const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetNumberField(TEXT("total_gpu_ms"), Backend.GetTotalGPUMilliseconds());
	return Result;
}

void AddDetailedSnapshot(FJsonObject& Result, const FGPUStatsSnapshot& Snapshot)
{
	TArray<TSharedPtr<FJsonValue>> PassValues;
	for (const FGPUStatsPassSample& Pass : Snapshot.Passes)
	{
		const TSharedRef<FJsonObject> PassObject = MakeShared<FJsonObject>();
		PassObject->SetStringField(TEXT("name"), Pass.Name);
		PassObject->SetNumberField(TEXT("busy_avg_ms"), Pass.BusyAverageMilliseconds);
		PassObject->SetNumberField(TEXT("busy_max_ms"), Pass.BusyMaximumMilliseconds);
		PassObject->SetNumberField(TEXT("busy_min_ms"), Pass.BusyMinimumMilliseconds);
		PassObject->SetNumberField(TEXT("wait_avg_ms"), Pass.WaitAverageMilliseconds);
		PassObject->SetNumberField(TEXT("idle_avg_ms"), Pass.IdleAverageMilliseconds);
		PassValues.Add(MakeShared<FJsonValueObject>(PassObject));
	}
	Result.SetBoolField(TEXT("detailed_available"), true);
	Result.SetArrayField(TEXT("passes"), PassValues);
	Result.SetNumberField(TEXT("pass_count"), PassValues.Num());
	Result.SetNumberField(TEXT("total_busy_ms"), Snapshot.TotalBusyMilliseconds);

	FString Summary = FString::Printf(
		TEXT("GPU: %.1fms total, %.1fms busy. Top passes: "),
		Result.GetNumberField(TEXT("total_gpu_ms")),
		Snapshot.TotalBusyMilliseconds);
	for (int32 Index = 0; Index < FMath::Min(5, Snapshot.Passes.Num()); ++Index)
	{
		const FGPUStatsPassSample& Pass = Snapshot.Passes[Index];
		Summary += FString::Printf(TEXT("%s=%.2fms"), *Pass.Name, Pass.BusyAverageMilliseconds);
		if (Index + 1 < FMath::Min(5, Snapshot.Passes.Num()))
		{
			Summary += TEXT(", ");
		}
	}
	Result.SetStringField(TEXT("message"), Summary);
}
}

FGetGPUStatsCommand::FGetGPUStatsCommand()
	: Backend(CreateGPUStatsCaptureBackend())
	, Capture(MakeUnique<FBoundedEditorStatCapture>(Backend, CreateEditorStatCaptureScheduler()))
{
}

FGetGPUStatsCommand::FGetGPUStatsCommand(
	TSharedRef<IGPUStatsCaptureBackend> InBackend,
	TSharedRef<IEditorStatCaptureScheduler> InScheduler)
	: Backend(MoveTemp(InBackend))
	, Capture(MakeUnique<FBoundedEditorStatCapture>(Backend, MoveTemp(InScheduler)))
{
}

FGetGPUStatsCommand::~FGetGPUStatsCommand()
= default;

FString FGetGPUStatsCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> Request;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters.IsEmpty() ? TEXT("{}") : Parameters);
	if (!FJsonSerializer::Deserialize(Reader, Request) || !Request.IsValid())
	{
		const TSharedRef<FJsonObject> Result = MakeBaseResponse(*Backend);
		Result->SetBoolField(TEXT("success"), false);
		Result->SetStringField(TEXT("error"), TEXT("Parameters must be a JSON object"));
		return SerializeResponse(Result);
	}

	FString Action = TEXT("snapshot");
	Request->TryGetStringField(TEXT("action"), Action);
	Action.ToLowerInline();

	const TSharedRef<FJsonObject> Result = MakeBaseResponse(*Backend);
	if (Action == TEXT("snapshot"))
	{
		FGPUStatsSnapshot Snapshot;
		if (Backend->TryReadDetailedSnapshot(Snapshot))
		{
			AddDetailedSnapshot(*Result, Snapshot);
		}
		else
		{
			Result->SetBoolField(TEXT("detailed_available"), false);
			Result->SetArrayField(TEXT("passes"), {});
			Result->SetNumberField(TEXT("pass_count"), 0);
			Result->SetStringField(TEXT("message"), TEXT("GPU frame-time snapshot captured without changing profiler state."));
		}
		Result->SetBoolField(TEXT("success"), true);
		return SerializeResponse(Result);
	}

	if (Action == TEXT("begin"))
	{
		double TimeoutSeconds = 3.0;
		if (Request->HasField(TEXT("timeout_seconds")) && !Request->TryGetNumberField(TEXT("timeout_seconds"), TimeoutSeconds))
		{
			Result->SetBoolField(TEXT("success"), false);
			Result->SetStringField(TEXT("error"), TEXT("timeout_seconds must be numeric"));
			return SerializeResponse(Result);
		}
		if (TimeoutSeconds < 0.1 || TimeoutSeconds > 30.0)
		{
			Result->SetBoolField(TEXT("success"), false);
			Result->SetStringField(TEXT("error"), TEXT("timeout_seconds must be between 0.1 and 30.0"));
			return SerializeResponse(Result);
		}

		FString Owner;
		FString Error;
		if (!Capture->Begin(static_cast<float>(TimeoutSeconds), Owner, Error))
		{
			Result->SetBoolField(TEXT("success"), false);
			Result->SetStringField(TEXT("error"), Error);
			return SerializeResponse(Result);
		}

		Result->SetBoolField(TEXT("success"), true);
		Result->SetBoolField(TEXT("capture_started"), true);
		Result->SetStringField(TEXT("capture_owner"), Owner);
		Result->SetNumberField(TEXT("timeout_seconds"), TimeoutSeconds);
		return SerializeResponse(Result);
	}

	if (Action == TEXT("read"))
	{
		bool bFinish = true;
		Request->TryGetBoolField(TEXT("finish"), bFinish);
		FGPUStatsSnapshot Snapshot;
		const bool bAvailable = Backend->TryReadDetailedSnapshot(Snapshot);
		if (bAvailable)
		{
			AddDetailedSnapshot(*Result, Snapshot);
			Result->SetBoolField(TEXT("success"), true);
		}
		else
		{
			Result->SetBoolField(TEXT("success"), false);
			Result->SetBoolField(TEXT("detailed_available"), false);
			Result->SetStringField(TEXT("error"), TEXT("Detailed GPU data is not ready"));
		}
		if (bFinish)
		{
			Capture->Finish();
		}
		return SerializeResponse(Result);
	}

	if (Action == TEXT("cancel"))
	{
		Capture->Finish();
		Result->SetBoolField(TEXT("success"), true);
		Result->SetBoolField(TEXT("capture_cancelled"), true);
		return SerializeResponse(Result);
	}

	Result->SetBoolField(TEXT("success"), false);
	Result->SetStringField(TEXT("error"), FString::Printf(TEXT("Unknown action '%s'"), *Action));
	return SerializeResponse(Result);
}

FString FGetGPUStatsCommand::GetCommandName() const
{
	return TEXT("get_gpu_stats");
}

bool FGetGPUStatsCommand::ValidateParams(const FString& Parameters) const
{
	return true;
}
