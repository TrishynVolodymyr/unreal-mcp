// NiagaraServiceHelpers.cpp - Utility and Internal Helper Methods
// FindSystem, FindEmitter, RefreshEditors, GetScriptUsageFromStage, etc.

#include "Services/NiagaraService.h"

#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/SavePackage.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraDataInterface.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraLightRendererProperties.h"
#include "NiagaraComponentRendererProperties.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraScriptSource.h"

// ============================================================================
// Utility Methods
// ============================================================================

UNiagaraSystem* FNiagaraService::FindSystem(const FString& SystemPath)
{
    return LoadObject<UNiagaraSystem>(nullptr, *SystemPath);
}

UNiagaraEmitter* FNiagaraService::FindEmitter(const FString& EmitterPath)
{
    return LoadObject<UNiagaraEmitter>(nullptr, *EmitterPath);
}

void FNiagaraService::RefreshEditors(UObject* Asset)
{
    if (!Asset || !GEditor)
    {
        return;
    }

    UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
    if (!AssetEditorSubsystem)
    {
        return;
    }

    // Niagara properly implements IAssetEditorInstance, so this works
    TArray<IAssetEditorInstance*> Editors = AssetEditorSubsystem->FindEditorsForAsset(Asset);
    for (IAssetEditorInstance* Editor : Editors)
    {
        if (Editor)
        {
            // The Niagara editor will refresh when the asset is marked dirty
            UE_LOG(LogNiagaraService, Verbose, TEXT("Found open Niagara editor for asset"));
        }
    }
}

// ============================================================================
// Internal Helper Methods
// ============================================================================

bool FNiagaraService::GetScriptUsageFromStage(const FString& Stage, uint8& OutUsage, FString& OutError) const
{
    if (Stage.Equals(TEXT("Spawn"), ESearchCase::IgnoreCase))
    {
        OutUsage = static_cast<uint8>(ENiagaraScriptUsage::ParticleSpawnScript);
        return true;
    }
    else if (Stage.Equals(TEXT("Update"), ESearchCase::IgnoreCase))
    {
        OutUsage = static_cast<uint8>(ENiagaraScriptUsage::ParticleUpdateScript);
        return true;
    }
    else if (Stage.Equals(TEXT("Event"), ESearchCase::IgnoreCase))
    {
        OutUsage = static_cast<uint8>(ENiagaraScriptUsage::ParticleEventScript);
        return true;
    }

    OutError = FString::Printf(TEXT("Invalid stage '%s'. Must be 'Spawn', 'Update', or 'Event'"), *Stage);
    return false;
}

FString FNiagaraService::GetStageFromScriptUsage(uint8 Usage) const
{
    switch (static_cast<ENiagaraScriptUsage>(Usage))
    {
    case ENiagaraScriptUsage::ParticleSpawnScript:
        return TEXT("Spawn");
    case ENiagaraScriptUsage::ParticleUpdateScript:
        return TEXT("Update");
    case ENiagaraScriptUsage::ParticleEventScript:
        return TEXT("Event");
    default:
        return TEXT("Unknown");
    }
}

bool FNiagaraService::FindEmitterHandleByName(UNiagaraSystem* System, const FString& EmitterName, const FNiagaraEmitterHandle** OutHandle) const
{
    if (!System)
    {
        return false;
    }

    for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
    {
        if (Handle.GetName().ToString().Equals(EmitterName, ESearchCase::IgnoreCase))
        {
            *OutHandle = &Handle;
            return true;
        }
    }

    return false;
}

int32 FNiagaraService::FindEmitterHandleIndex(UNiagaraSystem* System, const FString& EmitterName) const
{
    if (!System)
    {
        return INDEX_NONE;
    }

    const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
    for (int32 i = 0; i < Handles.Num(); i++)
    {
        if (Handles[i].GetName().ToString().Equals(EmitterName, ESearchCase::IgnoreCase))
        {
            return i;
        }
    }

    return INDEX_NONE;
}

FVersionedNiagaraEmitterData* FNiagaraService::GetEmitterData(const FNiagaraEmitterHandle& Handle) const
{
    return Handle.GetEmitterData();
}

UNiagaraRendererProperties* FNiagaraService::CreateRendererByType(const FString& RendererType, UObject* Outer) const
{
    if (RendererType.Equals(TEXT("Sprite"), ESearchCase::IgnoreCase))
    {
        return NewObject<UNiagaraSpriteRendererProperties>(Outer);
    }
    else if (RendererType.Equals(TEXT("Mesh"), ESearchCase::IgnoreCase))
    {
        return NewObject<UNiagaraMeshRendererProperties>(Outer);
    }
    else if (RendererType.Equals(TEXT("Ribbon"), ESearchCase::IgnoreCase))
    {
        return NewObject<UNiagaraRibbonRendererProperties>(Outer);
    }
    else if (RendererType.Equals(TEXT("Light"), ESearchCase::IgnoreCase))
    {
        return NewObject<UNiagaraLightRendererProperties>(Outer);
    }
    else if (RendererType.Equals(TEXT("Component"), ESearchCase::IgnoreCase))
    {
        return NewObject<UNiagaraComponentRendererProperties>(Outer);
    }

    return nullptr;
}

