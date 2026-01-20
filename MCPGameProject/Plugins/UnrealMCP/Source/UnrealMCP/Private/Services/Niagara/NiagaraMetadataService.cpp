// NiagaraMetadataService.cpp - Metadata queries
// GetMetadata, GetEmitterModules
// Note: GetModuleInputs is in NiagaraModuleInputsQuery.cpp

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraScriptSource.h"
#include "NiagaraTypes.h"
#include "EdGraphSchema_Niagara.h"

bool FNiagaraService::GetMetadata(const FString& AssetPath, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata, const FString& EmitterName, const FString& Stage)
{
    OutMetadata = MakeShared<FJsonObject>();

    // Try to load as system first
    UNiagaraSystem* System = FindSystem(AssetPath);
    if (System)
    {
        OutMetadata->SetStringField(TEXT("asset_type"), TEXT("NiagaraSystem"));
        OutMetadata->SetStringField(TEXT("asset_path"), AssetPath);
        OutMetadata->SetStringField(TEXT("asset_name"), System->GetName());
        AddSystemMetadata(System, Fields, OutMetadata, EmitterName, Stage);
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

// Helper function to get parameter map input pin
namespace
{
    UEdGraphPin* GetParameterMapInputPin(UNiagaraNode& Node)
    {
        TArray<UEdGraphPin*> InputPins;
        Node.GetInputPins(InputPins);

        for (UEdGraphPin* Pin : InputPins)
        {
            if (Pin)
            {
                const UEdGraphSchema_Niagara* NiagaraSchema = Cast<UEdGraphSchema_Niagara>(Pin->GetSchema());
                if (NiagaraSchema)
                {
                    FNiagaraTypeDefinition PinDefinition = NiagaraSchema->PinToTypeDefinition(Pin);
                    if (PinDefinition == FNiagaraTypeDefinition::GetParameterMapDef())
                    {
                        return Pin;
                    }
                }
            }
        }
        return nullptr;
    }
}

bool FNiagaraService::GetEmitterModules(const FString& SystemPath, const FString& EmitterName, TSharedPtr<FJsonObject>& OutModules)
{
    OutModules = MakeShared<FJsonObject>();

    // Find the system
    UNiagaraSystem* System = FindSystem(SystemPath);
    if (!System)
    {
        OutModules->SetBoolField(TEXT("success"), false);
        OutModules->SetStringField(TEXT("error"), FString::Printf(TEXT("System not found: %s"), *SystemPath));
        return false;
    }

    // Find the emitter handle by name
    int32 EmitterIndex = FindEmitterHandleIndex(System, EmitterName);
    if (EmitterIndex == INDEX_NONE)
    {
        OutModules->SetBoolField(TEXT("success"), false);
        OutModules->SetStringField(TEXT("error"), FString::Printf(TEXT("Emitter '%s' not found in system"), *EmitterName));
        return false;
    }

    const FNiagaraEmitterHandle& EmitterHandle = System->GetEmitterHandle(EmitterIndex);
    FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
    if (!EmitterData)
    {
        OutModules->SetBoolField(TEXT("success"), false);
        OutModules->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not get emitter data for '%s'"), *EmitterName));
        return false;
    }

    // Helper lambda to extract modules from a script by tracing from output node
    // This properly identifies which modules belong to which stage
    auto ExtractModulesFromScript = [](UNiagaraScript* Script, ENiagaraScriptUsage ExpectedUsage) -> TArray<TSharedPtr<FJsonValue>>
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

        UNiagaraGraph* Graph = ScriptSource->NodeGraph;

        // Find the output node for this script's usage
        UNiagaraNodeOutput* OutputNode = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            UNiagaraNodeOutput* TestNode = Cast<UNiagaraNodeOutput>(Node);
            if (TestNode && TestNode->GetUsage() == ExpectedUsage)
            {
                OutputNode = TestNode;
                break;
            }
        }

        if (!OutputNode)
        {
            return ModulesArray;
        }

        // Trace backwards from output node through parameter map chain to get ordered modules
        TArray<TPair<FString, UNiagaraNodeFunctionCall*>> OrderedModules;
        UNiagaraNode* CurrentNode = OutputNode;
        while (CurrentNode != nullptr)
        {
            UEdGraphPin* InputPin = GetParameterMapInputPin(*CurrentNode);
            if (InputPin != nullptr && InputPin->LinkedTo.Num() == 1)
            {
                UNiagaraNode* PreviousNode = Cast<UNiagaraNode>(InputPin->LinkedTo[0]->GetOwningNode());
                UNiagaraNodeFunctionCall* ModuleNode = Cast<UNiagaraNodeFunctionCall>(PreviousNode);
                if (ModuleNode != nullptr)
                {
                    OrderedModules.Insert(TPair<FString, UNiagaraNodeFunctionCall*>(ModuleNode->GetFunctionName(), ModuleNode), 0);
                }
                CurrentNode = PreviousNode;
            }
            else
            {
                CurrentNode = nullptr;
            }
        }

        // Convert to JSON array with full module details
        int32 ModuleIndex = 0;
        for (const auto& ModulePair : OrderedModules)
        {
            TSharedPtr<FJsonObject> ModuleObj = MakeShared<FJsonObject>();
            ModuleObj->SetStringField(TEXT("name"), ModulePair.Key);
            ModuleObj->SetNumberField(TEXT("index"), ModuleIndex);
            ModuleObj->SetBoolField(TEXT("enabled"), ModulePair.Value->IsNodeEnabled());

            // Get the module script path if available
            if (ModulePair.Value->FunctionScript)
            {
                ModuleObj->SetStringField(TEXT("script_path"), ModulePair.Value->FunctionScript->GetPathName());
            }

            ModulesArray.Add(MakeShared<FJsonValueObject>(ModuleObj));
            ModuleIndex++;
        }

        return ModulesArray;
    };

    // Build the stages object
    TSharedPtr<FJsonObject> StagesObj = MakeShared<FJsonObject>();

    // Get Emitter Spawn modules (emitter-level initialization)
    TArray<TSharedPtr<FJsonValue>> EmitterSpawnModules = ExtractModulesFromScript(
        EmitterData->EmitterSpawnScriptProps.Script,
        ENiagaraScriptUsage::EmitterSpawnScript
    );
    StagesObj->SetArrayField(TEXT("EmitterSpawn"), EmitterSpawnModules);

    // Get Emitter Update modules (spawn rate, spawn burst, etc.)
    TArray<TSharedPtr<FJsonValue>> EmitterUpdateModules = ExtractModulesFromScript(
        EmitterData->EmitterUpdateScriptProps.Script,
        ENiagaraScriptUsage::EmitterUpdateScript
    );
    StagesObj->SetArrayField(TEXT("EmitterUpdate"), EmitterUpdateModules);

    // Get Particle Spawn modules
    TArray<TSharedPtr<FJsonValue>> SpawnModules = ExtractModulesFromScript(
        EmitterData->SpawnScriptProps.Script,
        ENiagaraScriptUsage::ParticleSpawnScript
    );
    StagesObj->SetArrayField(TEXT("ParticleSpawn"), SpawnModules);

    // Get Particle Update modules
    TArray<TSharedPtr<FJsonValue>> UpdateModules = ExtractModulesFromScript(
        EmitterData->UpdateScriptProps.Script,
        ENiagaraScriptUsage::ParticleUpdateScript
    );
    StagesObj->SetArrayField(TEXT("ParticleUpdate"), UpdateModules);

    // Get Event handlers if any
    TArray<TSharedPtr<FJsonValue>> EventModulesArray;
    for (const FNiagaraEventScriptProperties& EventProps : EmitterData->GetEventHandlers())
    {
        TArray<TSharedPtr<FJsonValue>> EventModules = ExtractModulesFromScript(
            EventProps.Script,
            ENiagaraScriptUsage::ParticleEventScript
        );
        for (const TSharedPtr<FJsonValue>& Module : EventModules)
        {
            EventModulesArray.Add(Module);
        }
    }
    if (EventModulesArray.Num() > 0)
    {
        StagesObj->SetArrayField(TEXT("Event"), EventModulesArray);
    }

    // Build the response
    OutModules->SetBoolField(TEXT("success"), true);
    OutModules->SetStringField(TEXT("emitter_name"), EmitterName);
    OutModules->SetStringField(TEXT("system_path"), SystemPath);
    OutModules->SetObjectField(TEXT("stages"), StagesObj);

    // Add summary counts
    int32 TotalModules = EmitterSpawnModules.Num() + EmitterUpdateModules.Num() + SpawnModules.Num() + UpdateModules.Num() + EventModulesArray.Num();
    OutModules->SetNumberField(TEXT("total_module_count"), TotalModules);
    OutModules->SetNumberField(TEXT("emitter_spawn_count"), EmitterSpawnModules.Num());
    OutModules->SetNumberField(TEXT("emitter_update_count"), EmitterUpdateModules.Num());
    OutModules->SetNumberField(TEXT("spawn_count"), SpawnModules.Num());
    OutModules->SetNumberField(TEXT("update_count"), UpdateModules.Num());
    if (EventModulesArray.Num() > 0)
    {
        OutModules->SetNumberField(TEXT("event_count"), EventModulesArray.Num());
    }

    return true;
}
