// NiagaraAssetService.cpp - Core Asset Management (Feature 1)
// CreateSystem, CreateEmitter, AddEmitterToSystem

#include "Services/NiagaraService.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEmitterFactoryNew.h"
#include "NiagaraEditorUtilities.h"
#include "ObjectTools.h"

// ============================================================================
// Core Asset Management (Feature 1)
// ============================================================================

UNiagaraSystem* FNiagaraService::CreateSystem(const FNiagaraSystemCreationParams& Params, FString& OutSystemPath, FString& OutError)
{
    // Validate params
    if (!Params.IsValid(OutError))
    {
        return nullptr;
    }

    // Create package
    UPackage* Package = nullptr;
    if (!CreateAssetPackage(Params.Path, Params.Name, Package, OutError))
    {
        return nullptr;
    }

    // Create the system using the factory
    UNiagaraSystemFactoryNew* Factory = NewObject<UNiagaraSystemFactoryNew>();

    UNiagaraSystem* NewSystem = Cast<UNiagaraSystem>(
        Factory->FactoryCreateNew(
            UNiagaraSystem::StaticClass(),
            Package,
            FName(*Params.Name),
            RF_Public | RF_Standalone,
            nullptr,
            GWarn
        )
    );

    if (!NewSystem)
    {
        OutError = FString::Printf(TEXT("Failed to create Niagara System '%s'"), *Params.Name);
        return nullptr;
    }

    // If template specified, copy from it
    if (!Params.Template.IsEmpty())
    {
        UNiagaraSystem* TemplateSystem = FindSystem(Params.Template);
        if (TemplateSystem)
        {
            // Copy emitters from template
            for (const FNiagaraEmitterHandle& Handle : TemplateSystem->GetEmitterHandles())
            {
                if (Handle.GetInstance().Emitter)
                {
                    FGuid DummyGuid;
                    FString DummyError;
                    AddEmitterToSystem(
                        Package->GetPathName(),
                        Handle.GetInstance().Emitter->GetPathName(),
                        Handle.GetName().ToString(),
                        DummyGuid,
                        DummyError
                    );
                }
            }
        }
        else
        {
            UE_LOG(LogNiagaraService, Warning, TEXT("Template system '%s' not found, creating empty system"), *Params.Template);
        }
    }

    // Save the asset
    if (!SaveAsset(NewSystem, OutError))
    {
        return nullptr;
    }

    OutSystemPath = Package->GetPathName();
    UE_LOG(LogNiagaraService, Log, TEXT("Created Niagara System: %s"), *OutSystemPath);

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewSystem);

    return NewSystem;
}

UNiagaraEmitter* FNiagaraService::CreateEmitter(const FNiagaraEmitterCreationParams& Params, FString& OutEmitterPath, FString& OutError)
{
    // Validate params
    if (!Params.IsValid(OutError))
    {
        return nullptr;
    }

    // Create package
    UPackage* Package = nullptr;
    if (!CreateAssetPackage(Params.Path, Params.Name, Package, OutError))
    {
        return nullptr;
    }

    // Create the emitter using the factory
    UNiagaraEmitterFactoryNew* Factory = NewObject<UNiagaraEmitterFactoryNew>();

    UNiagaraEmitter* NewEmitter = Cast<UNiagaraEmitter>(
        Factory->FactoryCreateNew(
            UNiagaraEmitter::StaticClass(),
            Package,
            FName(*Params.Name),
            RF_Public | RF_Standalone,
            nullptr,
            GWarn
        )
    );

    if (!NewEmitter)
    {
        OutError = FString::Printf(TEXT("Failed to create Niagara Emitter '%s'"), *Params.Name);
        return nullptr;
    }

    // If template specified, we would copy settings here
    // (Template copying for emitters is more complex due to versioning)

    // Save the asset
    if (!SaveAsset(NewEmitter, OutError))
    {
        return nullptr;
    }

    OutEmitterPath = Package->GetPathName();
    UE_LOG(LogNiagaraService, Log, TEXT("Created Niagara Emitter: %s"), *OutEmitterPath);

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewEmitter);

    return NewEmitter;
}

