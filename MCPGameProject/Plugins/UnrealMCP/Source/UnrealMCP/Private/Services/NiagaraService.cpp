#include "Services/NiagaraService.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "ObjectTools.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/SavePackage.h"

// Niagara includes
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraDataInterface.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraLightRendererProperties.h"
#include "NiagaraComponentRendererProperties.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeInput.h"
#include "NiagaraScriptSource.h"

// Niagara Editor includes
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEmitterFactoryNew.h"
#include "NiagaraEditorUtilities.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraTypes.h"
#include "NiagaraParameterMapHistory.h"

DEFINE_LOG_CATEGORY_STATIC(LogNiagaraService, Log, All);

// Singleton instance
TUniquePtr<FNiagaraService> FNiagaraService::Instance;

FNiagaraService::FNiagaraService()
{
    UE_LOG(LogNiagaraService, Log, TEXT("FNiagaraService initialized"));
}

FNiagaraService& FNiagaraService::Get()
{
    if (!Instance.IsValid())
    {
        Instance = MakeUnique<FNiagaraService>();
    }
    return *Instance;
}

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
            const FNiagaraEmitterHandle& Handle = System->GetEmitterHandle(i);
            if (Handle.GetId() == OutEmitterHandleId)
            {
                System->Modify();
                // Note: SetName requires non-const access which may need different approach
                break;
            }
        }
    }

    // Mark dirty and refresh
    MarkSystemDirty(System);
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Added emitter '%s' to system '%s' with handle ID: %s"),
        *EmitterPath, *SystemPath, *OutEmitterHandleId.ToString());

    return true;
}

