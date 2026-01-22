#include "Services/SoundService.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Sound/AmbientSound.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Editor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Dom/JsonValue.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetImportTask.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

DEFINE_LOG_CATEGORY(LogSoundService);

// Singleton instance
TUniquePtr<FSoundService> FSoundService::Instance;

FSoundService::FSoundService()
{
    UE_LOG(LogSoundService, Log, TEXT("SoundService initialized"));
}

FSoundService& FSoundService::Get()
{
    if (!Instance.IsValid())
    {
        Instance = MakeUnique<FSoundService>();
    }
    return *Instance;
}

// ============================================================================
// Sound Wave Operations
// ============================================================================

bool FSoundService::ImportSoundFile(const FSoundWaveImportParams& Params, FString& OutAssetPath, FString& OutError)
{
    // Validate parameters
    if (!Params.IsValid(OutError))
    {
        return false;
    }

    // Check if source file exists
    if (!FPaths::FileExists(Params.SourceFilePath))
    {
        OutError = FString::Printf(TEXT("Source file does not exist: %s"), *Params.SourceFilePath);
        return false;
    }

    // Get the file extension to validate it's an audio file
    FString Extension = FPaths::GetExtension(Params.SourceFilePath).ToLower();
    TArray<FString> SupportedExtensions = { TEXT("wav"), TEXT("mp3"), TEXT("ogg"), TEXT("flac"), TEXT("aiff"), TEXT("aif") };
    if (!SupportedExtensions.Contains(Extension))
    {
        OutError = FString::Printf(TEXT("Unsupported audio format: %s. Supported: wav, mp3, ogg, flac, aiff"), *Extension);
        return false;
    }

    // Get the asset tools module
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = AssetToolsModule.Get();

    // Prepare the destination path
    FString DestinationPath = Params.FolderPath;
    if (!DestinationPath.StartsWith(TEXT("/")))
    {
        DestinationPath = TEXT("/Game/") + DestinationPath;
    }
    else if (!DestinationPath.StartsWith(TEXT("/Game")))
    {
        // Make sure the path is under /Game
        if (DestinationPath.StartsWith(TEXT("/")))
        {
            DestinationPath = TEXT("/Game") + DestinationPath;
        }
    }

    // Create the import task
    UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
    ImportTask->Filename = Params.SourceFilePath;
    ImportTask->DestinationPath = DestinationPath;
    ImportTask->DestinationName = Params.AssetName;
    ImportTask->bReplaceExisting = true;
    ImportTask->bAutomated = true;
    ImportTask->bSave = true;

    // Execute the import
    TArray<UAssetImportTask*> ImportTasks;
    ImportTasks.Add(ImportTask);
    AssetTools.ImportAssetTasks(ImportTasks);

    // Check if import was successful
    if (ImportTask->ImportedObjectPaths.Num() == 0)
    {
        OutError = FString::Printf(TEXT("Failed to import audio file: %s"), *Params.SourceFilePath);
        return false;
    }

    // Get the imported asset path
    OutAssetPath = ImportTask->ImportedObjectPaths[0];

    // Verify the asset was imported as a SoundWave
    USoundWave* ImportedSound = FindSoundWave(OutAssetPath);
    if (!ImportedSound)
    {
        OutError = FString::Printf(TEXT("Import succeeded but asset is not a SoundWave: %s"), *OutAssetPath);
        return false;
    }

    UE_LOG(LogSoundService, Log, TEXT("Imported audio file '%s' as SoundWave: %s (Duration: %.2fs, Channels: %d)"),
        *Params.SourceFilePath, *OutAssetPath, ImportedSound->Duration, ImportedSound->NumChannels);

    return true;
}

bool FSoundService::GetSoundWaveMetadata(const FString& SoundWavePath, TSharedPtr<FJsonObject>& OutMetadata, FString& OutError)
{
    USoundWave* SoundWave = FindSoundWave(SoundWavePath);
    if (!SoundWave)
    {
        OutError = FString::Printf(TEXT("Sound wave not found: %s"), *SoundWavePath);
        return false;
    }

    OutMetadata = MakeShared<FJsonObject>();
    OutMetadata->SetStringField(TEXT("name"), SoundWave->GetName());
    OutMetadata->SetStringField(TEXT("path"), SoundWavePath);
    OutMetadata->SetNumberField(TEXT("duration"), SoundWave->Duration);
    OutMetadata->SetNumberField(TEXT("sample_rate"), SoundWave->GetSampleRateForCurrentPlatform());
    OutMetadata->SetNumberField(TEXT("num_channels"), SoundWave->NumChannels);
    OutMetadata->SetBoolField(TEXT("is_looping"), SoundWave->bLooping);
    OutMetadata->SetNumberField(TEXT("volume"), SoundWave->Volume);
    OutMetadata->SetNumberField(TEXT("pitch"), SoundWave->Pitch);

    // Compression and streaming info
    OutMetadata->SetBoolField(TEXT("is_streaming"), SoundWave->IsStreaming());

    UE_LOG(LogSoundService, Log, TEXT("Retrieved metadata for sound wave: %s (Duration: %.2fs, Channels: %d)"),
        *SoundWavePath, SoundWave->Duration, SoundWave->NumChannels);

    return true;
}