bool FNiagaraService::AddEmitterToSystem(const FString& SystemPath, const FString& EmitterPath, const FString& EmitterName, FGuid& OutEmitterHandleId, FString& OutError)
{
    // Find the system
    UNiagaraSystem* System = FindSystem(SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("System not found: %s"), *SystemPath);
        return false;
    }

    // Find the emitter
    UNiagaraEmitter* Emitter = FindEmitter(EmitterPath);
    if (!Emitter)
    {
        OutError = FString::Printf(TEXT("Emitter not found: %s"), *EmitterPath);
        return false;
    }

    // Get emitter version GUID
    FGuid EmitterVersionGuid = Emitter->GetExposedVersion().VersionGuid;

    // Add emitter to system using editor utilities
    OutEmitterHandleId = FNiagaraEditorUtilities::AddEmitterToSystem(
        *System,
        *Emitter,
        EmitterVersionGuid,
        true  // bCreateCopy
    );

    if (!OutEmitterHandleId.IsValid())
    {
        OutError = TEXT("Failed to add emitter to system - invalid handle returned");
        return false;
    }

    // Set custom name if provided
    if (!EmitterName.IsEmpty())
    {
        // Find the handle and rename it
        for (int32 i = 0; i < System->GetEmitterHandles().Num(); i++)
        {
            FNiagaraEmitterHandle& Handle = System->GetEmitterHandle(i);
            if (Handle.GetId() == OutEmitterHandleId)
            {
                System->Modify();
                Handle.SetName(FName(*EmitterName), *System);
                UE_LOG(LogNiagaraService, Log, TEXT("Renamed emitter handle to '%s'"), *EmitterName);
                break;
            }
        }
    }

    // Mark dirty and refresh
    MarkSystemDirty(System);

    // Broadcast post-edit change to trigger parameter map rebuilding
    // This is what the engine does after adding emitters - fixes ParameterMap traversal errors
    System->OnSystemPostEditChange().Broadcast(System);

    // Request synchronous compilation and wait for it to complete
    System->RequestCompile(false);
    System->WaitForCompilationComplete();

    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Added emitter '%s' to system '%s' with handle ID: %s"),
        *EmitterPath, *SystemPath, *OutEmitterHandleId.ToString());

    return true;
}

bool FNiagaraService::DuplicateSystem(const FString& SourcePath, const FString& NewName, const FString& FolderPath, FString& OutNewPath, FString& OutError)
{
    // Find the source system
    UNiagaraSystem* SourceSystem = FindSystem(SourcePath);
    if (!SourceSystem)
    {
        OutError = FString::Printf(TEXT("Source system not found: %s"), *SourcePath);
        return false;
    }

    // Determine destination folder
    FString DestFolder = FolderPath;
    if (DestFolder.IsEmpty())
    {
        // Use source asset's folder
        DestFolder = FPackageName::GetLongPackagePath(SourceSystem->GetOutermost()->GetName());
    }

    // Ensure path starts with /Game
    if (!DestFolder.StartsWith(TEXT("/Game")))
    {
        DestFolder = TEXT("/Game") / DestFolder;
    }

    // Create the destination package path
    FString DestPackagePath = DestFolder / NewName;

    // Check if destination already exists
    if (FindPackage(nullptr, *DestPackagePath))
    {
        OutError = FString::Printf(TEXT("Asset already exists at path: %s"), *DestPackagePath);
        return false;
    }

    // Use Asset Tools to duplicate with proper name
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    // DuplicateAsset creates a properly named copy
    UObject* DuplicatedObject = AssetTools.DuplicateAsset(NewName, DestFolder, SourceSystem);

    if (!DuplicatedObject)
    {
        OutError = TEXT("Failed to duplicate system");
        return false;
    }

    UNiagaraSystem* NewSystem = Cast<UNiagaraSystem>(DuplicatedObject);
    if (!NewSystem)
    {
        OutError = TEXT("Duplicated object is not a Niagara System");
        return false;
    }

    // Save the new asset
    if (!SaveAsset(NewSystem, OutError))
    {
        return false;
    }

    OutNewPath = NewSystem->GetOutermost()->GetName();

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewSystem);

    UE_LOG(LogNiagaraService, Log, TEXT("Duplicated Niagara System from '%s' to '%s'"), *SourcePath, *OutNewPath);

    return true;
}

