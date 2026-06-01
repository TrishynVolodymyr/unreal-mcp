#include "Commands/Editor/OpenLevelCommand.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "LevelEditorSubsystem.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FString SerializeJson_OpenLevel(const TSharedPtr<FJsonObject>& Obj)
	{
		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Obj.ToSharedRef(), W);
		return Out;
	}

	FString MakeErr_OpenLevel(const FString& Msg)
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetBoolField(TEXT("success"), false);
		Obj->SetStringField(TEXT("error"), Msg);
		return SerializeJson_OpenLevel(Obj);
	}
}

FString FOpenLevelCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return MakeErr_OpenLevel(TEXT("Invalid JSON parameters"));
	}

	FString LevelPath;
	if (!JsonObject->TryGetStringField(TEXT("level_path"), LevelPath) || LevelPath.IsEmpty())
	{
		return MakeErr_OpenLevel(TEXT("Missing 'level_path' parameter"));
	}
	if (!LevelPath.StartsWith(TEXT("/Game")))
	{
		return MakeErr_OpenLevel(TEXT("'level_path' must start with /Game"));
	}

	if (!GEditor)
	{
		return MakeErr_OpenLevel(TEXT("GEditor is null — command requires editor context"));
	}

	ULevelEditorSubsystem* LES = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	if (!LES)
	{
		return MakeErr_OpenLevel(TEXT("ULevelEditorSubsystem unavailable"));
	}

	// Resolve "/Game/Path/Name" -> object path "/Game/Path/Name.Name" (the World object).
	FString ObjectPath = LevelPath;
	{
		int32 DotIdx;
		if (!ObjectPath.FindChar('.', DotIdx))
		{
			int32 SlashIdx;
			if (LevelPath.FindLastChar('/', SlashIdx))
			{
				ObjectPath = LevelPath + TEXT(".") + LevelPath.RightChop(SlashIdx + 1);
			}
		}
	}

	// Load the World as an OBJECT first — this does NOT switch the active editor world, so it
	// is safe even for WP / malformed maps (mirrors FSetLevelWorldSettingsCommand). We only
	// switch the active world (the leak-check-fatal path) AFTER confirming it is non-WP.
	UWorld* World = LoadObject<UWorld>(nullptr, *ObjectPath);
	if (!World)
	{
		return MakeErr_OpenLevel(FString::Printf(
			TEXT("Could not load a World at '%s' (missing or malformed map). Not switching the editor world."),
			*LevelPath));
	}

	// World Partition detection via the loaded WorldSettings (same accessor path the existing
	// set_level_world_settings command uses). A WP target is refused: switching the active
	// editor world into it can trip the engine map-load leak-check fatal
	// (EditorServer.cpp "World Memory Leaks") — see game-design/tech/voxel.md anti-pattern.
	const AWorldSettings* WS = World->GetWorldSettings(/*bCheckStreamingPersistent*/ false, /*bChecked*/ false);
	const bool bIsPartitioned = (WS != nullptr && WS->GetWorldPartition() != nullptr);
	if (bIsPartitioned)
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetBoolField(TEXT("success"), false);
		Obj->SetBoolField(TEXT("was_world_partition"), true);
		Obj->SetStringField(TEXT("level_path"), LevelPath);
		Obj->SetStringField(TEXT("error"), FString::Printf(
			TEXT("'%s' is a World Partition level. Refusing to switch the editor world into it via MCP — "
				 "that can trigger the map-load leak-check fatal (EditorServer.cpp 'World Memory Leaks'). "
				 "Open WP maps through the editor UI (File > Open Level)."), *LevelPath));
		return SerializeJson_OpenLevel(Obj);
	}

	// Non-WP: safe to switch the active editor world.
	const bool bOk = LES->LoadLevel(LevelPath);
	if (!bOk)
	{
		return MakeErr_OpenLevel(FString::Printf(TEXT("LoadLevel failed for '%s'."), *LevelPath));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("level_path"), LevelPath);
	Result->SetBoolField(TEXT("was_world_partition"), false);
	Result->SetStringField(TEXT("message"), TEXT("Level opened as the active editor world"));
	return SerializeJson_OpenLevel(Result);
}

FString FOpenLevelCommand::GetCommandName() const
{
	return TEXT("open_level");
}

bool FOpenLevelCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid()) return false;

	FString LevelPath;
	return JsonObject->TryGetStringField(TEXT("level_path"), LevelPath) && !LevelPath.IsEmpty();
}