bool FSoundService::SetSoundWaveProperties(const FString& SoundWavePath, bool bLooping, float Volume, float Pitch, FString& OutError)
{
    USoundWave* SoundWave = FindSoundWave(SoundWavePath);
    if (!SoundWave)
    {
        OutError = FString::Printf(TEXT("Sound wave not found: %s"), *SoundWavePath);
        return false;
    }

    SoundWave->Modify();
    SoundWave->bLooping = bLooping;
    SoundWave->Volume = FMath::Clamp(Volume, 0.0f, 4.0f);
    SoundWave->Pitch = FMath::Clamp(Pitch, 0.1f, 4.0f);

    if (!SaveAsset(SoundWave, OutError))
    {
        return false;
    }

    UE_LOG(LogSoundService, Log, TEXT("Set properties on sound wave: %s (Looping: %d, Volume: %.2f, Pitch: %.2f)"),
        *SoundWavePath, bLooping, Volume, Pitch);

    return true;
}

// ============================================================================
// Audio Component Operations
// ============================================================================

AAmbientSound* FSoundService::SpawnAmbientSound(const FAmbientSoundSpawnParams& Params, FString& OutActorName, FString& OutError)
{
    // Get the editor world
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        OutError = TEXT("No editor world available");
        return nullptr;
    }

    // Load the sound asset
    USoundBase* Sound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *Params.SoundPath));
    if (!Sound)
    {
        OutError = FString::Printf(TEXT("Failed to load sound: %s"), *Params.SoundPath);
        return nullptr;
    }

    // Spawn the ambient sound actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = FName(*Params.ActorName);
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AAmbientSound* AmbientSound = World->SpawnActor<AAmbientSound>(
        AAmbientSound::StaticClass(),
        Params.Location,
        Params.Rotation,
        SpawnParams
    );

    if (!AmbientSound)
    {
        OutError = TEXT("Failed to spawn ambient sound actor");
        return nullptr;
    }

    // Set the sound
    UAudioComponent* AudioComp = AmbientSound->GetAudioComponent();
    if (AudioComp)
    {
        AudioComp->SetSound(Sound);
        AudioComp->bAutoActivate = Params.bAutoActivate;

        // Set attenuation if provided
        if (!Params.AttenuationPath.IsEmpty())
        {
            USoundAttenuation* Attenuation = Cast<USoundAttenuation>(
                StaticLoadObject(USoundAttenuation::StaticClass(), nullptr, *Params.AttenuationPath)
            );
            if (Attenuation)
            {
                AudioComp->AttenuationSettings = Attenuation;
            }
        }
    }

    // Set the actor label
    AmbientSound->SetActorLabel(Params.ActorName);
    OutActorName = Params.ActorName;

    UE_LOG(LogSoundService, Log, TEXT("Spawned ambient sound: %s at (%.2f, %.2f, %.2f)"),
        *Params.ActorName, Params.Location.X, Params.Location.Y, Params.Location.Z);

    return AmbientSound;
}

bool FSoundService::PlaySoundAtLocation(const FString& SoundPath, const FVector& Location, float VolumeMultiplier, float PitchMultiplier, FString& OutError)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        OutError = TEXT("No editor world available");
        return false;
    }

    USoundBase* Sound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *SoundPath));
    if (!Sound)
    {
        OutError = FString::Printf(TEXT("Failed to load sound: %s"), *SoundPath);
        return false;
    }

    UGameplayStatics::PlaySoundAtLocation(
        World,
        Sound,
        Location,
        FRotator::ZeroRotator,
        VolumeMultiplier,
        PitchMultiplier
    );

    UE_LOG(LogSoundService, Log, TEXT("Playing sound at location: %s at (%.2f, %.2f, %.2f)"),
        *SoundPath, Location.X, Location.Y, Location.Z);

    return true;
}

// ============================================================================
// Sound Attenuation Operations
// ============================================================================

