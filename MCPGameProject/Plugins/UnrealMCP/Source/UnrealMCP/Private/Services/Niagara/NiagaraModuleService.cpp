// NiagaraModuleService.cpp - Module System (Feature 2)
// AddModule, SearchModules, SetModuleInput

#include "Services/NiagaraService.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraScriptSource.h"
#include "NiagaraTypes.h"
#include "NiagaraCommon.h"
#include "NiagaraEditorModule.h"
#include "EdGraphSchema_Niagara.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"

// Helper function to find ParameterMap pin from a collection of pins
static UEdGraphPin* GetParameterMapPinFromArray(const TArray<UEdGraphPin*>& Pins)
{
    for (UEdGraphPin* Pin : Pins)
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

// Helper function to get parameter map input pin (replicates FNiagaraStackGraphUtilities::GetParameterMapInputPin logic)
// This is needed because the original function is not exported from the NiagaraEditor module
static UEdGraphPin* GetParameterMapInputPinLocal(UNiagaraNode& Node)
{
    TArray<UEdGraphPin*> InputPins;
    Node.GetInputPins(InputPins);
    return GetParameterMapPinFromArray(InputPins);
}

// Helper function to get parameter map output pin (replicates FNiagaraStackGraphUtilities::GetParameterMapOutputPin logic)
static UEdGraphPin* GetParameterMapOutputPinLocal(UNiagaraNode& Node)
{
    TArray<UEdGraphPin*> OutputPins;
    Node.GetOutputPins(OutputPins);
    return GetParameterMapPinFromArray(OutputPins);
}

// ============================================================================
// Module System (Feature 2)
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
    case ENiagaraScriptUsage::EmitterSpawnScript:
        Script = EmitterData->EmitterSpawnScriptProps.Script;
        break;
    case ENiagaraScriptUsage::EmitterUpdateScript:
        Script = EmitterData->EmitterUpdateScriptProps.Script;
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

    // Calculate the correct insertion index based on module dependencies
    int32 TargetIndex = Params.Index >= 0 ? Params.Index : INDEX_NONE;

    // If user didn't specify an index, check module dependencies to find the correct position
    if (TargetIndex == INDEX_NONE)
    {
        FVersionedNiagaraScriptData* ModuleScriptData = ModuleScript->GetLatestScriptData();
        if (ModuleScriptData && ModuleScriptData->RequiredDependencies.Num() > 0)
        {
            // Use the same algorithm as FNiagaraStackGraphUtilities::GetOrderedModuleNodes
            // to get modules in correct execution order by tracing parameter map connections
            TArray<UNiagaraNodeFunctionCall*> OrderedModules;
            UNiagaraNode* PreviousNode = OutputNode;
            while (PreviousNode != nullptr)
            {
                UEdGraphPin* PreviousNodeInputPin = GetParameterMapInputPinLocal(*PreviousNode);
                if (PreviousNodeInputPin != nullptr && PreviousNodeInputPin->LinkedTo.Num() == 1)
                {
                    UNiagaraNode* CurrentNode = Cast<UNiagaraNode>(PreviousNodeInputPin->LinkedTo[0]->GetOwningNode());
                    UNiagaraNodeFunctionCall* ModuleNode = Cast<UNiagaraNodeFunctionCall>(CurrentNode);
                    if (ModuleNode != nullptr)
                    {
                        OrderedModules.Insert(ModuleNode, 0);  // Insert at front (building in reverse)
                    }
                    PreviousNode = CurrentNode;
                }
                else
                {
                    PreviousNode = nullptr;
                }
            }

            // Build array with module and its provided dependencies
            TArray<TPair<UNiagaraNodeFunctionCall*, TArray<FName>>> ExistingModulesWithDependencies;
            for (UNiagaraNodeFunctionCall* StackModule : OrderedModules)
            {
                TArray<FName> ProvidedDeps;
                if (FVersionedNiagaraScriptData* StackModuleData = StackModule->GetScriptData())
                {
                    ProvidedDeps = StackModuleData->ProvidedDependencies;
                }
                ExistingModulesWithDependencies.Add(TPair<UNiagaraNodeFunctionCall*, TArray<FName>>(StackModule, ProvidedDeps));

                // Debug: log each module and its provided dependencies
                FString ProvidedDepsStr;
                for (const FName& DepName : ProvidedDeps)
                {
                    ProvidedDepsStr += DepName.ToString() + TEXT(", ");
                }
                UE_LOG(LogNiagaraService, Log, TEXT("  [%d] Module '%s' provides: [%s]"),
                    ExistingModulesWithDependencies.Num() - 1, *StackModule->GetFunctionName(), *ProvidedDepsStr);
            }

            UE_LOG(LogNiagaraService, Log, TEXT("Module '%s' has %d required dependencies"), *ModuleScriptName, ModuleScriptData->RequiredDependencies.Num());

            // Check each required dependency
            for (const FNiagaraModuleDependency& Dependency : ModuleScriptData->RequiredDependencies)
            {
                UE_LOG(LogNiagaraService, Log, TEXT("  Checking dependency Id='%s' Type=%d"), *Dependency.Id.ToString(), (int32)Dependency.Type);

                if (Dependency.Type == ENiagaraModuleDependencyType::PostDependency)
                {
                    // PostDependency means the dependency provider must come AFTER us
                    // So we need to insert BEFORE the dependency provider
                    for (int32 i = 0; i < ExistingModulesWithDependencies.Num(); i++)
                    {
                        if (ExistingModulesWithDependencies[i].Value.Contains(Dependency.Id))
                        {
                            // Found the dependency provider - insert before it
                            if (TargetIndex == INDEX_NONE || i < TargetIndex)
                            {
                                TargetIndex = i;
                            }
                            UE_LOG(LogNiagaraService, Log, TEXT("Module '%s' has PostDependency on '%s', inserting at index %d (before '%s')"),
                                *ModuleScriptName, *Dependency.Id.ToString(), TargetIndex,
                                *ExistingModulesWithDependencies[i].Key->GetFunctionName());
                            break;
                        }
                    }
                }
                else if (Dependency.Type == ENiagaraModuleDependencyType::PreDependency)
                {
                    // PreDependency means the dependency provider must come BEFORE us
                    // So we need to insert AFTER the dependency provider
                    for (int32 i = ExistingModulesWithDependencies.Num() - 1; i >= 0; i--)
                    {
                        if (ExistingModulesWithDependencies[i].Value.Contains(Dependency.Id))
                        {
                            // Found the dependency provider - insert after it
                            int32 InsertAfterIndex = i + 1;
                            if (TargetIndex == INDEX_NONE || InsertAfterIndex > TargetIndex)
                            {
                                TargetIndex = InsertAfterIndex;
                            }
                            UE_LOG(LogNiagaraService, Log, TEXT("Module '%s' has PreDependency on '%s', inserting at index %d (after '%s')"),
                                *ModuleScriptName, *Dependency.Id.ToString(), TargetIndex,
                                *ExistingModulesWithDependencies[i].Key->GetFunctionName());
                            break;
                        }
                    }
                }
            }
        }
    }

    // Add the module using the stack graph utilities
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

    // Mark system dirty - DON'T trigger recompilation here
    // Recompilation will happen when compile_niagara_asset is called explicitly
    // This prevents invalidating rapid iteration parameters that were set earlier
    MarkSystemDirty(System);

    // Notify graph of changes without full recompilation
    Graph->NotifyGraphChanged();

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Added module '%s' to emitter '%s' stage '%s' with ID: %s (deferred compilation)"),
        *Params.ModulePath, *Params.EmitterName, *Params.Stage, *OutModuleId);

    return true;
}

