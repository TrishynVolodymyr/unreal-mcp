#include "Commands/Editor/SetLevelWorldSettingsCommand.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "LevelEditorSubsystem.h"

namespace
{
	FString MakeWorldSettingsErr(const FString& Msg)
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetBoolField(TEXT("success"), false);
		Obj->SetStringField(TEXT("error"), Msg);
		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Obj.ToSharedRef(), W);
		return Out;
	}

	UClass* ResolveGameModeClass(const FString& Path)
	{
		if (Path.IsEmpty()) return nullptr;

		// Try direct class load (e.g. /Script/Module.ClassName)
		if (Path.StartsWith(TEXT("/Script/")))
		{
			if (UClass* C = LoadClass<AGameModeBase>(nullptr, *Path)) return C;
			// Some sources may pass /Script/Module.ClassName without _C — also try LoadObject
			if (UClass* C2 = LoadObject<UClass>(nullptr, *Path)) return C2;
			return nullptr;
		}

		// Blueprint path — append _C if missing for class load
		FString ClassPath = Path;
		if (!ClassPath.EndsWith(TEXT("_C")))
		{
			// Convert /Game/Path/BP_X to /Game/Path/BP_X.BP_X_C
			FString PackageName;
			FString ObjectName = Path;
			int32 DotIdx;
			if (Path.FindChar('.', DotIdx))
			{
				PackageName = Path.Left(DotIdx);
				ObjectName = Path.RightChop(DotIdx + 1);
			}
			else
			{
				int32 SlashIdx;
				if (Path.FindLastChar('/', SlashIdx))
				{
					PackageName = Path;
					ObjectName = Path.RightChop(SlashIdx + 1);
				}
			}
			ClassPath = FString::Printf(TEXT("%s.%s_C"), *PackageName, *ObjectName);
		}
		return LoadClass<AGameModeBase>(nullptr, *ClassPath);
	}
}

FString FSetLevelWorldSettingsCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return MakeWorldSettingsErr(TEXT("Invalid JSON parameters"));
	}

	FString LevelPath;
	if (!JsonObject->TryGetStringField(TEXT("level_path"), LevelPath) || LevelPath.IsEmpty())
	{
		return MakeWorldSettingsErr(TEXT("Missing 'level_path' parameter"));
	}

	FString GameModePath;
	const bool bHasGameMode = JsonObject->TryGetStringField(TEXT("game_mode"), GameModePath);

	if (!GEditor)
	{
		return MakeWorldSettingsErr(TEXT("GEditor is null — command requires editor context"));
	}

	ULevelEditorSubsystem* LES = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	if (!LES)
	{
		return MakeWorldSettingsErr(TEXT("ULevelEditorSubsystem unavailable"));
	}

	if (!LES->LoadLevel(LevelPath))
	{
		return MakeWorldSettingsErr(FString::Printf(TEXT("Failed to load level at %s"), *LevelPath));
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return MakeWorldSettingsErr(TEXT("Editor world is null after LoadLevel"));
	}

	AWorldSettings* WS = World->GetWorldSettings();
	if (!WS)
	{
		return MakeWorldSettingsErr(TEXT("WorldSettings is null"));
	}

	bool bChanged = false;
	FString ResolvedGameModePath;

	if (bHasGameMode)
	{
		if (GameModePath.IsEmpty())
		{
			WS->DefaultGameMode = nullptr;
			bChanged = true;
		}
		else
		{
			UClass* GMClass = ResolveGameModeClass(GameModePath);
			if (!GMClass)
			{
				return MakeWorldSettingsErr(FString::Printf(
					TEXT("Could not resolve GameMode class from path: %s"), *GameModePath));
			}
			WS->DefaultGameMode = GMClass;
			ResolvedGameModePath = GMClass->GetPathName();
			bChanged = true;
		}
	}

	if (bChanged)
	{
		WS->MarkPackageDirty();
		if (!LES->SaveCurrentLevel())
		{
			UE_LOG(LogTemp, Warning, TEXT("SetLevelWorldSettings: SaveCurrentLevel returned false for %s"),
				*LevelPath);
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("level_path"), LevelPath);
	Result->SetStringField(TEXT("game_mode_resolved"), ResolvedGameModePath);
	Result->SetBoolField(TEXT("changed"), bChanged);

	FString OutString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutString);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return OutString;
}

FString FSetLevelWorldSettingsCommand::GetCommandName() const
{
	return TEXT("set_level_world_settings");
}

bool FSetLevelWorldSettingsCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid()) return false;

	FString LevelPath;
	return JsonObject->TryGetStringField(TEXT("level_path"), LevelPath) && !LevelPath.IsEmpty();
}