bool FNiagaraService::SetEmitterEnabled(const FString& SystemPath, const FString& EmitterName, bool bEnabled, FString& OutError)
{
    // Find the system
    UNiagaraSystem* System = FindSystem(SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("System not found: %s"), *SystemPath);
        return false;
    }

    // Find the emitter handle by name
    int32 EmitterIndex = FindEmitterHandleIndex(System, EmitterName);
    if (EmitterIndex == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("Emitter '%s' not found in system '%s'"), *EmitterName, *SystemPath);
        return false;
    }

    // Get the handle and modify enabled state
    FNiagaraEmitterHandle& Handle = System->GetEmitterHandle(EmitterIndex);

    // Check if already in desired state
    bool bCurrentState = Handle.GetIsEnabled();
    if (bCurrentState == bEnabled)
    {
        UE_LOG(LogNiagaraService, Log, TEXT("Emitter '%s' is already %s"),
            *EmitterName, bEnabled ? TEXT("enabled") : TEXT("disabled"));
        return true;
    }

    // Modify the system
    System->Modify();

    // Set the enabled state
    Handle.SetIsEnabled(bEnabled, *System, true);

    // Mark dirty and recompile
    MarkSystemDirty(System);

    // Request compilation and wait
    System->RequestCompile(false);
    System->WaitForCompilationComplete();

    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set emitter '%s' in system '%s' to %s"),
        *EmitterName, *SystemPath, bEnabled ? TEXT("enabled") : TEXT("disabled"));

    return true;
}