bool FNiagaraService::GetMetadata(const FString& AssetPath, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata)
{
    OutMetadata = MakeShared<FJsonObject>();

    // Try to load as system first
    UNiagaraSystem* System = FindSystem(AssetPath);
    if (System)
    {
        OutMetadata->SetStringField(TEXT("asset_type"), TEXT("NiagaraSystem"));
        OutMetadata->SetStringField(TEXT("asset_path"), AssetPath);
        OutMetadata->SetStringField(TEXT("asset_name"), System->GetName());
        AddSystemMetadata(System, Fields, OutMetadata);
        OutMetadata->SetBoolField(TEXT("success"), true);
        return true;
    }

    // Try as emitter
    UNiagaraEmitter* Emitter = FindEmitter(AssetPath);
    if (Emitter)
    {
        OutMetadata->SetStringField(TEXT("asset_type"), TEXT("NiagaraEmitter"));
        OutMetadata->SetStringField(TEXT("asset_path"), AssetPath);
        OutMetadata->SetStringField(TEXT("asset_name"), Emitter->GetName());
        AddEmitterMetadata(Emitter, Fields, OutMetadata);
        OutMetadata->SetBoolField(TEXT("success"), true);
        return true;
    }

    OutMetadata->SetBoolField(TEXT("success"), false);
    OutMetadata->SetStringField(TEXT("error"), FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
    return false;
}

bool FNiagaraService::CompileAsset(const FString& AssetPath, FString& OutError)
{
    // Try as system first
    UNiagaraSystem* System = FindSystem(AssetPath);
    if (System)
    {
        // Request compilation
        System->RequestCompile(false);  // false = synchronous
        System->WaitForCompilationComplete();

        // In UE5.7, we check if the system is valid after compilation
        if (System->IsValid())
        {
            UE_LOG(LogNiagaraService, Log, TEXT("Niagara System compiled successfully: %s"), *AssetPath);
            return true;
        }
        else
        {
            // Collect detailed error information
            TArray<FString> ErrorMessages;

            // Check each emitter for issues
            for (int32 i = 0; i < System->GetEmitterHandles().Num(); i++)
            {
                const FNiagaraEmitterHandle& Handle = System->GetEmitterHandle(i);
                FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();

                if (!EmitterData)
                {
                    ErrorMessages.Add(FString::Printf(TEXT("Emitter '%s': No emitter data available"), *Handle.GetName().ToString()));
                    continue;
                }

                // Helper lambda to extract compile errors from a script
                auto ExtractScriptErrors = [&ErrorMessages, &Handle](UNiagaraScript* Script, const FString& ScriptTypeName)
                {
                    if (!Script)
                    {
                        return;
                    }

                    // Check if script has errors
                    if (!Script->IsScriptCompilationPending(false) &&
                        Script->GetLastCompileStatus() == ENiagaraScriptCompileStatus::NCS_Error)
                    {
                        // Extract actual error messages from LastCompileEvents
                        const FNiagaraVMExecutableData& VMData = Script->GetVMExecutableData();
                        bool bFoundSpecificError = false;

                        for (const FNiagaraCompileEvent& Event : VMData.LastCompileEvents)
                        {
                            if (Event.Severity == FNiagaraCompileEventSeverity::Error)
                            {
                                ErrorMessages.Add(FString::Printf(TEXT("[%s] %s: %s"),
                                    *Handle.GetName().ToString(),
                                    *ScriptTypeName,
                                    *Event.Message));
                                bFoundSpecificError = true;
                            }
                            else if (Event.Severity == FNiagaraCompileEventSeverity::Warning)
                            {
                                ErrorMessages.Add(FString::Printf(TEXT("[%s] %s [Warning]: %s"),
                                    *Handle.GetName().ToString(),
                                    *ScriptTypeName,
                                    *Event.Message));
                            }
                        }

                        // Also check the ErrorMsg field
                        if (!VMData.ErrorMsg.IsEmpty())
                        {
                            ErrorMessages.Add(FString::Printf(TEXT("[%s] %s: %s"),
                                *Handle.GetName().ToString(),
                                *ScriptTypeName,
                                *VMData.ErrorMsg));
                            bFoundSpecificError = true;
                        }

                        // Fallback if no specific error found
                        if (!bFoundSpecificError)
                        {
                            ErrorMessages.Add(FString::Printf(TEXT("[%s] %s: Compilation error (no details available)"),
                                *Handle.GetName().ToString(),
                                *ScriptTypeName));
                        }
                    }
                };

                // Check spawn script
                ExtractScriptErrors(EmitterData->SpawnScriptProps.Script, TEXT("Spawn Script"));

                // Check update script
                ExtractScriptErrors(EmitterData->UpdateScriptProps.Script, TEXT("Update Script"));

                // Check renderers - use the FText version which is simpler
                for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
                {
                    if (Renderer)
                    {
                        // Get renderer feedback using FText version
                        TArray<FText> RendererErrors;
                        TArray<FText> RendererWarnings;
                        TArray<FText> RendererInfo;
                        Renderer->GetRendererFeedback(Handle.GetInstance(), RendererErrors, RendererWarnings, RendererInfo);

                        for (const FText& Error : RendererErrors)
                        {
                            ErrorMessages.Add(FString::Printf(TEXT("Emitter '%s' Renderer '%s': %s"),
                                *Handle.GetName().ToString(),
                                *Renderer->GetName(),
                                *Error.ToString()));
                        }

                        // Also include warnings as they may indicate why it's invalid
                        for (const FText& Warning : RendererWarnings)
                        {
                            ErrorMessages.Add(FString::Printf(TEXT("Emitter '%s' Renderer '%s' [Warning]: %s"),
                                *Handle.GetName().ToString(),
                                *Renderer->GetName(),
                                *Warning.ToString()));
                        }
                    }
                }
            }

            // If no specific errors found, provide generic message with hints
            if (ErrorMessages.Num() == 0)
            {
                ErrorMessages.Add(TEXT("System is invalid. Common causes:"));
                ErrorMessages.Add(TEXT("- Missing required modules (InitializeParticle, etc.)"));
                ErrorMessages.Add(TEXT("- No valid renderers configured"));
                ErrorMessages.Add(TEXT("- Missing required particle attributes"));
                ErrorMessages.Add(TEXT("- Unresolved parameter bindings"));
            }

            OutError = FString::Join(ErrorMessages, TEXT("\n"));
            return false;
        }
    }

    // Try as standalone emitter
    UNiagaraEmitter* Emitter = FindEmitter(AssetPath);
    if (Emitter)
    {
        // Emitters typically compile in context of a system
        // For standalone emitter, just validate it can be used
        OutError = TEXT("Standalone emitter compilation not fully supported - add to a system to compile");
        return true;  // Not a hard failure
    }

    OutError = FString::Printf(TEXT("Asset not found: %s"), *AssetPath);
    return false;
}

// ============================================================================
// Module System (Feature 2) - Stubs for now
// ============================================================================

bool FNiagaraService::AddModule(const FNiagaraModuleAddParams& Params, FString& OutModuleId, FString& OutError)
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

    const FNiagaraEmitterHandle& EmitterHandle = System->GetEmitterHandle(EmitterIndex);
    FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
    if (!EmitterData)
    {
        OutError = FString::Printf(TEXT("Could not get emitter data for '%s'"), *Params.EmitterName);
        return false;
    }

    // Convert stage to script usage
    uint8 UsageValue;
    if (!GetScriptUsageFromStage(Params.Stage, UsageValue, OutError))
    {
        return false;
    }
    ENiagaraScriptUsage ScriptUsage = static_cast<ENiagaraScriptUsage>(UsageValue);

    // Get the script for this stage
    UNiagaraScript* Script = nullptr;
    switch (ScriptUsage)
    {
    case ENiagaraScriptUsage::ParticleSpawnScript:
        Script = EmitterData->SpawnScriptProps.Script;
        break;
    case ENiagaraScriptUsage::ParticleUpdateScript:
        Script = EmitterData->UpdateScriptProps.Script;
        break;
    case ENiagaraScriptUsage::ParticleEventScript:
        // Event scripts require more complex handling with event handlers
        OutError = TEXT("Event stage module addition not yet fully supported");
        return false;
    default:
        OutError = FString::Printf(TEXT("Unsupported script usage for stage '%s'"), *Params.Stage);
        return false;
    }

    if (!Script)
    {
        OutError = FString::Printf(TEXT("Script not found for stage '%s' in emitter '%s'"), *Params.Stage, *Params.EmitterName);
        return false;
    }

    // Get the script source and graph
    UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
    if (!ScriptSource)
    {
        OutError = TEXT("Could not get script source");
        return false;
    }

    UNiagaraGraph* Graph = ScriptSource->NodeGraph;
    if (!Graph)
    {
        OutError = TEXT("Could not get script graph");
        return false;
    }

    // Find the output node for this script by iterating through nodes
    UNiagaraNodeOutput* OutputNode = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UNiagaraNodeOutput* TestNode = Cast<UNiagaraNodeOutput>(Node);
        if (TestNode && TestNode->GetUsage() == ScriptUsage)
        {
            OutputNode = TestNode;
            break;
        }
    }

    if (!OutputNode)
    {
        OutError = FString::Printf(TEXT("Could not find output node for stage '%s'"), *Params.Stage);
        return false;
    }

    // Load the module script
    UNiagaraScript* ModuleScript = LoadObject<UNiagaraScript>(nullptr, *Params.ModulePath);
    if (!ModuleScript)
    {
        OutError = FString::Printf(TEXT("Module script not found: %s"), *Params.ModulePath);
        return false;
    }

    // Check if this module already exists in the graph (prevent duplicates)
    FString ModuleScriptName = ModuleScript->GetName();
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UNiagaraNodeFunctionCall* FunctionNode = Cast<UNiagaraNodeFunctionCall>(Node);
        if (FunctionNode && FunctionNode->FunctionScript == ModuleScript)
        {
            OutError = FString::Printf(TEXT("Module '%s' already exists in emitter '%s'. Duplicate modules can cause compilation errors."),
                *ModuleScriptName, *Params.EmitterName);
            return false;
        }
    }

    // Mark the system as modified
    System->Modify();

    // Add the module using the stack graph utilities
    int32 TargetIndex = Params.Index >= 0 ? Params.Index : INDEX_NONE;
    UNiagaraNodeFunctionCall* NewModuleNode = FNiagaraStackGraphUtilities::AddScriptModuleToStack(
        ModuleScript,
        *OutputNode,
        TargetIndex
    );

    if (!NewModuleNode)
    {
        OutError = TEXT("Failed to add module to stack");
        return false;
    }

    // Get the module node ID
    OutModuleId = NewModuleNode->NodeGuid.ToString();

    // Mark system dirty and request recompile
    MarkSystemDirty(System);
    System->RequestCompile(false);

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Added module '%s' to emitter '%s' stage '%s' with ID: %s"),
        *Params.ModulePath, *Params.EmitterName, *Params.Stage, *OutModuleId);

    return true;
}

