#include "Commands/Editor/SetLevelWorldSettingsCommand.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "FileHelpers.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FString MakeErr(const FString& Msg)
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
		return MakeErr(TEXT("Invalid JSON parameters"));
	}

	FString LevelPath;
	if (!JsonObject->TryGetStringField(TEXT("level_path"), LevelPath) || LevelPath.IsEmpty())
	{
		return MakeErr(TEXT("Missing 'level_path' parameter"));
	}

	FString GameModePath;
	const bool bHasGameMode = JsonObject->TryGetStringField(TEXT("game_mode"), GameModePath);

	if (!GEditor)
	{
		return MakeErr(TEXT("GEditor is null — command requires editor context"));
	}

	// Resolve the World object path "/Game/Path/Name.Name" from the package path.
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

	// Load the World OBJECT without making it the active editor world.
	// We deliberately DO NOT use ULevelEditorSubsystem::LoadLevel: that performs a full
	// editor map-switch, and on a malformed / World-Partition map the switch can trip the
	// engine's map-load leak check (Fatal EditorServer.cpp:2524 "World Memory Leaks") and
	// kill the editor. Editing WorldSettings does not require switching the active world.
	// A missing/corrupt map returns null here, which we handle gracefully instead of crashing.
	UWorld* World = LoadObject<UWorld>(nullptr, *ObjectPath);
	if (!World)
	{
		return MakeErr(FString::Printf(
			TEXT("Could not load a World at '%s' (missing or malformed map — e.g. a corrupt "
				 "duplicate). Aborting safely without switching the editor world."), *LevelPath));
	}

	AWorldSettings* WS = World->GetWorldSettings(/*bCheckStreamingPersistent*/ false, /*bChecked*/ false);
	if (!WS)
	{
		return MakeErr(TEXT("WorldSettings is null on the loaded world"));
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
				return MakeErr(FString::Printf(
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
		// Save the map package directly — no editor world switch (avoids the map-load
		// leak-check fatal). SaveMap handles map (.umap) packages correctly, unlike
		// UEditorAssetLibrary::SaveAsset which fails on maps.
		if (!UEditorLoadingAndSavingUtils::SaveMap(World, LevelPath))
		{
			UE_LOG(LogTemp, Warning, TEXT("SetLevelWorldSettings: SaveMap returned false for %s"),
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