bool FNiagaraService::SetEmitterProperty(const FNiagaraEmitterPropertyParams& Params, FString& OutError)
{
    // Validate params
    if (!Params.IsValid(OutError))
    {
        return false;
    }

    // Find the system
    UNiagaraSystem* System = FindSystem(Params.SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("System not found: %s"), *Params.SystemPath);
        return false;
    }

    // Find the emitter handle by name
    int32 EmitterIndex = FindEmitterHandleIndex(System, Params.EmitterName);
    if (EmitterIndex == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("Emitter '%s' not found in system '%s'"), *Params.EmitterName, *Params.SystemPath);
        return false;
    }

    FNiagaraEmitterHandle& EmitterHandle = System->GetEmitterHandle(EmitterIndex);
    FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
    if (!EmitterData)
    {
        OutError = FString::Printf(TEXT("Could not get emitter data for '%s'"), *Params.EmitterName);
        return false;
    }

    // Modify the system
    System->Modify();

    // Parse property name and set value
    FString PropertyName = Params.PropertyName.ToLower();
    FString Value = Params.PropertyValue;

    if (PropertyName == TEXT("localspace") || PropertyName == TEXT("blocalspace"))
    {
        bool bLocalSpace = Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("1"));
        EmitterData->bLocalSpace = bLocalSpace;
        UE_LOG(LogNiagaraService, Log, TEXT("Set emitter '%s' bLocalSpace to %s"), *Params.EmitterName, bLocalSpace ? TEXT("true") : TEXT("false"));
    }
    else if (PropertyName == TEXT("determinism") || PropertyName == TEXT("bdeterminism"))
    {
        bool bDeterminism = Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("1"));
        EmitterData->bDeterminism = bDeterminism;
        UE_LOG(LogNiagaraService, Log, TEXT("Set emitter '%s' bDeterminism to %s"), *Params.EmitterName, bDeterminism ? TEXT("true") : TEXT("false"));
    }
    else if (PropertyName == TEXT("randomseed"))
    {
        int32 RandomSeed = FCString::Atoi(*Value);
        EmitterData->RandomSeed = RandomSeed;
        UE_LOG(LogNiagaraService, Log, TEXT("Set emitter '%s' RandomSeed to %d"), *Params.EmitterName, RandomSeed);
    }
    else if (PropertyName == TEXT("simtarget") || PropertyName == TEXT("simulationtarget"))
    {
        if (Value.Equals(TEXT("CPU"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("CPUSim"), ESearchCase::IgnoreCase))
        {
            EmitterData->SimTarget = ENiagaraSimTarget::CPUSim;
            UE_LOG(LogNiagaraService, Log, TEXT("Set emitter '%s' SimTarget to CPUSim"), *Params.EmitterName);
        }
        else if (Value.Equals(TEXT("GPU"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("GPUComputeSim"), ESearchCase::IgnoreCase))
        {
            EmitterData->SimTarget = ENiagaraSimTarget::GPUComputeSim;
            UE_LOG(LogNiagaraService, Log, TEXT("Set emitter '%s' SimTarget to GPUComputeSim"), *Params.EmitterName);
        }
        else
        {
            OutError = FString::Printf(TEXT("Invalid SimTarget value '%s'. Valid values: 'CPU', 'GPU'"), *Value);
            return false;
        }
    }
    else if (PropertyName == TEXT("requirespersistentids") || PropertyName == TEXT("brequirespersistentids"))
    {
        bool bRequires = Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("1"));
        EmitterData->bRequiresPersistentIDs = bRequires;
        UE_LOG(LogNiagaraService, Log, TEXT("Set emitter '%s' bRequiresPersistentIDs to %s"), *Params.EmitterName, bRequires ? TEXT("true") : TEXT("false"));
    }
    else if (PropertyName == TEXT("maxgpuparticlesspawnperframe"))
    {
        int32 MaxSpawn = FCString::Atoi(*Value);
        EmitterData->MaxGPUParticlesSpawnPerFrame = MaxSpawn;
        UE_LOG(LogNiagaraService, Log, TEXT("Set emitter '%s' MaxGPUParticlesSpawnPerFrame to %d"), *Params.EmitterName, MaxSpawn);
    }
    else if (PropertyName == TEXT("calculateboundsmode") || PropertyName == TEXT("boundsmode"))
    {
        if (Value.Equals(TEXT("Dynamic"), ESearchCase::IgnoreCase))
        {
            EmitterData->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Dynamic;
            UE_LOG(LogNiagaraService, Log, TEXT("Set emitter '%s' CalculateBoundsMode to Dynamic"), *Params.EmitterName);
        }
        else if (Value.Equals(TEXT("Fixed"), ESearchCase::IgnoreCase))
        {
            EmitterData->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Fixed;
            UE_LOG(LogNiagaraService, Log, TEXT("Set emitter '%s' CalculateBoundsMode to Fixed"), *Params.EmitterName);
        }
        else
        {
            OutError = FString::Printf(TEXT("Invalid CalculateBoundsMode value '%s'. Valid values: 'Dynamic', 'Fixed'"), *Value);
            return false;
        }
    }
    else if (PropertyName == TEXT("fixedbounds"))
    {
        // Parse FBox format: "MinX,MinY,MinZ,MaxX,MaxY,MaxZ"
        TArray<FString> Components;
        Value.ParseIntoArray(Components, TEXT(","), true);

        if (Components.Num() == 6)
        {
            FBox Bounds(
                FVector(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2])),
                FVector(FCString::Atof(*Components[3]), FCString::Atof(*Components[4]), FCString::Atof(*Components[5]))
            );
            EmitterData->FixedBounds = Bounds;
            UE_LOG(LogNiagaraService, Log, TEXT("Set emitter '%s' FixedBounds to Min(%s,%s,%s) Max(%s,%s,%s)"),
                *Params.EmitterName, *Components[0], *Components[1], *Components[2], *Components[3], *Components[4], *Components[5]);
        }
        else
        {
            OutError = FString::Printf(TEXT("Invalid FixedBounds format '%s'. Expected: 'MinX,MinY,MinZ,MaxX,MaxY,MaxZ'"), *Value);
            return false;
        }
    }
    else
    {
        OutError = FString::Printf(TEXT("Unknown emitter property '%s'. Valid properties: LocalSpace, Determinism, RandomSeed, SimTarget, RequiresPersistentIDs, MaxGPUParticlesSpawnPerFrame, CalculateBoundsMode, FixedBounds"), *Params.PropertyName);
        return false;
    }

    // Mark dirty and recompile
    MarkSystemDirty(System);

    // Request compilation and wait
    System->RequestCompile(false);
    System->WaitForCompilationComplete();

    RefreshEditors(System);

    return true;
}