bool FNiagaraService::SearchModules(const FString& SearchQuery, const FString& StageFilter, int32 MaxResults, TArray<TSharedPtr<FJsonObject>>& OutModules)
{
    // This can be implemented now as it's just asset discovery
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> ModuleAssets;
    AssetRegistry.GetAssetsByClass(FTopLevelAssetPath(TEXT("/Script/Niagara"), TEXT("NiagaraScript")), ModuleAssets);

    int32 Count = 0;
    for (const FAssetData& Asset : ModuleAssets)
    {
        if (Count >= MaxResults)
        {
            break;
        }

        FString AssetName = Asset.AssetName.ToString();

        // Apply search filter
        if (!SearchQuery.IsEmpty() && !AssetName.Contains(SearchQuery, ESearchCase::IgnoreCase))
        {
            continue;
        }

        TSharedPtr<FJsonObject> ModuleInfo = MakeShared<FJsonObject>();
        ModuleInfo->SetStringField(TEXT("name"), AssetName);
        ModuleInfo->SetStringField(TEXT("path"), Asset.GetObjectPathString());

        OutModules.Add(ModuleInfo);
        Count++;
    }

    return true;
}

bool FNiagaraService::SetModuleInput(const FNiagaraModuleInputParams& Params, FString& OutError)
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

    const FNiagaraEmitterHandle& EmitterHandle = System->GetEmitterHandle(EmitterIndex);
    FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
    if (!EmitterData)
    {
        OutError = FString::Printf(TEXT("Could not get emitter data for '%s'"), *Params.EmitterName);
        return false;
    }

    // Convert stage to script usage
    uint8 UsageValue;
    if (!GetScriptUsageFromStage(Params.Stage, UsageValue, OutError))
    {
        return false;
    }
    ENiagaraScriptUsage ScriptUsage = static_cast<ENiagaraScriptUsage>(UsageValue);

    // Get the script for this stage
    UNiagaraScript* Script = nullptr;
    switch (ScriptUsage)
    {
    case ENiagaraScriptUsage::ParticleSpawnScript:
        Script = EmitterData->SpawnScriptProps.Script;
        break;
    case ENiagaraScriptUsage::ParticleUpdateScript:
        Script = EmitterData->UpdateScriptProps.Script;
        break;
    default:
        OutError = FString::Printf(TEXT("Unsupported script usage for stage '%s'"), *Params.Stage);
        return false;
    }

    if (!Script)
    {
        OutError = FString::Printf(TEXT("Script not found for stage '%s' in emitter '%s'"), *Params.Stage, *Params.EmitterName);
        return false;
    }

    // Get the script source and graph
    UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
    if (!ScriptSource)
    {
        OutError = TEXT("Could not get script source");
        return false;
    }

    UNiagaraGraph* Graph = ScriptSource->NodeGraph;
    if (!Graph)
    {
        OutError = TEXT("Could not get script graph");
        return false;
    }

    // Find the module node by name (normalize by removing spaces for comparison)
    UNiagaraNodeFunctionCall* ModuleNode = nullptr;
    FString NormalizedSearchName = Params.ModuleName.Replace(TEXT(" "), TEXT(""));
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UNiagaraNodeFunctionCall* FunctionNode = Cast<UNiagaraNodeFunctionCall>(Node);
        if (FunctionNode)
        {
            FString NodeName = FunctionNode->GetFunctionName();
            FString NormalizedNodeName = NodeName.Replace(TEXT(" "), TEXT(""));
            if (NormalizedNodeName.Contains(NormalizedSearchName, ESearchCase::IgnoreCase) ||
                NormalizedSearchName.Contains(NormalizedNodeName, ESearchCase::IgnoreCase))
            {
                ModuleNode = FunctionNode;
                break;
            }
        }
    }

    if (!ModuleNode)
    {
        OutError = FString::Printf(TEXT("Module '%s' not found in stage '%s'"), *Params.ModuleName, *Params.Stage);
        return false;
    }

    // Get the value as string
    FString ValueStr;
    if (Params.Value.IsValid() && Params.Value->Type == EJson::String)
    {
        ValueStr = Params.Value->AsString();
    }
    else
    {
        OutError = TEXT("Value must be provided as a string");
        return false;
    }

    // Mark system for modification
    System->Modify();

    // First try to find an exposed pin (for static switches/enums)
    bool bFoundExposedPin = false;
    for (UEdGraphPin* Pin : ModuleNode->Pins)
    {
        if (Pin->Direction == EGPD_Input && Pin->PinName.ToString().Contains(Params.InputName, ESearchCase::IgnoreCase))
        {
            // Found an exposed pin, set its value directly
            FString TypeHint = Params.ValueType.ToLower();

            if (TypeHint == TEXT("vector") || TypeHint == TEXT("float3"))
            {
                TArray<FString> Components;
                ValueStr.ParseIntoArray(Components, TEXT(","), true);
                if (Components.Num() >= 3)
                {
                    FVector Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]));
                    Pin->DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), Vec.X, Vec.Y, Vec.Z);
                    bFoundExposedPin = true;
                }
            }
            else
            {
                Pin->DefaultValue = ValueStr;
                bFoundExposedPin = true;
            }
            break;
        }
    }

    // If not found as exposed pin, try the override pin system for value inputs
    if (!bFoundExposedPin)
    {
        // Get the module's called graph to find input parameters
        UNiagaraGraph* ModuleGraph = ModuleNode->GetCalledGraph();
        if (!ModuleGraph)
        {
            OutError = FString::Printf(TEXT("Could not get module graph for '%s'"), *Params.ModuleName);
            return false;
        }

        // Get module inputs using the proper Stack API
        // Create a constant resolver for the system context
        FCompileConstantResolver ConstantResolver(System, ScriptUsage);

        TArray<FNiagaraVariable> ModuleInputs;
        FNiagaraStackGraphUtilities::GetStackFunctionInputs(
            *ModuleNode,
            ModuleInputs,
            ConstantResolver,
            FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly
        );

        // Find the input by name
        FNiagaraVariable* FoundInput = nullptr;
        for (FNiagaraVariable& Input : ModuleInputs)
        {
            FString InputNameStr = Input.GetName().ToString();
            // Input names are in "Module.InputName" format, so extract just the name part
            FString SimpleName = InputNameStr;
            int32 DotIndex = INDEX_NONE;
            if (InputNameStr.FindLastChar('.', DotIndex))
            {
                SimpleName = InputNameStr.RightChop(DotIndex + 1);
            }

            if (SimpleName.Equals(Params.InputName, ESearchCase::IgnoreCase))
            {
                FoundInput = &Input;
                break;
            }
        }

        if (!FoundInput)
        {
            // List available inputs for debugging
            TArray<FString> AvailableInputs;
            for (const FNiagaraVariable& Input : ModuleInputs)
            {
                AvailableInputs.Add(Input.GetName().ToString());
            }
            OutError = FString::Printf(TEXT("Input '%s' not found on module '%s'. Available inputs: %s"),
                *Params.InputName, *Params.ModuleName, *FString::Join(AvailableInputs, TEXT(", ")));
            return false;
        }

        // Get input type and metadata
        FNiagaraTypeDefinition InputType = FoundInput->GetType();
        TOptional<FNiagaraVariableMetaData> InputMetaData = ModuleGraph->GetMetaData(*FoundInput);
        FGuid InputVariableGuid = InputMetaData.IsSet() ? InputMetaData->GetVariableGuid() : FGuid();

        // Create aliased parameter handle
        FNiagaraParameterHandle AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
            FoundInput->GetName(),
            FName(*ModuleNode->GetFunctionName())
        );

        // Get or create the override pin
        UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
            *ModuleNode,
            AliasedHandle,
            InputType,
            InputVariableGuid,
            FGuid()
        );

        // Parse the value and set it on the override pin
        FNiagaraVariable TempVariable(InputType, NAME_None);

        // Handle different types
        FString TypeHint = Params.ValueType.ToLower();
        bool bValueSet = false;

        if (InputType == FNiagaraTypeDefinition::GetFloatDef())
        {
            float FloatValue = FCString::Atof(*ValueStr);
            TempVariable.AllocateData();
            TempVariable.SetValue<float>(FloatValue);
            bValueSet = true;
        }
        else if (InputType == FNiagaraTypeDefinition::GetIntDef())
        {
            int32 IntValue = FCString::Atoi(*ValueStr);
            TempVariable.AllocateData();
            TempVariable.SetValue<int32>(IntValue);
            bValueSet = true;
        }
        else if (InputType == FNiagaraTypeDefinition::GetBoolDef())
        {
            bool BoolValue = ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("1"));
            TempVariable.AllocateData();
            TempVariable.SetValue<FNiagaraBool>(FNiagaraBool(BoolValue));
            bValueSet = true;
        }
        else if (InputType == FNiagaraTypeDefinition::GetVec3Def())
        {
            TArray<FString> Components;
            ValueStr.ParseIntoArray(Components, TEXT(","), true);
            if (Components.Num() >= 3)
            {
                FVector3f Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]));
                TempVariable.AllocateData();
                TempVariable.SetValue<FVector3f>(Vec);
                bValueSet = true;
            }
        }
        else if (InputType == FNiagaraTypeDefinition::GetColorDef())
        {
            TArray<FString> Components;
            ValueStr.ParseIntoArray(Components, TEXT(","), true);
            if (Components.Num() >= 4)
            {
                FLinearColor Color(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]),
                                   FCString::Atof(*Components[2]), FCString::Atof(*Components[3]));
                TempVariable.AllocateData();
                TempVariable.SetValue<FLinearColor>(Color);
                bValueSet = true;
            }
            else if (Components.Num() >= 3)
            {
                FLinearColor Color(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]),
                                   FCString::Atof(*Components[2]), 1.0f);
                TempVariable.AllocateData();
                TempVariable.SetValue<FLinearColor>(Color);
                bValueSet = true;
            }
        }

        if (!bValueSet)
        {
            OutError = FString::Printf(TEXT("Unsupported input type '%s' for input '%s'. Supported types: Float, Int, Bool, Vec3, Color"),
                *InputType.GetName(), *Params.InputName);
            return false;
        }

        // Convert to pin default value string
        const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
        FString PinDefaultValue;
        if (!Schema->TryGetPinDefaultValueFromNiagaraVariable(TempVariable, PinDefaultValue))
        {
            OutError = FString::Printf(TEXT("Could not convert value to pin default for input '%s'"), *Params.InputName);
            return false;
        }

        // Set the override pin value
        OverridePin.Modify();
        OverridePin.DefaultValue = PinDefaultValue;

        // Mark the node for synchronization
        UNiagaraNode* OverrideNode = Cast<UNiagaraNode>(OverridePin.GetOwningNode());
        if (OverrideNode)
        {
            OverrideNode->MarkNodeRequiresSynchronization(TEXT("Module input override value changed"), true);
        }

        UE_LOG(LogNiagaraService, Log, TEXT("Set input '%s' on module '%s' via override pin system to '%s'"),
            *Params.InputName, *Params.ModuleName, *PinDefaultValue);
    }

    // Notify graph of changes
    Graph->NotifyGraphChanged();

    // Mark system dirty and request recompile
    MarkSystemDirty(System);
    System->RequestCompile(false);

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set input '%s' on module '%s' in emitter '%s' stage '%s' to '%s'"),
        *Params.InputName, *Params.ModuleName, *Params.EmitterName, *Params.Stage, *ValueStr);

    return true;
}