UNiagaraDataInterface* FNiagaraService::CreateDataInterfaceByType(const FString& InterfaceType, UObject* Outer) const
{
    // Data interfaces are looked up dynamically
    FString ClassName = FString::Printf(TEXT("NiagaraDataInterface%s"), *InterfaceType);
    UClass* DIClass = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/Niagara.%s"), *ClassName));

    if (DIClass)
    {
        return NewObject<UNiagaraDataInterface>(Outer, DIClass);
    }

    return nullptr;
}

void FNiagaraService::AddSystemMetadata(UNiagaraSystem* System, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata, const FString& EmitterName, const FString& Stage) const
{
    bool bIncludeAll = !Fields || Fields->Num() == 0 || Fields->Contains(TEXT("*"));

    // Emitters
    if (bIncludeAll || Fields->Contains(TEXT("emitters")))
    {
        TArray<TSharedPtr<FJsonValue>> EmittersArray;
        for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
        {
            TSharedPtr<FJsonObject> EmitterObj = MakeShared<FJsonObject>();
            EmitterObj->SetStringField(TEXT("name"), Handle.GetName().ToString());
            EmitterObj->SetStringField(TEXT("id"), Handle.GetId().ToString());
            EmitterObj->SetBoolField(TEXT("enabled"), Handle.GetIsEnabled());

            if (Handle.GetInstance().Emitter)
            {
                EmitterObj->SetStringField(TEXT("emitter_path"), Handle.GetInstance().Emitter->GetPathName());
            }

            EmittersArray.Add(MakeShared<FJsonValueObject>(EmitterObj));
        }
        OutMetadata->SetArrayField(TEXT("emitters"), EmittersArray);
        OutMetadata->SetNumberField(TEXT("emitter_count"), EmittersArray.Num());
    }

    // Compilation status
    if (bIncludeAll || Fields->Contains(TEXT("status")))
    {
        // In UE5.7, use IsValid() to determine basic compilation status
        FString StatusString = System->IsValid() ? TEXT("Valid") : TEXT("Invalid");
        OutMetadata->SetStringField(TEXT("compile_status"), StatusString);
    }

    // Parameters
    if (bIncludeAll || Fields->Contains(TEXT("parameters")))
    {
        TArray<TSharedPtr<FJsonValue>> ParamsArray;
        const FNiagaraParameterStore& Store = System->GetExposedParameters();
        TArray<FNiagaraVariable> Params;
        Store.GetParameters(Params);

        for (const FNiagaraVariable& Param : Params)
        {
            TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
            ParamObj->SetStringField(TEXT("name"), Param.GetName().ToString());
            ParamObj->SetStringField(TEXT("type"), Param.GetType().GetName());
            ParamsArray.Add(MakeShared<FJsonValueObject>(ParamObj));
        }
        OutMetadata->SetArrayField(TEXT("parameters"), ParamsArray);
    }

    // Module list - compact summary (just names, no details)
    if (Fields && Fields->Contains(TEXT("module_list")))
    {
        TArray<TSharedPtr<FJsonValue>> EmitterSummaryArray;

        for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
        {
            FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();
            if (!EmitterData)
            {
                continue;
            }

            TSharedPtr<FJsonObject> EmitterObj = MakeShared<FJsonObject>();
            EmitterObj->SetStringField(TEXT("emitter_name"), Handle.GetName().ToString());

            // Helper lambda to extract module names only
            auto ExtractModuleNames = [](UNiagaraScript* Script) -> TArray<TSharedPtr<FJsonValue>>
            {
                TArray<TSharedPtr<FJsonValue>> NamesArray;
                if (!Script)
                {
                    return NamesArray;
                }

                UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
                if (!ScriptSource || !ScriptSource->NodeGraph)
                {
                    return NamesArray;
                }

                for (UEdGraphNode* Node : ScriptSource->NodeGraph->Nodes)
                {
                    UNiagaraNodeFunctionCall* FunctionNode = Cast<UNiagaraNodeFunctionCall>(Node);
                    if (FunctionNode)
                    {
                        NamesArray.Add(MakeShared<FJsonValueString>(FunctionNode->GetFunctionName()));
                    }
                }
                return NamesArray;
            };

            EmitterObj->SetArrayField(TEXT("spawn_modules"), ExtractModuleNames(EmitterData->SpawnScriptProps.Script));
            EmitterObj->SetArrayField(TEXT("update_modules"), ExtractModuleNames(EmitterData->UpdateScriptProps.Script));
            EmitterSummaryArray.Add(MakeShared<FJsonValueObject>(EmitterObj));
        }

        OutMetadata->SetArrayField(TEXT("module_list"), EmitterSummaryArray);
    }

    // Modules - requires emitter_name + stage filters to avoid excessive data
    if (Fields && Fields->Contains(TEXT("modules")))
    {
        if (EmitterName.IsEmpty() || Stage.IsEmpty())
        {
            OutMetadata->SetStringField(TEXT("modules_error"),
                TEXT("'modules' field requires 'emitter_name' AND 'stage' parameters.\n")
                TEXT("Valid stages: 'Spawn', 'Update', 'Render'\n")
                TEXT("Use 'module_list' field for a compact summary of all emitters."));
        }
        else
        {
            // Find the specific emitter
            const FNiagaraEmitterHandle* TargetHandle = nullptr;
            for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
            {
                if (Handle.GetName().ToString().Equals(EmitterName, ESearchCase::IgnoreCase))
                {
                    TargetHandle = &Handle;
                    break;
                }
            }

            if (!TargetHandle)
            {
                OutMetadata->SetStringField(TEXT("modules_error"), FString::Printf(TEXT("Emitter '%s' not found"), *EmitterName));
            }
            else
            {
                FVersionedNiagaraEmitterData* EmitterData = TargetHandle->GetEmitterData();
                if (EmitterData)
                {
                    // Helper lambda to extract full module details
                    auto ExtractModulesFromScript = [](UNiagaraScript* Script, const FString& StageName) -> TArray<TSharedPtr<FJsonValue>>
                    {
                        TArray<TSharedPtr<FJsonValue>> ModulesArray;
                        if (!Script)
                        {
                            return ModulesArray;
                        }

                        UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
                        if (!ScriptSource || !ScriptSource->NodeGraph)
                        {
                            return ModulesArray;
                        }

                        for (UEdGraphNode* Node : ScriptSource->NodeGraph->Nodes)
                        {
                            UNiagaraNodeFunctionCall* FunctionNode = Cast<UNiagaraNodeFunctionCall>(Node);
                            if (FunctionNode)
                            {
                                TSharedPtr<FJsonObject> ModuleObj = MakeShared<FJsonObject>();
                                ModuleObj->SetStringField(TEXT("name"), FunctionNode->GetFunctionName());
                                ModuleObj->SetStringField(TEXT("node_id"), FunctionNode->NodeGuid.ToString());
                                ModuleObj->SetStringField(TEXT("stage"), StageName);

                                if (UNiagaraScript* FunctionScript = FunctionNode->FunctionScript)
                                {
                                    ModuleObj->SetStringField(TEXT("script_path"), FunctionScript->GetPathName());
                                }

                                ModulesArray.Add(MakeShared<FJsonValueObject>(ModuleObj));
                            }
                        }
                        return ModulesArray;
                    };

                    TSharedPtr<FJsonObject> ModulesObj = MakeShared<FJsonObject>();
                    ModulesObj->SetStringField(TEXT("emitter_name"), EmitterName);
                    ModulesObj->SetStringField(TEXT("stage"), Stage);

                    if (Stage.Equals(TEXT("Spawn"), ESearchCase::IgnoreCase))
                    {
                        ModulesObj->SetArrayField(TEXT("modules"), ExtractModulesFromScript(EmitterData->SpawnScriptProps.Script, TEXT("Spawn")));
                    }
                    else if (Stage.Equals(TEXT("Update"), ESearchCase::IgnoreCase))
                    {
                        ModulesObj->SetArrayField(TEXT("modules"), ExtractModulesFromScript(EmitterData->UpdateScriptProps.Script, TEXT("Update")));
                    }
                    else if (Stage.Equals(TEXT("Render"), ESearchCase::IgnoreCase))
                    {
                        // Extract renderers
                        TArray<TSharedPtr<FJsonValue>> RenderersArray;
                        for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
                        {
                            if (Renderer)
                            {
                                TSharedPtr<FJsonObject> RendererObj = MakeShared<FJsonObject>();
                                RendererObj->SetStringField(TEXT("name"), Renderer->GetName());
                                RendererObj->SetStringField(TEXT("type"), Renderer->GetClass()->GetName());
                                RendererObj->SetBoolField(TEXT("enabled"), Renderer->GetIsEnabled());
                                RenderersArray.Add(MakeShared<FJsonValueObject>(RendererObj));
                            }
                        }
                        ModulesObj->SetArrayField(TEXT("modules"), RenderersArray);
                    }
                    else
                    {
                        OutMetadata->SetStringField(TEXT("modules_error"), FString::Printf(TEXT("Invalid stage '%s'. Use 'Spawn', 'Update', or 'Render'"), *Stage));
                    }

                    if (!OutMetadata->HasField(TEXT("modules_error")))
                    {
                        OutMetadata->SetObjectField(TEXT("modules"), ModulesObj);
                    }
                }
            }
        }
    }

    // Renderers - extract from each emitter in the system
    if (bIncludeAll || Fields->Contains(TEXT("renderers")))
    {
        TArray<TSharedPtr<FJsonValue>> EmitterRenderersArray;

        for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
        {
            FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();
            if (!EmitterData)
            {
                continue;
            }

            TSharedPtr<FJsonObject> EmitterRendererObj = MakeShared<FJsonObject>();
            EmitterRendererObj->SetStringField(TEXT("emitter_name"), Handle.GetName().ToString());

            TArray<TSharedPtr<FJsonValue>> RenderersArray;
            for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
            {
                if (Renderer)
                {
                    TSharedPtr<FJsonObject> RendererObj = MakeShared<FJsonObject>();
                    RendererObj->SetStringField(TEXT("name"), Renderer->GetName());
                    RendererObj->SetStringField(TEXT("type"), Renderer->GetClass()->GetName());
                    RendererObj->SetBoolField(TEXT("enabled"), Renderer->GetIsEnabled());
                    RenderersArray.Add(MakeShared<FJsonValueObject>(RendererObj));
                }
            }

            EmitterRendererObj->SetArrayField(TEXT("renderers"), RenderersArray);
            EmitterRendererObj->SetNumberField(TEXT("renderer_count"), RenderersArray.Num());
            EmitterRenderersArray.Add(MakeShared<FJsonValueObject>(EmitterRendererObj));
        }

        OutMetadata->SetArrayField(TEXT("renderers_by_emitter"), EmitterRenderersArray);
    }
}

