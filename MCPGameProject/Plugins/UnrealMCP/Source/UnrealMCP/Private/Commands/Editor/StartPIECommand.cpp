#include "Commands/Editor/StartPIECommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "LevelEditor.h"
#include "IAssetViewport.h"
#include "PlayInEditorDataTypes.h"

FString FStartPIECommand::Execute(const FString& Parameters)
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

	if (GUnrealEd->IsPlaySessionInProgress())
	{
		Result->SetBoolField(TEXT("success"), true);
		Result->SetBoolField(TEXT("already_running"), true);
		Result->SetStringField(TEXT("message"), TEXT("PIE session already in progress"));
		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Result.ToSharedRef(), W);
		return Out;
	}

	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<IAssetViewport> ActiveViewport = LevelEditorModule.GetFirstActiveViewport();

	if (!ActiveViewport.IsValid())
	{
		Result->SetBoolField(TEXT("success"), false);
		Result->SetStringField(TEXT("error"), TEXT("No active level viewport found"));
		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Result.ToSharedRef(), W);
		return Out;
	}

	FRequestPlaySessionParams Params;
	Params.WorldType = EPlaySessionWorldType::PlayInEditor;
	Params.DestinationSlateViewport = ActiveViewport;

	GUnrealEd->RequestPlaySession(Params);

	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("message"), TEXT("PIE session requested (starts next tick)"));

	FString Out;
	TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Result.ToSharedRef(), W);
	return Out;
}

FString FStartPIECommand::GetCommandName() const { return TEXT("start_pie"); }
bool FStartPIECommand::ValidateParams(const FString& Parameters) const { return true; }