USoundAttenuation* FSoundService::CreateSoundAttenuation(const FSoundAttenuationParams& Params, FString& OutAssetPath, FString& OutError)
{
    UPackage* Package = nullptr;
    if (!CreateAssetPackage(Params.FolderPath, Params.AssetName, Package, OutError))
    {
        return nullptr;
    }

    USoundAttenuation* Attenuation = NewObject<USoundAttenuation>(Package, FName(*Params.AssetName), RF_Public | RF_Standalone);
    if (!Attenuation)
    {
        OutError = TEXT("Failed to create Sound Attenuation object");
        return nullptr;
    }

    // Set attenuation properties
    FSoundAttenuationSettings& Settings = Attenuation->Attenuation;

    // Distance settings
    Settings.DistanceAlgorithm = static_cast<EAttenuationDistanceModel>(GetAttenuationFunction(Params.AttenuationFunction));
    Settings.AttenuationShape = Params.bSpatialize ? EAttenuationShape::Sphere : EAttenuationShape::Capsule;
    Settings.FalloffDistance = Params.FalloffDistance;

    // Radius settings
    Settings.AttenuationShapeExtents = FVector(Params.InnerRadius, 0.0f, 0.0f);

    // Spatialization
    Settings.bSpatialize = Params.bSpatialize;

    if (!SaveAsset(Attenuation, OutError))
    {
        return nullptr;
    }

    OutAssetPath = Package->GetPathName();
    UE_LOG(LogSoundService, Log, TEXT("Created Sound Attenuation: %s"), *OutAssetPath);

    return Attenuation;
}

bool FSoundService::SetAttenuationProperty(const FString& AttenuationPath, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError)
{
    USoundAttenuation* Attenuation = Cast<USoundAttenuation>(
        StaticLoadObject(USoundAttenuation::StaticClass(), nullptr, *AttenuationPath)
    );

    if (!Attenuation)
    {
        OutError = FString::Printf(TEXT("Sound Attenuation not found: %s"), *AttenuationPath);
        return false;
    }

    Attenuation->Modify();
    FSoundAttenuationSettings& Settings = Attenuation->Attenuation;

    FString PropLower = PropertyName.ToLower();

    if (PropLower == TEXT("falloffdistance") || PropLower == TEXT("falloff_distance"))
    {
        Settings.FalloffDistance = static_cast<float>(PropertyValue->AsNumber());
    }
    else if (PropLower == TEXT("innerradius") || PropLower == TEXT("inner_radius"))
    {
        Settings.AttenuationShapeExtents.X = static_cast<float>(PropertyValue->AsNumber());
    }
    else if (PropLower == TEXT("spatialize") || PropLower == TEXT("bspatialize"))
    {
        Settings.bSpatialize = PropertyValue->AsBool();
    }
    else if (PropLower == TEXT("distancealgorithm") || PropLower == TEXT("distance_algorithm"))
    {
        Settings.DistanceAlgorithm = static_cast<EAttenuationDistanceModel>(GetAttenuationFunction(PropertyValue->AsString()));
    }
    else
    {
        OutError = FString::Printf(TEXT("Unknown attenuation property: %s"), *PropertyName);
        return false;
    }

    if (!SaveAsset(Attenuation, OutError))
    {
        return false;
    }

    UE_LOG(LogSoundService, Log, TEXT("Set property '%s' on Sound Attenuation: %s"), *PropertyName, *AttenuationPath);
    return true;
}

// ============================================================================
// Sound Class/Mix Operations (Stubs)
// ============================================================================

USoundClass* FSoundService::CreateSoundClass(const FSoundClassParams& Params, FString& OutAssetPath, FString& OutError)
{
    // TODO: Implement in Phase 4
    OutError = TEXT("Sound Class creation not yet implemented");
    return nullptr;
}

USoundMix* FSoundService::CreateSoundMix(const FSoundMixParams& Params, FString& OutAssetPath, FString& OutError)
{
    // TODO: Implement in Phase 4
    OutError = TEXT("Sound Mix creation not yet implemented");
    return nullptr;
}

bool FSoundService::AddSoundMixModifier(const FString& SoundMixPath, const FString& SoundClassPath, float VolumeAdjust, float PitchAdjust, FString& OutError)
{
    // TODO: Implement in Phase 4
    OutError = TEXT("Sound Mix modifier not yet implemented");
    return false;
}

// ============================================================================
// Utility Methods
// ============================================================================

USoundWave* FSoundService::FindSoundWave(const FString& SoundWavePath)
{
    return Cast<USoundWave>(StaticLoadObject(USoundWave::StaticClass(), nullptr, *SoundWavePath));
}

