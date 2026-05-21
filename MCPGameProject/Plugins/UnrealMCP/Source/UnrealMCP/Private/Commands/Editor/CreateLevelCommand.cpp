#include "Commands/Editor/CreateLevelCommand.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Editor.h"
#include "LevelEditorSubsystem.h"

namespace
{
	FString MakeErr_CreateLevel(const FString& Msg)
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetBoolField(TEXT("success"), false);
		Obj->SetStringField(TEXT("error"), Msg);
		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Obj.ToSharedRef(), W);
		return Out;
	}
}

FString FCreateLevelCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return MakeErr_CreateLevel(TEXT("Invalid JSON parameters"));
	}

	FString LevelName;
	FString SavePath;
	FString Template;
	JsonObject->TryGetStringField(TEXT("template"), Template);

	if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) || LevelName.IsEmpty())
	{
		return MakeErr_CreateLevel(TEXT("Missing 'level_name' parameter"));
	}
	if (!JsonObject->TryGetStringField(TEXT("save_path"), SavePath) || SavePath.IsEmpty())
	{
		return MakeErr_CreateLevel(TEXT("Missing 'save_path' parameter"));
	}

	// Normalize: ensure save_path starts with /Game and does not end with /
	if (!SavePath.StartsWith(TEXT("/Game")))
	{
		return MakeErr_CreateLevel(TEXT("'save_path' must start with /Game"));
	}
	SavePath.RemoveFromEnd(TEXT("/"));
	const FString FullPath = FString::Printf(TEXT("%s/%s"), *SavePath, *LevelName);

	if (!GEditor)
	{
		return MakeErr_CreateLevel(TEXT("GEditor is null - command requires editor context"));
	}

	ULevelEditorSubsystem* LES = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	if (!LES)
	{
		return MakeErr_CreateLevel(TEXT("ULevelEditorSubsystem unavailable"));
	}

	bool bOk = false;
	if (Template.IsEmpty())
	{
		bOk = LES->NewLevel(FullPath);
	}
	else
	{
		bOk = LES->NewLevelFromTemplate(FullPath, Template);
	}

	if (!bOk)
	{
		return MakeErr_CreateLevel(FString::Printf(TEXT("Failed to create level at %s (template='%s')"),
			*FullPath, *Template));
	}

	// Persist the newly created level
	const bool bSaved = LES->SaveCurrentLevel();
	if (!bSaved)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateLevel: level created but SaveCurrentLevel returned false for %s"),
			*FullPath);
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("level_path"), FullPath);
	Result->SetStringField(TEXT("template"), Template);
	Result->SetBoolField(TEXT("saved"), bSaved);

	FString OutString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutString);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return OutString;
}

FString FCreateLevelCommand::GetCommandName() const
{
	return TEXT("create_level");
}

bool FCreateLevelCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid()) return false;

	FString LevelName, SavePath;
	return JsonObject->TryGetStringField(TEXT("level_name"), LevelName) && !LevelName.IsEmpty()
		&& JsonObject->TryGetStringField(TEXT("save_path"), SavePath) && !SavePath.IsEmpty();
}
