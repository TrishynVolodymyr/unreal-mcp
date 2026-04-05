#include "Commands/Editor/StopPIECommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"

FString FStopPIECommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

	if (!GUnrealEd)
	{
		Result->SetBoolField(TEXT("success"), false);
		Result->SetStringField(TEXT("error"), TEXT("GUnrealEd is null"));
		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Result.ToSharedRef(), W);
		return Out;
	}

	if (!GUnrealEd->IsPlaySessionInProgress())
	{
		Result->SetBoolField(TEXT("success"), true);
		Result->SetBoolField(TEXT("already_stopped"), true);
		Result->SetStringField(TEXT("message"), TEXT("No PIE session is running"));
		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Result.ToSharedRef(), W);
		return Out;
	}

	GUnrealEd->RequestEndPlayMap();

	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("message"), TEXT("PIE session end requested"));

	FString Out;
	TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Result.ToSharedRef(), W);
	return Out;
}

FString FStopPIECommand::GetCommandName() const { return TEXT("stop_pie"); }
bool FStopPIECommand::ValidateParams(const FString& Parameters) const { return true; }
