// NiagaraLinkedInputService.cpp - Linked Input Support for Niagara Modules
// SetModuleLinkedInput - binds module inputs to particle attributes

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraGraph.h"
#include "NiagaraScriptVariable.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeParameterMapSet.h"
#include "NiagaraNodeParameterMapGet.h"
#include "NiagaraScriptSource.h"
#include "NiagaraTypes.h"
#include "NiagaraCommon.h"
#include "EdGraphSchema_Niagara.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"

// ============================================================================
// Helper to find module node by name
// ============================================================================

static UNiagaraNodeFunctionCall* FindModuleNodeByNameForLinked(
    UNiagaraGraph* Graph,
    const FString& ModuleName)
{
    FString NormalizedSearchName = ModuleName.Replace(TEXT(" "), TEXT(""));
    UNiagaraNodeFunctionCall* PartialMatchNode = nullptr;

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
                return FunctionNode;
            }
            // Track partial match as fallback
            if (!PartialMatchNode && NormalizedNodeName.Contains(NormalizedSearchName, ESearchCase::IgnoreCase))
            {
                PartialMatchNode = FunctionNode;
            }
        }
    }
    return PartialMatchNode;
}

// ============================================================================
// Helper to properly remove override nodes
// Simply removing the connected node (Input, ParameterMapGet, or FunctionCall)
// is sufficient - the SetLinkedParameterValueForFunctionInput will create new nodes
// ============================================================================

static void RemoveOverrideNodesForPinLinked(UEdGraphPin& OverridePin)
{
    if (OverridePin.LinkedTo.Num() == 0)
    {
        return;
    }

    // Get the graph for node removal
    UEdGraphNode* ConnectedNode = OverridePin.LinkedTo[0]->GetOwningNode();
    UEdGraph* Graph = ConnectedNode ? ConnectedNode->GetGraph() : nullptr;

    if (!Graph)
    {
        OverridePin.BreakAllPinLinks(true);
        return;
    }

    // Handle UNiagaraNodeInput (data interface inputs) or UNiagaraNodeParameterMapGet (linked parameters)
    // These are simple - just remove the node
    if (ConnectedNode->IsA<UNiagaraNodeInput>() || ConnectedNode->IsA<UNiagaraNodeParameterMapGet>())
    {
        Graph->RemoveNode(ConnectedNode);
        return;
    }

    // Handle UNiagaraNodeFunctionCall (dynamic inputs like curves)
    // For dynamic inputs, we also need to remove the node
    if (ConnectedNode->IsA<UNiagaraNodeFunctionCall>())
    {
        Graph->RemoveNode(ConnectedNode);
        return;
    }

    // Unknown node type - fall back to breaking links
    OverridePin.BreakAllPinLinks(true);
}

// ============================================================================
// Set Module Linked Input - bind input to a particle attribute
// ============================================================================

// Helper to get the type definition for a linked parameter name
static FNiagaraTypeDefinition GetLinkedParameterType(const FString& LinkedValue)
{
    // Common particle attributes and their types
    static TMap<FString, FNiagaraTypeDefinition> ParticleAttributeTypes = {
        {TEXT("Particles.NormalizedAge"), FNiagaraTypeDefinition::GetFloatDef()},
        {TEXT("Particles.Age"), FNiagaraTypeDefinition::GetFloatDef()},
        {TEXT("Particles.Lifetime"), FNiagaraTypeDefinition::GetFloatDef()},
        {TEXT("Particles.Mass"), FNiagaraTypeDefinition::GetFloatDef()},
        {TEXT("Particles.SpriteRotation"), FNiagaraTypeDefinition::GetFloatDef()},
        {TEXT("Particles.Position"), FNiagaraTypeDefinition::GetVec3Def()},
        {TEXT("Particles.Velocity"), FNiagaraTypeDefinition::GetVec3Def()},
        {TEXT("Particles.Color"), FNiagaraTypeDefinition::GetColorDef()},
        {TEXT("Particles.SpriteSize"), FNiagaraTypeDefinition::GetVec2Def()},
    };

    if (const FNiagaraTypeDefinition* FoundType = ParticleAttributeTypes.Find(LinkedValue))
    {
        return *FoundType;
    }

    // Default to float for unknown attributes
    return FNiagaraTypeDefinition::GetFloatDef();
}