// ============================================================================
// Parameters (Feature 3)
// ============================================================================

bool FNiagaraService::AddParameter(const FNiagaraParameterAddParams& Params, FString& OutError)
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

    // Determine the type definition
    FNiagaraTypeDefinition TypeDef;
    FString TypeLower = Params.ParameterType.ToLower();

    if (TypeLower == TEXT("float"))
    {
        TypeDef = FNiagaraTypeDefinition::GetFloatDef();
    }
    else if (TypeLower == TEXT("int") || TypeLower == TEXT("int32"))
    {
        TypeDef = FNiagaraTypeDefinition::GetIntDef();
    }
    else if (TypeLower == TEXT("bool") || TypeLower == TEXT("boolean"))
    {
        TypeDef = FNiagaraTypeDefinition::GetBoolDef();
    }
    else if (TypeLower == TEXT("vector") || TypeLower == TEXT("vec3") || TypeLower == TEXT("vector3"))
    {
        TypeDef = FNiagaraTypeDefinition::GetVec3Def();
    }
    else if (TypeLower == TEXT("linearcolor") || TypeLower == TEXT("color"))
    {
        TypeDef = FNiagaraTypeDefinition::GetColorDef();
    }
    else
    {
        OutError = FString::Printf(TEXT("Unsupported parameter type '%s'. Supported: Float, Int, Bool, Vector, LinearColor"), *Params.ParameterType);
        return false;
    }

    // Build the full parameter name with scope prefix
    FString FullParameterName = Params.ParameterName;
    if (!FullParameterName.Contains(TEXT(".")))
    {
        // Add default scope prefix if not already present
        if (Params.Scope.Equals(TEXT("user"), ESearchCase::IgnoreCase))
        {
            FullParameterName = TEXT("User.") + FullParameterName;
        }
        else if (Params.Scope.Equals(TEXT("system"), ESearchCase::IgnoreCase))
        {
            FullParameterName = TEXT("System.") + FullParameterName;
        }
        else if (Params.Scope.Equals(TEXT("emitter"), ESearchCase::IgnoreCase))
        {
            FullParameterName = TEXT("Emitter.") + FullParameterName;
        }
    }

    // Create the parameter variable
    FNiagaraVariable NewParam(TypeDef, FName(*FullParameterName));

    // Set default value if provided
    if (Params.DefaultValue.IsValid() && Params.DefaultValue->Type == EJson::String)
    {
        FString ValueStr = Params.DefaultValue->AsString();
        NewParam.AllocateData();

        if (TypeDef == FNiagaraTypeDefinition::GetFloatDef())
        {
            NewParam.SetValue<float>(FCString::Atof(*ValueStr));
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetIntDef())
        {
            NewParam.SetValue<int32>(FCString::Atoi(*ValueStr));
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetBoolDef())
        {
            bool bValue = ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("1"));
            NewParam.SetValue<FNiagaraBool>(FNiagaraBool(bValue));
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetVec3Def())
        {
            TArray<FString> Components;
            ValueStr.ParseIntoArray(Components, TEXT(","), true);
            if (Components.Num() >= 3)
            {
                FVector3f Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]));
                NewParam.SetValue<FVector3f>(Vec);
            }
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetColorDef())
        {
            TArray<FString> Components;
            ValueStr.ParseIntoArray(Components, TEXT(","), true);
            if (Components.Num() >= 3)
            {
                float A = Components.Num() >= 4 ? FCString::Atof(*Components[3]) : 1.0f;
                FLinearColor Color(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]), A);
                NewParam.SetValue<FLinearColor>(Color);
            }
        }
    }
    else
    {
        // Initialize with default values
        NewParam.AllocateData();
        if (TypeDef == FNiagaraTypeDefinition::GetFloatDef())
        {
            NewParam.SetValue<float>(0.0f);
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetIntDef())
        {
            NewParam.SetValue<int32>(0);
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetBoolDef())
        {
            NewParam.SetValue<FNiagaraBool>(FNiagaraBool(false));
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetVec3Def())
        {
            NewParam.SetValue<FVector3f>(FVector3f::ZeroVector);
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetColorDef())
        {
            NewParam.SetValue<FLinearColor>(FLinearColor::White);
        }
    }

    // Get the exposed parameter store and add the parameter
    System->Modify();
    FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();

    // Check if parameter already exists
    if (ExposedParams.FindParameterOffset(NewParam) != nullptr)
    {
        OutError = FString::Printf(TEXT("Parameter '%s' already exists in system"), *FullParameterName);
        return false;
    }

    // Add the parameter
    ExposedParams.AddParameter(NewParam, true, true);

    // Mark dirty and refresh
    MarkSystemDirty(System);
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Added parameter '%s' (%s) to system '%s'"),
        *FullParameterName, *Params.ParameterType, *Params.SystemPath);

    return true;
}