bool FNiagaraService::RemoveModule(const FNiagaraModuleRemoveParams& Params, FString& OutError)
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
    case ENiagaraScriptUsage::EmitterSpawnScript:
        Script = EmitterData->EmitterSpawnScriptProps.Script;
        break;
    case ENiagaraScriptUsage::EmitterUpdateScript:
        Script = EmitterData->EmitterUpdateScriptProps.Script;
        break;
    default:
        OutError = FString::Printf(TEXT("Unsupported stage '%s' for module removal"), *Params.Stage);
        return false;
    }

    if (!Script)
    {
        OutError = FString::Printf(TEXT("Script not found for stage '%s'"), *Params.Stage);
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

    // Find the module node by name - prioritize exact matches
    UNiagaraNodeFunctionCall* ModuleNode = nullptr;
    UNiagaraNodeFunctionCall* PartialMatchNode = nullptr;
    FString NormalizedSearchName = Params.ModuleName.Replace(TEXT(" "), TEXT(""));
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UNiagaraNodeFunctionCall* FunctionNode = Cast<UNiagaraNodeFunctionCall>(Node);
        if (FunctionNode)
        {
            FString NodeName = FunctionNode->GetFunctionName();
            FString NormalizedNodeName = NodeName.Replace(TEXT(" "), TEXT(""));

            // Exact match takes priority
            if (NormalizedNodeName.Equals(NormalizedSearchName, ESearchCase::IgnoreCase))
            {
                ModuleNode = FunctionNode;
                break;
            }
            // Track partial match as fallback
            if (!PartialMatchNode && NormalizedNodeName.Contains(NormalizedSearchName, ESearchCase::IgnoreCase))
            {
                PartialMatchNode = FunctionNode;
            }
        }
    }
    // Use partial match only if no exact match found
    if (!ModuleNode && PartialMatchNode)
    {
        ModuleNode = PartialMatchNode;
    }

    if (!ModuleNode)
    {
        OutError = FString::Printf(TEXT("Module '%s' not found in stage '%s'"), *Params.ModuleName, *Params.Stage);
        return false;
    }

    // Mark for modification
    System->Modify();
    Graph->Modify();
    ModuleNode->Modify();

    // Get the module's input and output ParameterMap pins
    UEdGraphPin* ModuleInputPin = GetParameterMapInputPinLocal(*ModuleNode);
    UEdGraphPin* ModuleOutputPin = GetParameterMapOutputPinLocal(*ModuleNode);

    if (!ModuleInputPin || !ModuleOutputPin)
    {
        OutError = FString::Printf(TEXT("Module '%s' has invalid ParameterMap pins"), *Params.ModuleName);
        return false;
    }

    // Find the previous node's output pin (what connects to our input)
    UEdGraphPin* PreviousOutputPin = nullptr;
    if (ModuleInputPin->LinkedTo.Num() > 0)
    {
        PreviousOutputPin = ModuleInputPin->LinkedTo[0];
    }

    // Collect the next nodes' input pins (what connects to our output)
    TArray<UEdGraphPin*> NextInputPins;
    for (UEdGraphPin* LinkedPin : ModuleOutputPin->LinkedTo)
    {
        NextInputPins.Add(LinkedPin);
    }

    // Break all links from the module (disconnect from chain)
    ModuleInputPin->BreakAllPinLinks();
    ModuleOutputPin->BreakAllPinLinks();

    // Reconnect: previous output → next inputs (bypass the removed module)
    if (PreviousOutputPin)
    {
        for (UEdGraphPin* NextInputPin : NextInputPins)
        {
            PreviousOutputPin->MakeLinkTo(NextInputPin);
        }
    }

    // Now remove the module node from the graph (links already broken)
    bool bRemoved = Graph->RemoveNode(ModuleNode, /*bBreakAllLinks=*/false, /*bAlwaysMarkDirty=*/true);

    if (!bRemoved)
    {
        OutError = FString::Printf(TEXT("Failed to remove module '%s' from graph"), *Params.ModuleName);
        return false;
    }

    // Mark system dirty
    MarkSystemDirty(System);

    // Notify graph of changes
    Graph->NotifyGraphChanged();

    // Request compilation and wait for it
    System->RequestCompile(false);
    System->WaitForCompilationComplete();

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Removed module '%s' from emitter '%s' stage '%s'"),
        *Params.ModuleName, *Params.EmitterName, *Params.Stage);

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

        // Apply search filter - split query into words and match ALL
        if (!SearchQuery.IsEmpty())
        {
            TArray<FString> SearchWords;
            SearchQuery.ParseIntoArray(SearchWords, TEXT(" "), true);

            if (SearchWords.Num() > 0)
            {
                bool bAllWordsFound = true;
                for (const FString& Word : SearchWords)
                {
                    if (!AssetName.Contains(Word, ESearchCase::IgnoreCase))
                    {
                        bAllWordsFound = false;
                        break;
                    }
                }

                if (!bAllWordsFound)
                {
                    continue;
                }
            }
        }

        TSharedPtr<FJsonObject> ModuleInfo = MakeShared<FJsonObject>();
        ModuleInfo->SetStringField(TEXT("name"), AssetName);
        ModuleInfo->SetStringField(TEXT("path"), Asset.GetObjectPathString());

        OutModules.Add(ModuleInfo);
        Count++;
    }

    return true;
}