bool FNiagaraService::SetModuleLinkedInput(const FNiagaraModuleLinkedInputParams& Params, FString& OutError)
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

    // Find the module node
    UNiagaraNodeFunctionCall* ModuleNode = FindModuleNodeByNameForLinked(Graph, Params.ModuleName);
    if (!ModuleNode)
    {
        OutError = FString::Printf(TEXT("Module '%s' not found in stage '%s'"), *Params.ModuleName, *Params.Stage);
        return false;
    }

    // Get module inputs using the Stack API
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

        // Try exact full name match first
        if (InputNameStr.Equals(Params.InputName, ESearchCase::IgnoreCase))
        {
            FoundInput = &Input;
            break;
        }

        // Try suffix match
        if (InputNameStr.EndsWith(Params.InputName, ESearchCase::IgnoreCase))
        {
            int32 MatchPos = InputNameStr.Len() - Params.InputName.Len();
            if (MatchPos == 0 || (MatchPos > 0 && InputNameStr[MatchPos - 1] == '.'))
            {
                FoundInput = &Input;
                break;
            }
        }

        // Try simple name match
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
        TArray<FString> AvailableInputs;
        for (const FNiagaraVariable& Input : ModuleInputs)
        {
            AvailableInputs.Add(Input.GetName().ToString());
        }
        OutError = FString::Printf(TEXT("Input '%s' not found on module '%s'. Available inputs: %s"),
            *Params.InputName, *Params.ModuleName, *FString::Join(AvailableInputs, TEXT(", ")));
        return false;
    }

    FNiagaraTypeDefinition InputType = FoundInput->GetType();

    // Mark system for modification
    System->Modify();
    Graph->Modify();

    // Create the aliased module parameter handle
    FNiagaraParameterHandle AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
        FoundInput->GetName(),
        FName(*ModuleNode->GetFunctionName())
    );

    // Get or create the override pin for this input
    UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
        *ModuleNode,
        AliasedHandle,
        InputType,
        FGuid(),
        FGuid()
    );

    // Properly remove existing override nodes (not just break links!)
    if (OverridePin.LinkedTo.Num() > 0)
    {
        RemoveOverrideNodesForPinLinked(OverridePin);
    }

    // Create the linked parameter variable with the CORRECT type for the particle attribute
    // (NOT the input type - that was a bug causing crashes)
    FNiagaraTypeDefinition LinkedParamType = GetLinkedParameterType(Params.LinkedValue);
    FNiagaraVariableBase LinkedParameter(LinkedParamType, FName(*Params.LinkedValue));

    // CRITICAL FIX: Build FULL parameter context from graph, not manual list
    // The manual list causes particle-scope attributes to not be recognized properly.
    // This replicates the logic from FNiagaraStackGraphUtilities::GetParametersForContext
    Graph->ConditionalRefreshParameterReferences();

    TSet<FNiagaraVariableBase> KnownParameters;

    // Step 1: Get all script variables from the graph's metadata (GetAllMetaData is EXPORTED)
    // This gives us all variables defined in the graph context
    const UNiagaraGraph::FScriptVariableMap& ScriptVariables = Graph->GetAllMetaData();
    for (const TPair<FNiagaraVariable, TObjectPtr<UNiagaraScriptVariable>>& VarPair : ScriptVariables)
    {
        KnownParameters.Add(VarPair.Key);
    }

    // Step 2: Get all user parameters from the system's exposed parameter store
    TArray<FNiagaraVariable> UserParams;
    System->GetExposedParameters().GetUserParameters(UserParams);
    for (FNiagaraVariable& Var : UserParams)
    {
        FNiagaraUserRedirectionParameterStore::MakeUserVariable(Var);
        KnownParameters.Add(Var);
    }

    // Step 3: CRITICAL - Add ALL common particle attributes explicitly
    // These are implicit system-level attributes NOT stored in graph metadata
    // Without these, particle-scope linking won't work correctly
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), FName(TEXT("Particles.NormalizedAge"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), FName(TEXT("Particles.Age"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), FName(TEXT("Particles.Lifetime"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), FName(TEXT("Particles.Mass"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), FName(TEXT("Particles.SpriteRotation"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), FName(TEXT("Particles.RibbonWidth"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), FName(TEXT("Particles.RibbonTwist"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), FName(TEXT("Particles.Position"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), FName(TEXT("Particles.Velocity"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), FName(TEXT("Particles.Color"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), FName(TEXT("Particles.SpriteSize"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), FName(TEXT("Particles.Scale"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), FName(TEXT("Particles.RibbonID"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), FName(TEXT("Particles.RibbonLinkOrder"))));
    KnownParameters.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), FName(TEXT("Particles.UniqueID"))));

    // Still add the linked parameter explicitly to ensure it's included
    KnownParameters.Add(LinkedParameter);

    UE_LOG(LogNiagaraService, Log, TEXT("Built KnownParameters set with %d parameters (including particle attributes) for particle-scope linking"), KnownParameters.Num());

    // Use the exported SetLinkedParameterValueForFunctionInput
    // This function handles all the internal node creation and linking
    FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(
        OverridePin,
        LinkedParameter,
        KnownParameters,
        ENiagaraDefaultMode::FailIfPreviouslyNotSet,
        FGuid()
    );

    // Mark system dirty
    MarkSystemDirty(System);

    // Notify graph of changes
    Graph->NotifyGraphChanged();

    // CRITICAL: Force system recompile for runtime to pick up graph changes
    // Use bForce=true to ensure recompile even if system thinks nothing changed
    System->RequestCompile(true);

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set linked input '%s' on module '%s' to '%s'"),
        *Params.InputName, *Params.ModuleName, *Params.LinkedValue);

    return true;
}