bool FNiagaraService::SetParameter(const FString& SystemPath, const FString& ParameterName, const TSharedPtr<FJsonValue>& Value, FString& OutError)
{
    // Find the system
    UNiagaraSystem* System = FindSystem(SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("System not found: %s"), *SystemPath);
        return false;
    }

    // Get the value as string
    FString ValueStr;
    if (Value.IsValid() && Value->Type == EJson::String)
    {
        ValueStr = Value->AsString();
    }
    else
    {
        OutError = TEXT("Value must be provided as a string");
        return false;
    }

    // Get the exposed parameter store
    FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();

    // Get all parameters and find the one we want
    TArray<FNiagaraVariable> AllParams;
    ExposedParams.GetParameters(AllParams);

    FNiagaraVariable* FoundParam = nullptr;
    for (FNiagaraVariable& Param : AllParams)
    {
        if (Param.GetName().ToString().Equals(ParameterName, ESearchCase::IgnoreCase) ||
            Param.GetName().ToString().EndsWith(ParameterName, ESearchCase::IgnoreCase))
        {
            FoundParam = &Param;
            break;
        }
    }

    if (!FoundParam)
    {
        OutError = FString::Printf(TEXT("Parameter '%s' not found in system. Available: %s"),
            *ParameterName, *FString::JoinBy(AllParams, TEXT(", "), [](const FNiagaraVariable& V) { return V.GetName().ToString(); }));
        return false;
    }

    // Create a copy to set the value
    FNiagaraVariable UpdatedParam = *FoundParam;
    UpdatedParam.AllocateData();

    // Set value based on type
    FNiagaraTypeDefinition TypeDef = FoundParam->GetType();

    if (TypeDef == FNiagaraTypeDefinition::GetFloatDef())
    {
        UpdatedParam.SetValue<float>(FCString::Atof(*ValueStr));
    }
    else if (TypeDef == FNiagaraTypeDefinition::GetIntDef())
    {
        UpdatedParam.SetValue<int32>(FCString::Atoi(*ValueStr));
    }
    else if (TypeDef == FNiagaraTypeDefinition::GetBoolDef())
    {
        bool bValue = ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("1"));
        UpdatedParam.SetValue<FNiagaraBool>(FNiagaraBool(bValue));
    }
    else if (TypeDef == FNiagaraTypeDefinition::GetVec3Def())
    {
        TArray<FString> Components;
        ValueStr.ParseIntoArray(Components, TEXT(","), true);
        if (Components.Num() >= 3)
        {
            FVector3f Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]));
            UpdatedParam.SetValue<FVector3f>(Vec);
        }
        else
        {
            OutError = TEXT("Vector value requires 3 comma-separated components (x,y,z)");
            return false;
        }
    }
    else if (TypeDef == FNiagaraTypeDefinition::GetColorDef())
    {
        TArray<FString> Components;
        ValueStr.ParseIntoArray(Components, TEXT(","), true);
        if (Components.Num() >= 3)
        {
            float A = Components.Num() >= 4 ? FCString::Atof(*Components[3]) : 1.0f;
            FLinearColor Color(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]), A);
            UpdatedParam.SetValue<FLinearColor>(Color);
        }
        else
        {
            OutError = TEXT("Color value requires 3-4 comma-separated components (r,g,b[,a])");
            return false;
        }
    }
    else
    {
        OutError = FString::Printf(TEXT("Unsupported parameter type: %s"), *TypeDef.GetName());
        return false;
    }

    // Set the parameter value in the store
    System->Modify();
    ExposedParams.SetParameterData(UpdatedParam.GetData(), UpdatedParam, true);

    // Mark dirty and refresh
    MarkSystemDirty(System);
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set parameter '%s' to '%s' in system '%s'"),
        *ParameterName, *ValueStr, *SystemPath);

    return true;
}

