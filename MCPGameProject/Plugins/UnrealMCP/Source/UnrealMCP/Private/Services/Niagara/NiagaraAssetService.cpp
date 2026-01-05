// NiagaraAssetService.cpp - Core Asset Management (Feature 1)
// CreateSystem, CreateEmitter, AddEmitterToSystem

#include "Services/NiagaraService.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEmitterFactoryNew.h"
#include "NiagaraEditorUtilities.h"

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