bool FNiagaraService::GetEmitterProperties(const FString& SystemPath, const FString& EmitterName, TSharedPtr<FJsonObject>& OutProperties, FString& OutError)
{
    // Find the system
    UNiagaraSystem* System = FindSystem(SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("System not found: %s"), *SystemPath);
        return false;
    }

    // Find the emitter handle by name
    int32 EmitterIndex = FindEmitterHandleIndex(System, EmitterName);
    if (EmitterIndex == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("Emitter '%s' not found in system '%s'"), *EmitterName, *SystemPath);
        return false;
    }

    const FNiagaraEmitterHandle& EmitterHandle = System->GetEmitterHandle(EmitterIndex);
    FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
    if (!EmitterData)
    {
        OutError = FString::Printf(TEXT("Could not get emitter data for '%s'"), *EmitterName);
        return false;
    }

    // Build the properties JSON
    OutProperties = MakeShared<FJsonObject>();
    OutProperties->SetBoolField(TEXT("success"), true);
    OutProperties->SetStringField(TEXT("emitter_name"), EmitterName);
    OutProperties->SetStringField(TEXT("system_path"), SystemPath);

    // Properties object
    TSharedPtr<FJsonObject> PropsObj = MakeShared<FJsonObject>();

    // Local Space
    PropsObj->SetBoolField(TEXT("LocalSpace"), EmitterData->bLocalSpace);

    // Determinism
    PropsObj->SetBoolField(TEXT("Determinism"), EmitterData->bDeterminism);

    // Random Seed
    PropsObj->SetNumberField(TEXT("RandomSeed"), EmitterData->RandomSeed);

    // Sim Target
    FString SimTargetStr = (EmitterData->SimTarget == ENiagaraSimTarget::GPUComputeSim) ? TEXT("GPU") : TEXT("CPU");
    PropsObj->SetStringField(TEXT("SimTarget"), SimTargetStr);

    // Requires Persistent IDs
    PropsObj->SetBoolField(TEXT("RequiresPersistentIDs"), EmitterData->bRequiresPersistentIDs);

    // Max GPU Particles Spawn Per Frame
    PropsObj->SetNumberField(TEXT("MaxGPUParticlesSpawnPerFrame"), EmitterData->MaxGPUParticlesSpawnPerFrame);

    // Calculate Bounds Mode
    FString BoundsMode;
    switch (EmitterData->CalculateBoundsMode)
    {
        case ENiagaraEmitterCalculateBoundMode::Dynamic: BoundsMode = TEXT("Dynamic"); break;
        case ENiagaraEmitterCalculateBoundMode::Fixed: BoundsMode = TEXT("Fixed"); break;
        default: BoundsMode = TEXT("Unknown"); break;
    }
    PropsObj->SetStringField(TEXT("CalculateBoundsMode"), BoundsMode);

    OutProperties->SetObjectField(TEXT("properties"), PropsObj);

    return true;
}

bool FNiagaraService::RemoveEmitterFromSystem(const FString& SystemPath, const FString& EmitterName, FString& OutError)
{
    // Find the system
    UNiagaraSystem* System = FindSystem(SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("System not found: %s"), *SystemPath);
        return false;
    }

    // Find the emitter handle by name
    int32 EmitterIndex = FindEmitterHandleIndex(System, EmitterName);
    if (EmitterIndex == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("Emitter '%s' not found in system '%s'"), *EmitterName, *SystemPath);
        return false;
    }

    // Get the emitter handle to remove
    const FNiagaraEmitterHandle& Handle = System->GetEmitterHandle(EmitterIndex);

    // Modify the system
    System->Modify();

    // Remove the emitter handle
    System->RemoveEmitterHandle(Handle);

    // Mark dirty and recompile
    MarkSystemDirty(System);

    // Broadcast post-edit change to trigger parameter map rebuilding
    System->OnSystemPostEditChange().Broadcast(System);

    // Request compilation and wait
    System->RequestCompile(false);
    System->WaitForCompilationComplete();

    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Removed emitter '%s' from system '%s'"),
        *EmitterName, *SystemPath);

    return true;
}