// ============================================================================
// Data Interfaces (Feature 4)
// ============================================================================

bool FNiagaraService::AddDataInterface(const FNiagaraDataInterfaceParams& Params, FString& OutInterfaceId, FString& OutError)
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

    const FNiagaraEmitterHandle& EmitterHandle = System->GetEmitterHandle(EmitterIndex);
    FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
    if (!EmitterData)
    {
        OutError = FString::Printf(TEXT("Could not get emitter data for '%s'"), *Params.EmitterName);
        return false;
    }

    // Create the data interface
    UNiagaraDataInterface* NewDI = CreateDataInterfaceByType(Params.InterfaceType, EmitterHandle.GetInstance().Emitter);
    if (!NewDI)
    {
        OutError = FString::Printf(TEXT("Failed to create data interface of type '%s'. Supported types: StaticMesh, SkeletalMesh, Spline, Audio, Curve, Texture, Grid2D, Grid3D"), *Params.InterfaceType);
        return false;
    }

    // Generate interface name
    FString DIName = Params.InterfaceName.IsEmpty() ?
        FString::Printf(TEXT("%s_DI_%d"), *Params.InterfaceType, FMath::Rand() % 1000) :
        Params.InterfaceName;

    // Mark system modified
    System->Modify();

    // Create a parameter for the data interface and add to exposed parameters
    FNiagaraTypeDefinition DITypeDef(NewDI->GetClass());
    FNiagaraVariable DIVar(DITypeDef, FName(*DIName));

    // Add to system's exposed parameters so it can be found and modified later
    FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();
    ExposedParams.AddParameter(DIVar, true, true);

    // Set the data interface value using the parameter store's SetDataInterface
    const int32* DIOffset = ExposedParams.FindParameterOffset(DIVar);
    if (DIOffset != nullptr)
    {
        ExposedParams.SetDataInterface(NewDI, *DIOffset);
    }

    OutInterfaceId = DIName;

    UE_LOG(LogNiagaraService, Log, TEXT("Added data interface '%s' of type '%s' to emitter '%s'"),
        *DIName, *Params.InterfaceType, *Params.EmitterName);

    // Mark dirty and refresh
    MarkSystemDirty(System);
    RefreshEditors(System);

    return true;
}

