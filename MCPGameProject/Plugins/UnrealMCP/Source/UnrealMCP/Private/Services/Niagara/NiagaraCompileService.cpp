// NiagaraCompileService.cpp - Compilation
// CompileAsset

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraScriptSource.h"

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
        bool bIsValid = System->IsValid();

        // Always collect warnings (even on successful compilation)
        TArray<FString> WarningMessages;

        // Helper lambda to extract module warnings (deprecated, experimental, notes)
        auto ExtractModuleWarningsForValidation = [&WarningMessages](UNiagaraSystem* InSystem)
        {
            for (const FNiagaraEmitterHandle& Handle : InSystem->GetEmitterHandles())
            {
                FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();
                if (!EmitterData)
                {
                    continue;
                }

                auto CheckScriptModules = [&WarningMessages, &Handle](UNiagaraScript* Script, const FString& StageName)
                {
                    if (!Script)
                    {
                        return;
                    }

                    UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
                    if (!ScriptSource || !ScriptSource->NodeGraph)
                    {
                        return;
                    }

                    for (UEdGraphNode* Node : ScriptSource->NodeGraph->Nodes)
                    {
                        UNiagaraNodeFunctionCall* FunctionNode = Cast<UNiagaraNodeFunctionCall>(Node);
                        if (!FunctionNode || !FunctionNode->FunctionScript)
                        {
                            continue;
                        }

                        FString ModuleName = FunctionNode->GetFunctionName();
                        FVersionedNiagaraScriptData* ScriptData = FunctionNode->GetScriptData();

                        if (ScriptData)
                        {
                            // Check for deprecation
                            if (ScriptData->bDeprecated)
                            {
                                FString DeprecationMsg = FString::Printf(TEXT("[%s] %s Module '%s' [DEPRECATED]"),
                                    *Handle.GetName().ToString(),
                                    *StageName,
                                    *ModuleName);

                                if (!ScriptData->DeprecationMessage.IsEmpty())
                                {
                                    DeprecationMsg += TEXT(": ") + ScriptData->DeprecationMessage.ToString();
                                }

                                if (ScriptData->DeprecationRecommendation)
                                {
                                    DeprecationMsg += FString::Printf(TEXT(" Suggested: %s"),
                                        *ScriptData->DeprecationRecommendation->GetPathName());
                                }

                                WarningMessages.Add(DeprecationMsg);
                            }

                            // Check for experimental
                            if (ScriptData->bExperimental)
                            {
                                FString ExperimentalMsg = FString::Printf(TEXT("[%s] %s Module '%s' [EXPERIMENTAL]"),
                                    *Handle.GetName().ToString(),
                                    *StageName,
                                    *ModuleName);

                                if (!ScriptData->ExperimentalMessage.IsEmpty())
                                {
                                    ExperimentalMsg += TEXT(": ") + ScriptData->ExperimentalMessage.ToString();
                                }

                                WarningMessages.Add(ExperimentalMsg);
                            }

                            // Check for note messages (general warnings)
                            if (!ScriptData->NoteMessage.IsEmpty())
                            {
                                WarningMessages.Add(FString::Printf(TEXT("[%s] %s Module '%s' [Note]: %s"),
                                    *Handle.GetName().ToString(),
                                    *StageName,
                                    *ModuleName,
                                    *ScriptData->NoteMessage.ToString()));
                            }
                        }
                    }
                };

                CheckScriptModules(EmitterData->SpawnScriptProps.Script, TEXT("Spawn"));
                CheckScriptModules(EmitterData->UpdateScriptProps.Script, TEXT("Update"));
            }
        };

        // Collect warnings
        ExtractModuleWarningsForValidation(System);

        if (bIsValid)
        {
            // Report warnings even on success
            if (WarningMessages.Num() > 0)
            {
                OutError = TEXT("Compilation successful with warnings:\n") + FString::Join(WarningMessages, TEXT("\n"));
            }
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

            // Add module warnings (already collected above)
            if (WarningMessages.Num() > 0)
            {
                ErrorMessages.Add(TEXT("\n--- Module Warnings ---"));
                ErrorMessages.Append(WarningMessages);
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