// SetModuleInput implementation is in NiagaraModuleInputService.cpp

bool FNiagaraService::MoveModule(const FNiagaraModuleMoveParams& Params, FString& OutError)
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
    case ENiagaraScriptUsage::EmitterSpawnScript:
        Script = EmitterData->EmitterSpawnScriptProps.Script;
        break;
    case ENiagaraScriptUsage::EmitterUpdateScript:
        Script = EmitterData->EmitterUpdateScriptProps.Script;
        break;
    default:
        OutError = FString::Printf(TEXT("Unsupported stage '%s' for module move"), *Params.Stage);
        return false;
    }

    if (!Script)
    {
        OutError = FString::Printf(TEXT("Script not found for stage '%s'"), *Params.Stage);
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

    // Find the module node by name - prioritize exact matches
    UNiagaraNodeFunctionCall* ModuleNode = nullptr;
    UNiagaraNodeFunctionCall* PartialMatchNode = nullptr;
    FString NormalizedSearchName = Params.ModuleName.Replace(TEXT(" "), TEXT(""));
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UNiagaraNodeFunctionCall* FunctionNode = Cast<UNiagaraNodeFunctionCall>(Node);
        if (FunctionNode)
        {
            FString NodeName = FunctionNode->GetFunctionName();
            FString NormalizedNodeName = NodeName.Replace(TEXT(" "), TEXT(""));

            // Exact match takes priority
            if (NormalizedNodeName.Equals(NormalizedSearchName, ESearchCase::IgnoreCase))
            {
                ModuleNode = FunctionNode;
                break;
            }
            // Track partial match as fallback
            if (!PartialMatchNode && NormalizedNodeName.Contains(NormalizedSearchName, ESearchCase::IgnoreCase))
            {
                PartialMatchNode = FunctionNode;
            }
        }
    }
    // Use partial match only if no exact match found
    if (!ModuleNode && PartialMatchNode)
    {
        ModuleNode = PartialMatchNode;
    }

    if (!ModuleNode)
    {
        OutError = FString::Printf(TEXT("Module '%s' not found in stage '%s'"), *Params.ModuleName, *Params.Stage);
        return false;
    }

    // Mark system as modified before making changes
    System->Modify();
    Graph->Modify();

    // Manual module move implementation:
    // 1. Get ordered list of modules in the chain
    // 2. Find output node for this stage
    // 3. Remove module from current position (reconnect neighbors)
    // 4. Insert at new position

    // Find the output node for this script
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

    // Get ordered modules by tracing parameter map connections backwards from output
    TArray<UNiagaraNodeFunctionCall*> OrderedModules;
    UNiagaraNode* CurrentNode = OutputNode;
    while (CurrentNode != nullptr)
    {
        UEdGraphPin* InputPin = GetParameterMapInputPinLocal(*CurrentNode);
        if (InputPin != nullptr && InputPin->LinkedTo.Num() == 1)
        {
            UNiagaraNode* PreviousNode = Cast<UNiagaraNode>(InputPin->LinkedTo[0]->GetOwningNode());
            UNiagaraNodeFunctionCall* ModuleCallNode = Cast<UNiagaraNodeFunctionCall>(PreviousNode);
            if (ModuleCallNode != nullptr)
            {
                OrderedModules.Insert(ModuleCallNode, 0);  // Insert at front (we're walking backwards)
            }
            CurrentNode = PreviousNode;
        }
        else
        {
            CurrentNode = nullptr;
        }
    }

    // Find current index of module to move
    int32 CurrentIndex = OrderedModules.IndexOfByKey(ModuleNode);
    if (CurrentIndex == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("Module '%s' not found in ordered module list"), *Params.ModuleName);
        return false;
    }

    // Validate new index
    int32 TargetIndex = Params.NewIndex;
    if (TargetIndex < 0 || TargetIndex >= OrderedModules.Num())
    {
        OutError = FString::Printf(TEXT("Invalid target index %d. Valid range is 0-%d"), TargetIndex, OrderedModules.Num() - 1);
        return false;
    }

    // If moving to same position, nothing to do
    if (CurrentIndex == TargetIndex)
    {
        UE_LOG(LogNiagaraService, Log, TEXT("Module '%s' is already at index %d"), *Params.ModuleName, TargetIndex);
        return true;
    }

    // Step 1: Get module's parameter map input and output pins
    UEdGraphPin* ModuleInputPin = GetParameterMapInputPinLocal(*ModuleNode);
    UEdGraphPin* ModuleOutputPin = nullptr;
    TArray<UEdGraphPin*> OutputPins;
    ModuleNode->GetOutputPins(OutputPins);
    for (UEdGraphPin* Pin : OutputPins)
    {
        if (Pin)
        {
            const UEdGraphSchema_Niagara* NiagaraSchema = Cast<UEdGraphSchema_Niagara>(Pin->GetSchema());
            if (NiagaraSchema)
            {
                FNiagaraTypeDefinition PinDef = NiagaraSchema->PinToTypeDefinition(Pin);
                if (PinDef == FNiagaraTypeDefinition::GetParameterMapDef())
                {
                    ModuleOutputPin = Pin;
                    break;
                }
            }
        }
    }

    if (!ModuleInputPin || !ModuleOutputPin)
    {
        OutError = TEXT("Could not find parameter map pins on module");
        return false;
    }

    // Step 2: Get the nodes connected before and after the module
    UEdGraphPin* PreviousOutputPin = (ModuleInputPin->LinkedTo.Num() > 0) ? ModuleInputPin->LinkedTo[0] : nullptr;
    UEdGraphPin* NextInputPin = (ModuleOutputPin->LinkedTo.Num() > 0) ? ModuleOutputPin->LinkedTo[0] : nullptr;

    // Step 3: Disconnect module from chain
    if (PreviousOutputPin)
    {
        ModuleInputPin->BreakLinkTo(PreviousOutputPin);
    }
    if (NextInputPin)
    {
        ModuleOutputPin->BreakLinkTo(NextInputPin);
    }

    // Step 4: Reconnect the gap (connect previous directly to next)
    if (PreviousOutputPin && NextInputPin)
    {
        const UEdGraphSchema* Schema = Graph->GetSchema();
        Schema->TryCreateConnection(PreviousOutputPin, NextInputPin);
    }

    // Step 5: Reorder the array
    OrderedModules.RemoveAt(CurrentIndex);
    OrderedModules.Insert(ModuleNode, TargetIndex);

    // Step 6: Find insertion point in chain
    // Get the pin to connect our module's INPUT to (output from module before us)
    UEdGraphPin* InsertAfterOutputPin = nullptr;
    if (TargetIndex > 0)
    {
        UNiagaraNodeFunctionCall* NodeBefore = OrderedModules[TargetIndex - 1];
        TArray<UEdGraphPin*> BeforeOutputPins;
        NodeBefore->GetOutputPins(BeforeOutputPins);
        for (UEdGraphPin* Pin : BeforeOutputPins)
        {
            if (Pin)
            {
                const UEdGraphSchema_Niagara* NiagaraSchema = Cast<UEdGraphSchema_Niagara>(Pin->GetSchema());
                if (NiagaraSchema && NiagaraSchema->PinToTypeDefinition(Pin) == FNiagaraTypeDefinition::GetParameterMapDef())
                {
                    InsertAfterOutputPin = Pin;
                    break;
                }
            }
        }
    }

    // Get the pin to connect our module's OUTPUT to (input of module after us, or output node)
    UEdGraphPin* InsertBeforeInputPin = nullptr;
    if (TargetIndex < OrderedModules.Num() - 1)
    {
        UNiagaraNodeFunctionCall* NodeAfter = OrderedModules[TargetIndex + 1];
        InsertBeforeInputPin = GetParameterMapInputPinLocal(*NodeAfter);
    }
    else
    {
        // Last module connects to output node
        InsertBeforeInputPin = GetParameterMapInputPinLocal(*OutputNode);
    }

    // Step 7: Break the connection at the insertion point
    if (InsertAfterOutputPin && InsertBeforeInputPin)
    {
        InsertAfterOutputPin->BreakLinkTo(InsertBeforeInputPin);
    }

    // Step 8: Insert module at new position
    const UEdGraphSchema* Schema = Graph->GetSchema();
    if (InsertAfterOutputPin)
    {
        Schema->TryCreateConnection(InsertAfterOutputPin, ModuleInputPin);
    }
    if (InsertBeforeInputPin)
    {
        Schema->TryCreateConnection(ModuleOutputPin, InsertBeforeInputPin);
    }

    // Mark system dirty
    MarkSystemDirty(System);

    // Notify graph of changes
    Graph->NotifyGraphChanged();

    // Request compilation
    System->RequestCompile(false);
    System->WaitForCompilationComplete();

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Moved module '%s' to index %d in emitter '%s' stage '%s'"),
        *Params.ModuleName, Params.NewIndex, *Params.EmitterName, *Params.Stage);

    return true;
}