bool FNiagaraService::SetDataInterfaceProperty(const FString& SystemPath, const FString& EmitterName, const FString& InterfaceName, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError)
{
    // Find the system
    UNiagaraSystem* System = FindSystem(SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("System not found: %s"), *SystemPath);
        return false;
    }

    // Get the exposed parameters and find the data interface
    FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();
    const TArray<UNiagaraDataInterface*>& DataInterfaces = ExposedParams.GetDataInterfaces();

    // Find the DI by name
    UNiagaraDataInterface* FoundDI = nullptr;
    for (UNiagaraDataInterface* DI : DataInterfaces)
    {
        if (DI && DI->GetName().Contains(InterfaceName, ESearchCase::IgnoreCase))
        {
            FoundDI = DI;
            break;
        }
    }

    if (!FoundDI)
    {
        // List available data interfaces
        TArray<FString> DINames;
        for (UNiagaraDataInterface* DI : DataInterfaces)
        {
            if (DI)
            {
                DINames.Add(DI->GetName());
            }
        }
        OutError = FString::Printf(TEXT("Data interface '%s' not found. Available: %s"),
            *InterfaceName, DINames.Num() > 0 ? *FString::Join(DINames, TEXT(", ")) : TEXT("none"));
        return false;
    }

    // Get value as string
    FString ValueStr;
    if (PropertyValue.IsValid() && PropertyValue->Type == EJson::String)
    {
        ValueStr = PropertyValue->AsString();
    }
    else
    {
        OutError = TEXT("Property value must be provided as a string");
        return false;
    }

    // Use reflection to set the property
    System->Modify();
    FoundDI->Modify();

    FProperty* Property = FoundDI->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property '%s' not found on data interface '%s'"), *PropertyName, *InterfaceName);
        return false;
    }

    // Handle different property types
    if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
    {
        // For object properties, load the asset
        UObject* LoadedAsset = LoadObject<UObject>(nullptr, *ValueStr);
        if (LoadedAsset)
        {
            ObjectProp->SetObjectPropertyValue_InContainer(FoundDI, LoadedAsset);
        }
        else
        {
            OutError = FString::Printf(TEXT("Failed to load asset: %s"), *ValueStr);
            return false;
        }
    }
    else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        bool BoolValue = ValueStr.ToBool() || ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("1"));
        BoolProp->SetPropertyValue_InContainer(FoundDI, BoolValue);
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        float FloatValue = FCString::Atof(*ValueStr);
        FloatProp->SetPropertyValue_InContainer(FoundDI, FloatValue);
    }
    else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
        double DoubleValue = FCString::Atod(*ValueStr);
        DoubleProp->SetPropertyValue_InContainer(FoundDI, DoubleValue);
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        int32 IntValue = FCString::Atoi(*ValueStr);
        IntProp->SetPropertyValue_InContainer(FoundDI, IntValue);
    }
    else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        StrProp->SetPropertyValue_InContainer(FoundDI, ValueStr);
    }
    else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        NameProp->SetPropertyValue_InContainer(FoundDI, FName(*ValueStr));
    }
    else
    {
        OutError = FString::Printf(TEXT("Unsupported property type for '%s'"), *PropertyName);
        return false;
    }

    UE_LOG(LogNiagaraService, Log, TEXT("Set data interface property '%s' to '%s' on '%s'"),
        *PropertyName, *ValueStr, *InterfaceName);

    // Mark dirty and refresh
    MarkSystemDirty(System);
    RefreshEditors(System);

    return true;
}

// ============================================================================
// Renderers (Feature 5)
// ============================================================================

bool FNiagaraService::AddRenderer(const FNiagaraRendererParams& Params, FString& OutRendererId, FString& OutError)
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

    const FNiagaraEmitterHandle& EmitterHandle = System->GetEmitterHandle(EmitterIndex);
    FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
    if (!EmitterData)
    {
        OutError = FString::Printf(TEXT("Could not get emitter data for '%s'"), *Params.EmitterName);
        return false;
    }

    // Get the emitter for modification
    UNiagaraEmitter* Emitter = EmitterHandle.GetInstance().Emitter;
    if (!Emitter)
    {
        OutError = TEXT("Could not get emitter instance");
        return false;
    }

    // Create the renderer
    UNiagaraRendererProperties* NewRenderer = CreateRendererByType(Params.RendererType, Emitter);
    if (!NewRenderer)
    {
        OutError = FString::Printf(TEXT("Failed to create renderer of type '%s'. Supported types: Sprite, Mesh, Ribbon, Light, Component"), *Params.RendererType);
        return false;
    }

    // Set custom name if provided
    if (!Params.RendererName.IsEmpty())
    {
        NewRenderer->Rename(*Params.RendererName);
    }

    // Mark for modification
    System->Modify();
    Emitter->Modify();

    // Add the renderer to the emitter
    Emitter->AddRenderer(NewRenderer, EmitterData->Version.VersionGuid);

    // Get renderer ID (use name or generate one)
    OutRendererId = NewRenderer->GetName();

    // Mark dirty and request recompile
    MarkSystemDirty(System);
    System->RequestCompile(false);

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Added renderer '%s' of type '%s' to emitter '%s'"),
        *OutRendererId, *Params.RendererType, *Params.EmitterName);

    return true;
}

bool FNiagaraService::SetRendererProperty(const FString& SystemPath, const FString& EmitterName, const FString& RendererName, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError)
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

    // Find the renderer by name
    UNiagaraRendererProperties* FoundRenderer = nullptr;
    for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
    {
        if (Renderer && (Renderer->GetName().Contains(RendererName, ESearchCase::IgnoreCase)))
        {
            FoundRenderer = Renderer;
            break;
        }
    }

    if (!FoundRenderer)
    {
        // List available renderers
        TArray<FString> RendererNames;
        for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
        {
            if (Renderer)
            {
                RendererNames.Add(Renderer->GetName());
            }
        }
        OutError = FString::Printf(TEXT("Renderer '%s' not found. Available: %s"),
            *RendererName, *FString::Join(RendererNames, TEXT(", ")));
        return false;
    }

    // Get value as string
    FString ValueStr;
    if (PropertyValue.IsValid() && PropertyValue->Type == EJson::String)
    {
        ValueStr = PropertyValue->AsString();
    }
    else
    {
        OutError = TEXT("Property value must be provided as a string");
        return false;
    }

    // Use reflection to set the property
    System->Modify();
    FoundRenderer->Modify();

    FProperty* Property = FoundRenderer->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        // Try common property name variations
        FString AltPropertyName = PropertyName;
        if (!PropertyName.StartsWith(TEXT("b")))
        {
            AltPropertyName = TEXT("b") + PropertyName;
        }
        Property = FoundRenderer->GetClass()->FindPropertyByName(FName(*AltPropertyName));
    }

    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property '%s' not found on renderer '%s'"), *PropertyName, *RendererName);
        return false;
    }

    // Handle different property types
    if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
    {
        // For object properties like Material, load the asset
        UObject* LoadedAsset = LoadObject<UObject>(nullptr, *ValueStr);
        if (LoadedAsset)
        {
            ObjectProp->SetObjectPropertyValue_InContainer(FoundRenderer, LoadedAsset);
        }
        else
        {
            OutError = FString::Printf(TEXT("Failed to load asset: %s"), *ValueStr);
            return false;
        }
    }
    else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        bool bValue = ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("1"));
        BoolProp->SetPropertyValue_InContainer(FoundRenderer, bValue);
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        FloatProp->SetPropertyValue_InContainer(FoundRenderer, FCString::Atof(*ValueStr));
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        IntProp->SetPropertyValue_InContainer(FoundRenderer, FCString::Atoi(*ValueStr));
    }
    else
    {
        OutError = FString::Printf(TEXT("Unsupported property type for '%s'"), *PropertyName);
        return false;
    }

    // Mark dirty and request recompile
    MarkSystemDirty(System);
    System->RequestCompile(false);

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set renderer property '%s' to '%s' on renderer '%s'"),
        *PropertyName, *ValueStr, *RendererName);

    return true;
}