USoundCue* FSoundService::FindSoundCue(const FString& SoundCuePath)
{
    return Cast<USoundCue>(StaticLoadObject(USoundCue::StaticClass(), nullptr, *SoundCuePath));
}

// ============================================================================
// Internal Helper Methods
// ============================================================================

bool FSoundService::CreateAssetPackage(const FString& Path, const FString& Name, UPackage*& OutPackage, FString& OutError) const
{
    FString PackagePath = Path / Name;
    if (!PackagePath.StartsWith(TEXT("/")))
    {
        PackagePath = TEXT("/Game/") + PackagePath;
    }

    OutPackage = CreatePackage(*PackagePath);
    if (!OutPackage)
    {
        OutError = FString::Printf(TEXT("Failed to create package: %s"), *PackagePath);
        return false;
    }

    OutPackage->FullyLoad();
    return true;
}

bool FSoundService::SaveAsset(UObject* Asset, FString& OutError) const
{
    if (!Asset)
    {
        OutError = TEXT("Cannot save null asset");
        return false;
    }

    UPackage* Package = Asset->GetOutermost();
    if (!Package)
    {
        OutError = TEXT("Asset has no package");
        return false;
    }

    Package->MarkPackageDirty();

    FString PackageFileName = FPackageName::LongPackageNameToFilename(
        Package->GetPathName(),
        FPackageName::GetAssetPackageExtension()
    );

    // Ensure the directory exists
    FString PackageDir = FPaths::GetPath(PackageFileName);
    if (!IFileManager::Get().DirectoryExists(*PackageDir))
    {
        if (!IFileManager::Get().MakeDirectory(*PackageDir, true))
        {
            OutError = FString::Printf(TEXT("Failed to create directory: %s"), *PackageDir);
            return false;
        }
    }

    // Check if file is writable (not read-only)
    if (IFileManager::Get().FileExists(*PackageFileName))
    {
        if (IFileManager::Get().IsReadOnly(*PackageFileName))
        {
            OutError = FString::Printf(TEXT("File is read-only: %s"), *PackageFileName);
            return false;
        }
    }

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    // Don't suppress errors so we can diagnose issues
    SaveArgs.SaveFlags = SAVE_None;
    SaveArgs.Error = GError;

    FSavePackageResultStruct SaveResult = UPackage::Save(Package, Asset, *PackageFileName, SaveArgs);
    if (SaveResult.Result != ESavePackageResult::Success)
    {
        FString ResultStr;
        switch (SaveResult.Result)
        {
            case ESavePackageResult::Canceled: ResultStr = TEXT("Save was canceled"); break;
            case ESavePackageResult::Error: ResultStr = TEXT("Save error occurred"); break;
            case ESavePackageResult::MissingFile: ResultStr = TEXT("Missing file"); break;
            case ESavePackageResult::ReplaceCompletely: ResultStr = TEXT("Replace completely failed"); break;
            case ESavePackageResult::ContainsEditorOnlyData: ResultStr = TEXT("Contains editor-only data"); break;
            case ESavePackageResult::ReferencedOnlyByEditorOnlyData: ResultStr = TEXT("Referenced only by editor-only data"); break;
            case ESavePackageResult::DifferentContent: ResultStr = TEXT("Different content"); break;
            default: ResultStr = FString::Printf(TEXT("Unknown error code: %d"), static_cast<int32>(SaveResult.Result)); break;
        }
        OutError = FString::Printf(TEXT("Failed to save package '%s': %s"), *PackageFileName, *ResultStr);
        return false;
    }

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(Asset);

    return true;
}

uint8 FSoundService::GetAttenuationFunction(const FString& FunctionName) const
{
    if (FunctionName.Equals(TEXT("Logarithmic"), ESearchCase::IgnoreCase))
    {
        return static_cast<uint8>(EAttenuationDistanceModel::Logarithmic);
    }
    else if (FunctionName.Equals(TEXT("Inverse"), ESearchCase::IgnoreCase))
    {
        return static_cast<uint8>(EAttenuationDistanceModel::Inverse);
    }
    else if (FunctionName.Equals(TEXT("LogReverse"), ESearchCase::IgnoreCase))
    {
        return static_cast<uint8>(EAttenuationDistanceModel::LogReverse);
    }
    else if (FunctionName.Equals(TEXT("NaturalSound"), ESearchCase::IgnoreCase))
    {
        return static_cast<uint8>(EAttenuationDistanceModel::NaturalSound);
    }
    // Default to Linear
    return static_cast<uint8>(EAttenuationDistanceModel::Linear);
}