void FNiagaraService::AddEmitterMetadata(UNiagaraEmitter* Emitter, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata) const
{
    bool bIncludeAll = !Fields || Fields->Num() == 0 || Fields->Contains(TEXT("*"));

    // Version info
    OutMetadata->SetStringField(TEXT("version"), Emitter->GetExposedVersion().VersionGuid.ToString());

    // Get latest emitter data
    FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData();
    if (!EmitterData)
    {
        return;
    }

    // Renderers
    if (bIncludeAll || Fields->Contains(TEXT("renderers")))
    {
        TArray<TSharedPtr<FJsonValue>> RenderersArray;
        for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
        {
            if (Renderer)
            {
                TSharedPtr<FJsonObject> RendererObj = MakeShared<FJsonObject>();
                RendererObj->SetStringField(TEXT("name"), Renderer->GetName());
                RendererObj->SetStringField(TEXT("type"), Renderer->GetClass()->GetName());
                RendererObj->SetBoolField(TEXT("enabled"), Renderer->GetIsEnabled());
                RenderersArray.Add(MakeShared<FJsonValueObject>(RendererObj));
            }
        }
        OutMetadata->SetArrayField(TEXT("renderers"), RenderersArray);
    }
}

bool FNiagaraService::CreateAssetPackage(const FString& Path, const FString& Name, UPackage*& OutPackage, FString& OutError) const
{
    FString PackagePath = Path / Name;

    // Ensure path starts with /Game
    if (!PackagePath.StartsWith(TEXT("/Game")))
    {
        PackagePath = TEXT("/Game") / PackagePath;
    }

    // Check if asset already exists
    if (FindPackage(nullptr, *PackagePath))
    {
        OutError = FString::Printf(TEXT("Asset already exists at path: %s"), *PackagePath);
        return false;
    }

    OutPackage = CreatePackage(*PackagePath);
    if (!OutPackage)
    {
        OutError = FString::Printf(TEXT("Failed to create package: %s"), *PackagePath);
        return false;
    }

    return true;
}

bool FNiagaraService::SaveAsset(UObject* Asset, FString& OutError) const
{
    if (!Asset)
    {
        OutError = TEXT("Cannot save null asset");
        return false;
    }

    UPackage* Package = Asset->GetOutermost();
    Package->MarkPackageDirty();

    FString PackageFileName = FPackageName::LongPackageNameToFilename(
        Package->GetName(),
        FPackageName::GetAssetPackageExtension()
    );

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;

    FSavePackageResultStruct Result = UPackage::Save(Package, Asset, *PackageFileName, SaveArgs);

    if (!Result.IsSuccessful())
    {
        OutError = FString::Printf(TEXT("Failed to save package: %s"), *PackageFileName);
        return false;
    }

    return true;
}

void FNiagaraService::MarkSystemDirty(UNiagaraSystem* System) const
{
    if (System)
    {
        System->Modify();
        System->MarkPackageDirty();
    }
}