// ============================================================================
// Level Integration (Feature 6)
// ============================================================================

ANiagaraActor* FNiagaraService::SpawnActor(const FNiagaraActorSpawnParams& Params, FString& OutActorName, FString& OutError)
{
    // Validate params
    if (!Params.IsValid(OutError))
    {
        return nullptr;
    }

    // Get editor world
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        OutError = TEXT("No valid editor world");
        return nullptr;
    }

    // Find the system
    UNiagaraSystem* System = FindSystem(Params.SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("Niagara System not found: %s"), *Params.SystemPath);
        return nullptr;
    }

    // Spawn the actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = FName(*Params.ActorName);
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ANiagaraActor* NiagaraActor = World->SpawnActor<ANiagaraActor>(
        ANiagaraActor::StaticClass(),
        Params.Location,
        Params.Rotation,
        SpawnParams
    );

    if (!NiagaraActor)
    {
        OutError = TEXT("Failed to spawn Niagara Actor");
        return nullptr;
    }

    // Set the system asset
    UNiagaraComponent* NiagaraComponent = NiagaraActor->GetNiagaraComponent();
    if (NiagaraComponent)
    {
        NiagaraComponent->SetAsset(System);
        NiagaraComponent->SetAutoActivate(Params.bAutoActivate);

        if (Params.bAutoActivate)
        {
            NiagaraComponent->Activate(true);
        }
    }

    // Set actor label
    NiagaraActor->SetActorLabel(Params.ActorName);
    OutActorName = NiagaraActor->GetActorLabel();

    UE_LOG(LogNiagaraService, Log, TEXT("Spawned Niagara Actor '%s' with system '%s' at (%f, %f, %f)"),
        *OutActorName, *Params.SystemPath,
        Params.Location.X, Params.Location.Y, Params.Location.Z);

    return NiagaraActor;
}

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

void FNiagaraService::AddSystemMetadata(UNiagaraSystem* System, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata) const
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

    // Modules - extract from each emitter's scripts
    if (bIncludeAll || Fields->Contains(TEXT("modules")))
    {
        TArray<TSharedPtr<FJsonValue>> EmitterModulesArray;

        for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
        {
            FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();
            if (!EmitterData)
            {
                continue;
            }

            TSharedPtr<FJsonObject> EmitterModuleObj = MakeShared<FJsonObject>();
            EmitterModuleObj->SetStringField(TEXT("emitter_name"), Handle.GetName().ToString());

            // Helper lambda to extract modules from a script
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

                        // Get the module script path if available
                        if (UNiagaraScript* FunctionScript = FunctionNode->FunctionScript)
                        {
                            ModuleObj->SetStringField(TEXT("script_path"), FunctionScript->GetPathName());
                        }

                        ModulesArray.Add(MakeShared<FJsonValueObject>(ModuleObj));
                    }
                }

                return ModulesArray;
            };

            // Extract from Spawn script
            TArray<TSharedPtr<FJsonValue>> SpawnModules = ExtractModulesFromScript(EmitterData->SpawnScriptProps.Script, TEXT("Spawn"));
            EmitterModuleObj->SetArrayField(TEXT("spawn_modules"), SpawnModules);

            // Extract from Update script
            TArray<TSharedPtr<FJsonValue>> UpdateModules = ExtractModulesFromScript(EmitterData->UpdateScriptProps.Script, TEXT("Update"));
            EmitterModuleObj->SetArrayField(TEXT("update_modules"), UpdateModules);

            EmitterModulesArray.Add(MakeShared<FJsonValueObject>(EmitterModuleObj));
        }

        OutMetadata->SetArrayField(TEXT("modules_by_emitter"), EmitterModulesArray);
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

    // Use Asset Tools to duplicate
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    TArray<UObject*> ObjectsToDuplicate;
    ObjectsToDuplicate.Add(SourceSystem);

    TArray<FAssetRenameData> RenameData;
    FAssetRenameData& Data = RenameData.AddDefaulted_GetRef();
    Data.Asset = SourceSystem;
    Data.NewPackagePath = DestFolder;
    Data.NewName = NewName;

    // Duplicate the asset
    TArray<UObject*> DuplicatedObjects;
    ObjectTools::DuplicateObjects(ObjectsToDuplicate, TEXT(""), DestFolder, false, &DuplicatedObjects);

    if (DuplicatedObjects.Num() == 0)
    {
        OutError = TEXT("Failed to duplicate system");
        return false;
    }

    UNiagaraSystem* NewSystem = Cast<UNiagaraSystem>(DuplicatedObjects[0]);
    if (!NewSystem)
    {
        OutError = TEXT("Duplicated object is not a Niagara System");
        return false;
    }

    // Rename to desired name if not already correct
    if (NewSystem->GetName() != NewName)
    {
        NewSystem->Rename(*NewName, NewSystem->GetOuter());
    }

    // Save the new asset
    if (!SaveAsset(NewSystem, OutError))
    {
        return false;
    }

    OutNewPath = NewSystem->GetOutermost()->GetName();

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewSystem);

    UE_LOG(LogNiagaraService, Log, TEXT("Duplicated Niagara System from '%s' to '%s'"),
        *SourcePath, *OutNewPath);

    return true;
}
