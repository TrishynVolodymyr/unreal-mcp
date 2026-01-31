// NiagaraCurveInputService.cpp - Curve and Color Curve Inputs for Niagara Modules
// SetModuleCurveInput, SetModuleColorCurveInput

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
#include "NiagaraDataInterfaceCurve.h"
#include "NiagaraDataInterfaceColorCurve.h"
#include "EdGraphSchema_Niagara.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "NiagaraEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Curves/RichCurve.h"

// Stateless module support
#include "NiagaraEmitterHandle.h"
#include "Stateless/NiagaraStatelessEmitter.h"
#include "Stateless/NiagaraStatelessModule.h"
#include "Stateless/NiagaraStatelessDistribution.h"

// ============================================================================
// Helper to find module node by name (shared logic)
// ============================================================================

static UNiagaraNodeFunctionCall* FindModuleNodeByName(
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
// is sufficient - the SetDataInterfaceValueForFunctionInput will create new nodes
// ============================================================================

static void RemoveOverrideNodesForPin(UEdGraphPin& OverridePin)
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
// Helper to find Dynamic Input script that outputs the target type
// ============================================================================

static UNiagaraScript* FindDynamicInputScriptForType(
    const FNiagaraTypeDefinition& TargetType,
    const FString& PreferredNameContains = TEXT(""))
{
    // Query for all Dynamic Input scripts
    FNiagaraEditorUtilities::FGetFilteredScriptAssetsOptions FilterOptions;
    FilterOptions.ScriptUsageToInclude = ENiagaraScriptUsage::DynamicInput;
    FilterOptions.bIncludeNonLibraryScripts = false; // Only use library scripts

    TArray<FAssetData> DynamicInputAssets;
    FNiagaraEditorUtilities::GetFilteredScriptAssets(FilterOptions, DynamicInputAssets);

    const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
    UNiagaraScript* BestMatch = nullptr;
    UNiagaraScript* FirstMatch = nullptr;

    for (const FAssetData& AssetData : DynamicInputAssets)
    {
        UNiagaraScript* Script = Cast<UNiagaraScript>(AssetData.GetAsset());
        if (!Script)
        {
            continue;
        }

        // Get the script source and check output type
        UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
        if (!ScriptSource || !ScriptSource->NodeGraph)
        {
            continue;
        }

        // Find the output node
        TArray<UNiagaraNodeOutput*> OutputNodes;
        ScriptSource->NodeGraph->GetNodesOfClass<UNiagaraNodeOutput>(OutputNodes);
        if (OutputNodes.Num() != 1)
        {
            continue;
        }

        // Get output pins and check type
        FPinCollectorArray InputPins;
        OutputNodes[0]->GetInputPins(InputPins);
        if (InputPins.Num() != 1)
        {
            continue;
        }

        FNiagaraTypeDefinition OutputType = NiagaraSchema->PinToTypeDefinition(InputPins[0]);

        // Check if output type is assignable to target type
        if (FNiagaraEditorUtilities::AreTypesAssignable(OutputType, TargetType))
        {
            // Found a match
            if (!FirstMatch)
            {
                FirstMatch = Script;
            }

            // Check if this matches the preferred name pattern
            if (!PreferredNameContains.IsEmpty())
            {
                FString ScriptName = Script->GetName();
                if (ScriptName.Contains(PreferredNameContains, ESearchCase::IgnoreCase))
                {
                    BestMatch = Script;
                    break; // Found preferred match
                }
            }
        }
    }

    return BestMatch ? BestMatch : FirstMatch;
}

// ============================================================================
// Helper to find and configure Curve input on a Dynamic Input for float outputs
// ============================================================================

static bool FindAndConfigureFloatCurveOnDynamicInput(
    UNiagaraNodeFunctionCall* DynamicInputNode,
    UNiagaraSystem* System,
    const TArray<FNiagaraCurveKeyframe>& Keyframes,
    FString& OutError)
{
    if (!DynamicInputNode || !DynamicInputNode->FunctionScript)
    {
        OutError = TEXT("Invalid dynamic input node");
        return false;
    }

    UNiagaraGraph* Graph = Cast<UNiagaraGraph>(DynamicInputNode->GetGraph());
    if (!Graph)
    {
        OutError = TEXT("Could not get graph from dynamic input node");
        return false;
    }

    // Get inputs for this function call
    FCompileConstantResolver ConstantResolver(System, ENiagaraScriptUsage::ParticleUpdateScript);
    TArray<FNiagaraVariable> FunctionInputs;
    FNiagaraStackGraphUtilities::GetStackFunctionInputs(
        *DynamicInputNode,
        FunctionInputs,
        ConstantResolver,
        FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly
    );

    // Find an input that accepts Curve DI
    FNiagaraTypeDefinition CurveType = FNiagaraTypeDefinition(UNiagaraDataInterfaceCurve::StaticClass());
    FNiagaraVariable* CurveInput = nullptr;

    for (FNiagaraVariable& Input : FunctionInputs)
    {
        // Check for Curve type or look for "Curve" in name
        if (Input.GetType() == CurveType ||
            Input.GetName().ToString().Contains(TEXT("Curve"), ESearchCase::IgnoreCase))
        {
            CurveInput = &Input;
            break;
        }
    }

    if (!CurveInput)
    {
        // List available inputs for debugging
        TArray<FString> AvailableInputs;
        for (const FNiagaraVariable& Input : FunctionInputs)
        {
            AvailableInputs.Add(FString::Printf(TEXT("%s (%s)"),
                *Input.GetName().ToString(), *Input.GetType().GetName()));
        }
        OutError = FString::Printf(TEXT("Could not find Curve input on dynamic input '%s'. Available inputs: %s"),
            *DynamicInputNode->GetFunctionName(), *FString::Join(AvailableInputs, TEXT(", ")));
        return false;
    }

    // Create aliased handle for this input
    FNiagaraParameterHandle AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
        CurveInput->GetName(),
        FName(*DynamicInputNode->GetFunctionName())
    );

    // Get or create override pin for the curve input
    UEdGraphPin& CurveOverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
        *DynamicInputNode,
        AliasedHandle,
        CurveType,
        FGuid(),
        FGuid()
    );

    // Properly remove existing override nodes
    if (CurveOverridePin.LinkedTo.Num() > 0)
    {
        RemoveOverrideNodesForPin(CurveOverridePin);
    }

    // Create the curve data interface
    UNiagaraDataInterface* DataInterface = nullptr;
    FNiagaraStackGraphUtilities::SetDataInterfaceValueForFunctionInput(
        CurveOverridePin,
        UNiagaraDataInterfaceCurve::StaticClass(),
        AliasedHandle.GetParameterHandleString().ToString(),
        DataInterface,
        FGuid()
    );

    if (!DataInterface)
    {
        OutError = TEXT("Failed to create curve data interface for dynamic input");
        return false;
    }

    // Configure the curve
    UNiagaraDataInterfaceCurve* CurveDI = Cast<UNiagaraDataInterfaceCurve>(DataInterface);
    if (!CurveDI)
    {
        OutError = TEXT("Failed to cast to UNiagaraDataInterfaceCurve");
        return false;
    }

    CurveDI->Modify();
    CurveDI->Curve.Reset();

    UE_LOG(LogNiagaraService, Log, TEXT("Adding %d keyframes to curve:"), Keyframes.Num());
    for (const FNiagaraCurveKeyframe& Keyframe : Keyframes)
    {
        FKeyHandle KeyHandle = CurveDI->Curve.AddKey(Keyframe.Time, Keyframe.Value);
        CurveDI->Curve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic);
        CurveDI->Curve.SetKeyTangentMode(KeyHandle, ERichCurveTangentMode::RCTM_Auto);
        UE_LOG(LogNiagaraService, Log, TEXT("  Keyframe: time=%.3f, value=%.3f"), Keyframe.Time, Keyframe.Value);
    }

    CurveDI->UpdateTimeRanges();
    CurveDI->UpdateLUT();
    CurveDI->MarkPackageDirty();

    UE_LOG(LogNiagaraService, Log, TEXT("Curve configured: TimeRange=[%.3f, %.3f], NumKeys=%d"),
        CurveDI->Curve.GetFirstKey().Time, CurveDI->Curve.GetLastKey().Time, CurveDI->Curve.GetNumKeys());

    // ========================================================================
    // CRITICAL: Set the Curve Index input to link to Particles.NormalizedAge
    // Without this, the curve will not sample based on particle age!
    // ========================================================================

    // DEBUG: Log ALL available inputs on the FloatFromCurve dynamic input
    UE_LOG(LogNiagaraService, Log, TEXT("=== FloatFromCurve Available Inputs ==="));
    for (const FNiagaraVariable& Input : FunctionInputs)
    {
        UE_LOG(LogNiagaraService, Log, TEXT("  Input: '%s' (Type: %s)"),
            *Input.GetName().ToString(), *Input.GetType().GetName());
    }
    UE_LOG(LogNiagaraService, Log, TEXT("=== End Available Inputs ==="));

    // Find the Curve Index input
    FNiagaraVariable* CurveIndexInput = nullptr;
    for (FNiagaraVariable& Input : FunctionInputs)
    {
        FString InputNameStr = Input.GetName().ToString();
        if (InputNameStr.Contains(TEXT("Index"), ESearchCase::IgnoreCase) ||
            InputNameStr.Contains(TEXT("CurveIndex"), ESearchCase::IgnoreCase) ||
            InputNameStr.Equals(TEXT("Module.Curve Index"), ESearchCase::IgnoreCase))
        {
            CurveIndexInput = &Input;
            break;
        }
    }

    if (CurveIndexInput)
    {
        UE_LOG(LogNiagaraService, Log, TEXT("Found Curve Index input '%s' - linking to Particles.NormalizedAge"),
            *CurveIndexInput->GetName().ToString());

        // Create aliased handle for Curve Index
        FNiagaraParameterHandle CurveIndexHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
            CurveIndexInput->GetName(),
            FName(*DynamicInputNode->GetFunctionName())
        );

        UE_LOG(LogNiagaraService, Log, TEXT("Created CurveIndexHandle: '%s'"),
            *CurveIndexHandle.GetParameterHandleString().ToString());

        // Get or create override pin for Curve Index
        UEdGraphPin& CurveIndexPin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
            *DynamicInputNode,
            CurveIndexHandle,
            FNiagaraTypeDefinition::GetFloatDef(),
            FGuid(),
            FGuid()
        );

        UE_LOG(LogNiagaraService, Log, TEXT("CurveIndexPin: Name='%s', LinkedTo.Num=%d"),
            *CurveIndexPin.PinName.ToString(), CurveIndexPin.LinkedTo.Num());

        // Remove existing override if any
        if (CurveIndexPin.LinkedTo.Num() > 0)
        {
            UE_LOG(LogNiagaraService, Log, TEXT("Removing existing override connection from CurveIndex pin"));
            RemoveOverrideNodesForPin(CurveIndexPin);
        }

        // Create the FNiagaraVariable for Particles.NormalizedAge
        FNiagaraVariable NormalizedAgeVar(FNiagaraTypeDefinition::GetFloatDef(), FName(TEXT("Particles.NormalizedAge")));

        // CRITICAL FIX: Build FULL parameter context from graph, not minimal set
        // The minimal set causes particle-scope attributes to not be recognized properly,
        // resulting in the curve sampling at emitter scope instead of per-particle.
        // This replicates the logic from FNiagaraStackGraphUtilities::GetParametersForContext
        UNiagaraGraph* DynamicInputGraph = Cast<UNiagaraGraph>(DynamicInputNode->GetGraph());
        if (DynamicInputGraph)
        {
            DynamicInputGraph->ConditionalRefreshParameterReferences();
        }

        TSet<FNiagaraVariableBase> KnownParameters;

        // Step 1: Get all script variables from the graph's metadata (GetAllMetaData is EXPORTED)
        // This gives us all variables defined in the graph context
        if (DynamicInputGraph)
        {
            const UNiagaraGraph::FScriptVariableMap& ScriptVariables = DynamicInputGraph->GetAllMetaData();
            for (const TPair<FNiagaraVariable, TObjectPtr<UNiagaraScriptVariable>>& VarPair : ScriptVariables)
            {
                KnownParameters.Add(VarPair.Key);
            }
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

        UE_LOG(LogNiagaraService, Log, TEXT("Built KnownParameters set with %d parameters (including particle attributes) for particle-scope linking"), KnownParameters.Num());

        // Set up the linked input to read from Particles.NormalizedAge
        FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(
            CurveIndexPin,
            NormalizedAgeVar,
            KnownParameters
        );

        // Verify the connection was made
        UE_LOG(LogNiagaraService, Log, TEXT("After SetLinkedParameterValueForFunctionInput: CurveIndexPin.LinkedTo.Num=%d"),
            CurveIndexPin.LinkedTo.Num());

        if (CurveIndexPin.LinkedTo.Num() > 0)
        {
            UEdGraphPin* LinkedPin = CurveIndexPin.LinkedTo[0];
            if (LinkedPin)
            {
                UE_LOG(LogNiagaraService, Log, TEXT("  Linked to pin: '%s' on node: '%s'"),
                    *LinkedPin->PinName.ToString(),
                    LinkedPin->GetOwningNode() ? *LinkedPin->GetOwningNode()->GetName() : TEXT("NULL"));
            }
        }
        else
        {
            UE_LOG(LogNiagaraService, Warning, TEXT("  WARNING: CurveIndexPin has NO connections after SetLinkedParameterValueForFunctionInput!"));
        }

        UE_LOG(LogNiagaraService, Log, TEXT("Linked Curve Index to Particles.NormalizedAge"));
    }
    else
    {
        UE_LOG(LogNiagaraService, Warning, TEXT("Could not find Curve Index input on Dynamic Input - curve may not sample by NormalizedAge"));
        UE_LOG(LogNiagaraService, Warning, TEXT("Available inputs were logged above"));
    }

    return true;
}

// ============================================================================
// Helper to find ColorCurve input on a Dynamic Input script's function call
// ============================================================================

static bool FindAndConfigureColorCurveOnDynamicInput(
    UNiagaraNodeFunctionCall* DynamicInputNode,
    UNiagaraSystem* System,
    const TArray<FNiagaraColorCurveKeyframe>& Keyframes,
    FString& OutError)
{
    if (!DynamicInputNode || !DynamicInputNode->FunctionScript)
    {
        OutError = TEXT("Invalid dynamic input node");
        return false;
    }

    UNiagaraGraph* Graph = Cast<UNiagaraGraph>(DynamicInputNode->GetGraph());
    if (!Graph)
    {
        OutError = TEXT("Could not get graph from dynamic input node");
        return false;
    }

    // Get the script source to find inputs
    UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(DynamicInputNode->FunctionScript->GetLatestSource());
    if (!ScriptSource || !ScriptSource->NodeGraph)
    {
        OutError = TEXT("Could not get script source for dynamic input");
        return false;
    }

    // Get inputs for this function call
    FCompileConstantResolver ConstantResolver(System, ENiagaraScriptUsage::ParticleUpdateScript);
    TArray<FNiagaraVariable> FunctionInputs;
    FNiagaraStackGraphUtilities::GetStackFunctionInputs(
        *DynamicInputNode,
        FunctionInputs,
        ConstantResolver,
        FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly
    );

    // Find an input that accepts ColorCurve DI
    FNiagaraTypeDefinition ColorCurveType = FNiagaraTypeDefinition(UNiagaraDataInterfaceColorCurve::StaticClass());
    FNiagaraVariable* CurveInput = nullptr;

    for (FNiagaraVariable& Input : FunctionInputs)
    {
        // Check for ColorCurve type or look for "Curve" in name
        if (Input.GetType() == ColorCurveType ||
            Input.GetName().ToString().Contains(TEXT("Curve"), ESearchCase::IgnoreCase) ||
            Input.GetName().ToString().Contains(TEXT("Color"), ESearchCase::IgnoreCase))
        {
            CurveInput = &Input;
            break;
        }
    }

    if (!CurveInput)
    {
        // List available inputs for debugging
        TArray<FString> AvailableInputs;
        for (const FNiagaraVariable& Input : FunctionInputs)
        {
            AvailableInputs.Add(FString::Printf(TEXT("%s (%s)"),
                *Input.GetName().ToString(), *Input.GetType().GetName()));
        }
        OutError = FString::Printf(TEXT("Could not find ColorCurve input on dynamic input '%s'. Available inputs: %s"),
            *DynamicInputNode->GetFunctionName(), *FString::Join(AvailableInputs, TEXT(", ")));
        return false;
    }

    // Create aliased handle for this input
    FNiagaraParameterHandle AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
        CurveInput->GetName(),
        FName(*DynamicInputNode->GetFunctionName())
    );

    // Get or create override pin for the curve input
    UEdGraphPin& CurveOverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
        *DynamicInputNode,
        AliasedHandle,
        ColorCurveType,
        FGuid(),
        FGuid()
    );

    // Properly remove existing override nodes (not just break links!)
    if (CurveOverridePin.LinkedTo.Num() > 0)
    {
        RemoveOverrideNodesForPin(CurveOverridePin);
    }

    // Create the color curve data interface
    UNiagaraDataInterface* DataInterface = nullptr;
    FNiagaraStackGraphUtilities::SetDataInterfaceValueForFunctionInput(
        CurveOverridePin,
        UNiagaraDataInterfaceColorCurve::StaticClass(),
        AliasedHandle.GetParameterHandleString().ToString(),
        DataInterface,
        FGuid()
    );

    if (!DataInterface)
    {
        OutError = TEXT("Failed to create color curve data interface for dynamic input");
        return false;
    }

    // Configure the curve
    UNiagaraDataInterfaceColorCurve* ColorCurveDI = Cast<UNiagaraDataInterfaceColorCurve>(DataInterface);
    if (!ColorCurveDI)
    {
        OutError = TEXT("Failed to cast to UNiagaraDataInterfaceColorCurve");
        return false;
    }

    ColorCurveDI->Modify();
    ColorCurveDI->RedCurve.Reset();
    ColorCurveDI->GreenCurve.Reset();
    ColorCurveDI->BlueCurve.Reset();
    ColorCurveDI->AlphaCurve.Reset();

    for (const FNiagaraColorCurveKeyframe& Keyframe : Keyframes)
    {
        ColorCurveDI->RedCurve.AddKey(Keyframe.Time, Keyframe.R);
        ColorCurveDI->GreenCurve.AddKey(Keyframe.Time, Keyframe.G);
        ColorCurveDI->BlueCurve.AddKey(Keyframe.Time, Keyframe.B);
        ColorCurveDI->AlphaCurve.AddKey(Keyframe.Time, Keyframe.A);
    }

    ColorCurveDI->UpdateTimeRanges();
    ColorCurveDI->UpdateLUT();
    ColorCurveDI->MarkPackageDirty();

    return true;
}

// ============================================================================
// Helper to find input variable
// ============================================================================

static bool FindModuleInputVariable(
    UNiagaraNodeFunctionCall* ModuleNode,
    UNiagaraSystem* System,
    ENiagaraScriptUsage ScriptUsage,
    const FString& InputName,
    FNiagaraVariable& OutVariable,
    FString& OutError)
{
    FCompileConstantResolver ConstantResolver(System, ScriptUsage);

    TArray<FNiagaraVariable> ModuleInputs;
    FNiagaraStackGraphUtilities::GetStackFunctionInputs(
        *ModuleNode,
        ModuleInputs,
        ConstantResolver,
        FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly
    );

    // Find the input by name
    for (FNiagaraVariable& Input : ModuleInputs)
    {
        FString InputNameStr = Input.GetName().ToString();

        // Try exact full name match first
        if (InputNameStr.Equals(InputName, ESearchCase::IgnoreCase))
        {
            OutVariable = Input;
            return true;
        }

        // Try suffix match
        if (InputNameStr.EndsWith(InputName, ESearchCase::IgnoreCase))
        {
            int32 MatchPos = InputNameStr.Len() - InputName.Len();
            if (MatchPos == 0 || (MatchPos > 0 && InputNameStr[MatchPos - 1] == '.'))
            {
                OutVariable = Input;
                return true;
            }
        }

        // Try simple name match (last component)
        FString SimpleName = InputNameStr;
        int32 DotIndex = INDEX_NONE;
        if (InputNameStr.FindLastChar('.', DotIndex))
        {
            SimpleName = InputNameStr.RightChop(DotIndex + 1);
        }

        if (SimpleName.Equals(InputName, ESearchCase::IgnoreCase))
        {
            OutVariable = Input;
            return true;
        }
    }

    // Build error with available inputs
    TArray<FString> AvailableInputs;
    for (const FNiagaraVariable& Input : ModuleInputs)
    {
        AvailableInputs.Add(Input.GetName().ToString());
    }
    OutError = FString::Printf(TEXT("Input '%s' not found. Available: %s"),
        *InputName, *FString::Join(AvailableInputs, TEXT(", ")));
    return false;
}

// ============================================================================
// Helper to set curve on Stateless Module distribution property
// Stateless modules (like ScaleRibbonWidth, ScaleColor, ScaleSpriteSize) use
// FNiagaraDistribution* properties directly, not graph-based curve DIs.
// ============================================================================

static bool SetStatelessModuleCurveInput(
    UNiagaraStatelessEmitter* StatelessEmitter,
    const FString& ModuleName,
    const FString& InputName,
    const TArray<FNiagaraCurveKeyframe>& Keyframes,
    FString& OutError)
{
    if (!StatelessEmitter)
    {
        OutError = TEXT("Invalid stateless emitter");
        return false;
    }

    // Normalize search name (remove spaces for comparison)
    FString NormalizedSearchName = ModuleName.Replace(TEXT(" "), TEXT(""));

    // Find the module by name in the stateless emitter's module array
    UNiagaraStatelessModule* TargetModule = nullptr;
    const TArray<TObjectPtr<UNiagaraStatelessModule>>& Modules = StatelessEmitter->GetModules();

    for (UNiagaraStatelessModule* Module : Modules)
    {
        if (!Module)
        {
            continue;
        }

        // Get module display name from class
        FString ModuleClassName = Module->GetClass()->GetName();
        // Class names are like "NiagaraStatelessModule_ScaleRibbonWidth" - extract the suffix
        FString ModuleDisplayName = ModuleClassName;
        if (ModuleClassName.StartsWith(TEXT("NiagaraStatelessModule_")))
        {
            ModuleDisplayName = ModuleClassName.RightChop(23); // Length of "NiagaraStatelessModule_"
        }

        // Also check metadata DisplayName
        FString DisplayName = Module->GetClass()->GetMetaData(TEXT("DisplayName"));

        // Normalize for comparison
        FString NormalizedClassName = ModuleDisplayName.Replace(TEXT(" "), TEXT(""));
        FString NormalizedDisplayName = DisplayName.Replace(TEXT(" "), TEXT(""));

        if (NormalizedClassName.Equals(NormalizedSearchName, ESearchCase::IgnoreCase) ||
            NormalizedDisplayName.Equals(NormalizedSearchName, ESearchCase::IgnoreCase) ||
            NormalizedClassName.Contains(NormalizedSearchName, ESearchCase::IgnoreCase) ||
            NormalizedDisplayName.Contains(NormalizedSearchName, ESearchCase::IgnoreCase))
        {
            TargetModule = Module;
            break;
        }
    }

    if (!TargetModule)
    {
        // List available modules for debugging
        TArray<FString> AvailableModules;
        for (UNiagaraStatelessModule* Module : Modules)
        {
            if (Module)
            {
                FString DisplayName = Module->GetClass()->GetMetaData(TEXT("DisplayName"));
                if (DisplayName.IsEmpty())
                {
                    DisplayName = Module->GetClass()->GetName();
                }
                AvailableModules.Add(DisplayName);
            }
        }
        OutError = FString::Printf(TEXT("Stateless module '%s' not found. Available modules: %s"),
            *ModuleName, *FString::Join(AvailableModules, TEXT(", ")));
        return false;
    }

    // Find the distribution property by name
    // Common patterns: "Scale" -> "ScaleDistribution", or the InputName might be the property name directly
    TArray<FString> PropertyNamesToTry;
    PropertyNamesToTry.Add(InputName);
    PropertyNamesToTry.Add(InputName + TEXT("Distribution"));
    PropertyNamesToTry.Add(TEXT("Scale")); // Common default for scale modules
    PropertyNamesToTry.Add(TEXT("ScaleDistribution"));

    FNiagaraDistributionBase* DistributionPtr = nullptr;
    FProperty* FoundProperty = nullptr;

    for (const FString& PropName : PropertyNamesToTry)
    {
        FProperty* Property = TargetModule->GetClass()->FindPropertyByName(FName(*PropName));
        if (Property)
        {
            // Check if it's a struct property that derives from FNiagaraDistributionBase
            FStructProperty* StructProperty = CastField<FStructProperty>(Property);
            if (StructProperty && StructProperty->Struct)
            {
                // Check if this struct inherits from FNiagaraDistributionBase
                UScriptStruct* DistributionBaseStruct = FNiagaraDistributionBase::StaticStruct();
                if (StructProperty->Struct == DistributionBaseStruct ||
                    StructProperty->Struct->IsChildOf(DistributionBaseStruct))
                {
                    DistributionPtr = StructProperty->ContainerPtrToValuePtr<FNiagaraDistributionBase>(TargetModule);
                    FoundProperty = Property;
                    break;
                }

                // Also check FNiagaraDistributionFloat specifically
                UScriptStruct* FloatDistStruct = FNiagaraDistributionFloat::StaticStruct();
                if (StructProperty->Struct == FloatDistStruct)
                {
                    DistributionPtr = StructProperty->ContainerPtrToValuePtr<FNiagaraDistributionBase>(TargetModule);
                    FoundProperty = Property;
                    break;
                }
            }
        }
    }

    if (!DistributionPtr || !FoundProperty)
    {
        // List available properties for debugging
        TArray<FString> AvailableProperties;
        for (TFieldIterator<FProperty> PropIt(TargetModule->GetClass()); PropIt; ++PropIt)
        {
            FStructProperty* StructProp = CastField<FStructProperty>(*PropIt);
            if (StructProp && StructProp->Struct)
            {
                AvailableProperties.Add(FString::Printf(TEXT("%s (%s)"),
                    *PropIt->GetName(), *StructProp->Struct->GetName()));
            }
        }
        OutError = FString::Printf(TEXT("Distribution property '%s' not found on module '%s'. Available struct properties: %s"),
            *InputName, *ModuleName, *FString::Join(AvailableProperties, TEXT(", ")));
        return false;
    }

    // Mark the module for modification
    TargetModule->Modify();

    // Set up the distribution for curve mode
    DistributionPtr->Mode = ENiagaraDistributionMode::UniformCurve;
    DistributionPtr->LookupValueMode = static_cast<uint8>(ENiagaraDistributionLookupValueMode::ParticlesNormalizedAge);

#if WITH_EDITORONLY_DATA
    // Clear and populate the curve data
    DistributionPtr->ChannelCurves.SetNum(1);
    DistributionPtr->ChannelCurves[0].Reset();

    for (const FNiagaraCurveKeyframe& Keyframe : Keyframes)
    {
        FKeyHandle KeyHandle = DistributionPtr->ChannelCurves[0].AddKey(Keyframe.Time, Keyframe.Value);
        // Set interpolation mode to smooth curves
        DistributionPtr->ChannelCurves[0].SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic);
        DistributionPtr->ChannelCurves[0].SetKeyTangentMode(KeyHandle, ERichCurveTangentMode::RCTM_Auto);
    }

    // Update the LUT values from the curve data
    DistributionPtr->UpdateValuesFromDistribution();
#endif

    // Mark package dirty
    TargetModule->MarkPackageDirty();

    UE_LOG(LogNiagaraService, Log, TEXT("Set stateless module '%s' property '%s' to curve with %d keyframes"),
        *ModuleName, *FoundProperty->GetName(), Keyframes.Num());

    return true;
}

// ============================================================================
// Set Module Curve Input
// ============================================================================

bool FNiagaraService::SetModuleCurveInput(const FNiagaraModuleCurveInputParams& Params, FString& OutError)
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

    // Check if this is a Stateless Emitter - they require special handling
    // Stateless modules use FNiagaraDistribution* properties directly, not graph-based curve DIs
    UNiagaraStatelessEmitter* StatelessEmitter = EmitterHandle.GetStatelessEmitter();
    if (StatelessEmitter && EmitterHandle.GetEmitterMode() == ENiagaraEmitterMode::Stateless)
    {
        UE_LOG(LogNiagaraService, Log, TEXT("Detected Stateless Emitter '%s' - using direct distribution property access"),
            *Params.EmitterName);

        // Mark system for modification
        System->Modify();

        bool bSuccess = SetStatelessModuleCurveInput(
            StatelessEmitter,
            Params.ModuleName,
            Params.InputName,
            Params.Keyframes,
            OutError);

        if (bSuccess)
        {
            MarkSystemDirty(System);
            RefreshEditors(System);
        }
        return bSuccess;
    }

    // Standard graph-based emitter path
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

    // Find the module node
    UNiagaraNodeFunctionCall* ModuleNode = FindModuleNodeByName(Graph, Params.ModuleName);
    if (!ModuleNode)
    {
        OutError = FString::Printf(TEXT("Module '%s' not found in stage '%s'"), *Params.ModuleName, *Params.Stage);
        return false;
    }

    // Find the input variable
    FNiagaraVariable InputVariable;
    if (!FindModuleInputVariable(ModuleNode, System, ScriptUsage, Params.InputName, InputVariable, OutError))
    {
        OutError = FString::Printf(TEXT("Input '%s' not found on module '%s'. %s"), *Params.InputName, *Params.ModuleName, *OutError);
        return false;
    }

    // Get the input type
    FNiagaraTypeDefinition InputType = InputVariable.GetType();
    FNiagaraTypeDefinition CurveType = FNiagaraTypeDefinition(UNiagaraDataInterfaceCurve::StaticClass());

    // Mark system for modification
    System->Modify();
    Graph->Modify();

    // Create the aliased module parameter handle
    FNiagaraParameterHandle AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
        InputVariable.GetName(),
        FName(*ModuleNode->GetFunctionName())
    );

    // ========================================================================
    // APPROACH 1: Float inputs - Use Dynamic Input wrapper to sample curve
    // This is required because float inputs don't automatically sample curves
    // ========================================================================
    if (InputType == FNiagaraTypeDefinition::GetFloatDef())
    {
        UE_LOG(LogNiagaraService, Log, TEXT("Input '%s' is Float type - using Dynamic Input to sample curve by NormalizedAge"),
            *Params.InputName);

        // Find a Dynamic Input script that outputs Float and samples a curve by NormalizedAge
        // IMPORTANT: "FloatFromCurve" is the correct script - it samples the curve at Particles.NormalizedAge
        // "DistributionCurveFloat" is a helper script that does NOT sample by NormalizedAge automatically
        UNiagaraScript* DynamicInputScript = FindDynamicInputScriptForType(
            FNiagaraTypeDefinition::GetFloatDef(),
            TEXT("FloatFromCurve")  // Exact match for the correct Dynamic Input
        );

        if (!DynamicInputScript)
        {
            // Try alternate naming
            DynamicInputScript = FindDynamicInputScriptForType(
                FNiagaraTypeDefinition::GetFloatDef(),
                TEXT("Float From Curve")
            );
        }

        if (!DynamicInputScript)
        {
            // Fall back to ScaleFloatByCurve if FloatFromCurve not found
            DynamicInputScript = FindDynamicInputScriptForType(
                FNiagaraTypeDefinition::GetFloatDef(),
                TEXT("ScaleFloatByCurve")
            );
        }

        if (!DynamicInputScript)
        {
            OutError = FString::Printf(
                TEXT("Could not find a Dynamic Input script that outputs Float and accepts a curve. ")
                TEXT("This is required because input '%s' is a Float type, which cannot directly accept Curve data interfaces. ")
                TEXT("Standard graph-based modules need a Dynamic Input like 'Float from Curve' to sample curves by NormalizedAge. ")
                TEXT("Please ensure Niagara content is loaded."),
                *Params.InputName);
            return false;
        }

        UE_LOG(LogNiagaraService, Log, TEXT("Found Dynamic Input script: %s"), *DynamicInputScript->GetPathName());

        // Get or create override pin for the Float input
        UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
            *ModuleNode,
            AliasedHandle,
            InputType,  // Use the actual input type (Float)
            FGuid(),
            FGuid()
        );

        // Properly remove existing override nodes
        if (OverridePin.LinkedTo.Num() > 0)
        {
            RemoveOverrideNodesForPin(OverridePin);
        }

        // Set the Dynamic Input script on the override pin
        UNiagaraNodeFunctionCall* DynamicInputNode = nullptr;
        FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput(
            OverridePin,
            DynamicInputScript,
            DynamicInputNode,
            FGuid(),
            TEXT(""),  // Let it auto-generate name
            FGuid()    // Use default version
        );

        if (!DynamicInputNode)
        {
            OutError = TEXT("Failed to create Dynamic Input function call node");
            return false;
        }

        // Now configure the curve on the Dynamic Input's nested inputs
        if (!FindAndConfigureFloatCurveOnDynamicInput(DynamicInputNode, System, Params.Keyframes, OutError))
        {
            // OutError already set
            return false;
        }

        UE_LOG(LogNiagaraService, Log, TEXT("Successfully configured Dynamic Input '%s' with curve (samples by NormalizedAge)"),
            *DynamicInputNode->GetFunctionName());
    }
    // ========================================================================
    // APPROACH 2: Curve DI inputs - Direct assignment
    // ========================================================================
    else if (InputType == CurveType)
    {
        UE_LOG(LogNiagaraService, Log, TEXT("Input '%s' is Curve DI type - direct assignment"),
            *Params.InputName);

        // Get or create the override pin for this input
        UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
            *ModuleNode,
            AliasedHandle,
            CurveType,
            FGuid(),
            FGuid()
        );

        // Check if the pin already has a connection
        if (OverridePin.LinkedTo.Num() > 0)
        {
            RemoveOverrideNodesForPin(OverridePin);
        }

        // Create the curve data interface
        UNiagaraDataInterface* DataInterface = nullptr;
        FNiagaraStackGraphUtilities::SetDataInterfaceValueForFunctionInput(
            OverridePin,
            UNiagaraDataInterfaceCurve::StaticClass(),
            AliasedHandle.GetParameterHandleString().ToString(),
            DataInterface,
            FGuid()
        );

        if (!DataInterface)
        {
            OutError = TEXT("Failed to create curve data interface");
            return false;
        }

        UNiagaraDataInterfaceCurve* CurveDI = Cast<UNiagaraDataInterfaceCurve>(DataInterface);
        if (!CurveDI)
        {
            OutError = TEXT("Failed to cast to UNiagaraDataInterfaceCurve");
            return false;
        }

        CurveDI->Modify();
        CurveDI->Curve.Reset();

        for (const FNiagaraCurveKeyframe& Keyframe : Params.Keyframes)
        {
            CurveDI->Curve.AddKey(Keyframe.Time, Keyframe.Value);
        }

        CurveDI->UpdateTimeRanges();
        CurveDI->UpdateLUT();
        CurveDI->MarkPackageDirty();
    }
    // ========================================================================
    // APPROACH 3: Other types - Try Dynamic Input with warning
    // ========================================================================
    else
    {
        UE_LOG(LogNiagaraService, Warning,
            TEXT("Input '%s' type is '%s' - attempting Dynamic Input wrapper, but results may vary"),
            *Params.InputName, *InputType.GetName());

        // Try to find a matching dynamic input for this type
        UNiagaraScript* DynamicInputScript = FindDynamicInputScriptForType(InputType, TEXT("Curve"));

        if (DynamicInputScript)
        {
            UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
                *ModuleNode,
                AliasedHandle,
                InputType,
                FGuid(),
                FGuid()
            );

            if (OverridePin.LinkedTo.Num() > 0)
            {
                RemoveOverrideNodesForPin(OverridePin);
            }

            UNiagaraNodeFunctionCall* DynamicInputNode = nullptr;
            FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput(
                OverridePin,
                DynamicInputScript,
                DynamicInputNode,
                FGuid(),
                TEXT(""),
                FGuid()
            );

            if (DynamicInputNode)
            {
                if (!FindAndConfigureFloatCurveOnDynamicInput(DynamicInputNode, System, Params.Keyframes, OutError))
                {
                    UE_LOG(LogNiagaraService, Warning, TEXT("Could not configure curve on Dynamic Input: %s"), *OutError);
                }
            }
        }
        else
        {
            OutError = FString::Printf(
                TEXT("Input type '%s' is not directly compatible with curve inputs. ")
                TEXT("Curve inputs work best with Float or Curve DI types."),
                *InputType.GetName());
            return false;
        }
    }

    // Mark system dirty
    MarkSystemDirty(System);

    // Notify graph of changes
    Graph->NotifyGraphChanged();

    // CRITICAL: Force system recompile for runtime to pick up graph changes
    // Use bForce=true to ensure recompile even if system thinks nothing changed
    System->RequestCompile(true);
    UE_LOG(LogNiagaraService, Log, TEXT("Forced system recompile after curve input change"));

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set curve input '%s' on module '%s' with %d keyframes"),
        *Params.InputName, *Params.ModuleName, Params.Keyframes.Num());

    return true;
}

// ============================================================================
// Set Module Color Curve Input
// ============================================================================

bool FNiagaraService::SetModuleColorCurveInput(const FNiagaraModuleColorCurveInputParams& Params, FString& OutError)
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

    // Find the module node
    UNiagaraNodeFunctionCall* ModuleNode = FindModuleNodeByName(Graph, Params.ModuleName);
    if (!ModuleNode)
    {
        OutError = FString::Printf(TEXT("Module '%s' not found in stage '%s'"), *Params.ModuleName, *Params.Stage);
        return false;
    }

    // Find the input variable
    FNiagaraVariable InputVariable;
    if (!FindModuleInputVariable(ModuleNode, System, ScriptUsage, Params.InputName, InputVariable, OutError))
    {
        OutError = FString::Printf(TEXT("Input '%s' not found on module '%s'. %s"), *Params.InputName, *Params.ModuleName, *OutError);
        return false;
    }

    // Check input type to determine approach
    FNiagaraTypeDefinition InputType = InputVariable.GetType();
    FNiagaraTypeDefinition ColorCurveType = FNiagaraTypeDefinition(UNiagaraDataInterfaceColorCurve::StaticClass());

    // Mark system for modification
    System->Modify();
    Graph->Modify();

    // Create the aliased module parameter handle
    FNiagaraParameterHandle AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
        InputVariable.GetName(),
        FName(*ModuleNode->GetFunctionName())
    );

    // ========================================================================
    // APPROACH 1: LinearColor inputs - Use Dynamic Input wrapper
    // ========================================================================
    if (InputType == FNiagaraTypeDefinition::GetColorDef())
    {
        UE_LOG(LogNiagaraService, Log, TEXT("Input '%s' is LinearColor type - using Dynamic Input approach"),
            *Params.InputName);

        // Find a Dynamic Input script that outputs LinearColor and accepts a curve
        // Prefer scripts with "Scale" or "Curve" in their name
        UNiagaraScript* DynamicInputScript = FindDynamicInputScriptForType(
            FNiagaraTypeDefinition::GetColorDef(),
            TEXT("Scale")  // Prefer "Scale Linear Color by Curve" style scripts
        );

        if (!DynamicInputScript)
        {
            // Try without preference
            DynamicInputScript = FindDynamicInputScriptForType(
                FNiagaraTypeDefinition::GetColorDef(),
                TEXT("Curve")
            );
        }

        if (!DynamicInputScript)
        {
            OutError = FString::Printf(
                TEXT("Could not find a Dynamic Input script that outputs LinearColor and accepts a color curve. ")
                TEXT("This is required because input '%s' is a LinearColor type, which cannot directly accept ColorCurve data interfaces. ")
                TEXT("Please ensure Niagara content is loaded (you may need to restart the editor)."),
                *Params.InputName);
            return false;
        }

        UE_LOG(LogNiagaraService, Log, TEXT("Found Dynamic Input script: %s"), *DynamicInputScript->GetPathName());

        // Get or create override pin for the LinearColor input
        UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
            *ModuleNode,
            AliasedHandle,
            InputType,  // Use the actual input type (LinearColor)
            FGuid(),
            FGuid()
        );

        // Properly remove existing override nodes (not just break links!)
        if (OverridePin.LinkedTo.Num() > 0)
        {
            RemoveOverrideNodesForPin(OverridePin);
        }

        // Set the Dynamic Input script on the override pin
        UNiagaraNodeFunctionCall* DynamicInputNode = nullptr;
        FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput(
            OverridePin,
            DynamicInputScript,
            DynamicInputNode,
            FGuid(),
            TEXT(""),  // Let it auto-generate name
            FGuid()    // Use default version
        );

        if (!DynamicInputNode)
        {
            OutError = TEXT("Failed to create Dynamic Input function call node");
            return false;
        }

        // Now configure the curve on the Dynamic Input's nested inputs
        if (!FindAndConfigureColorCurveOnDynamicInput(DynamicInputNode, System, Params.Keyframes, OutError))
        {
            // OutError already set
            return false;
        }

        UE_LOG(LogNiagaraService, Log, TEXT("Successfully configured Dynamic Input '%s' with color curve"),
            *DynamicInputNode->GetFunctionName());
    }
    // ========================================================================
    // APPROACH 2: ColorCurve or compatible inputs - Direct assignment
    // ========================================================================
    else
    {
        // Check if input type is compatible with ColorCurve DI
        if (InputType != ColorCurveType)
        {
            UE_LOG(LogNiagaraService, Warning,
                TEXT("Input '%s' type is '%s', which may not be compatible with ColorCurve. ")
                TEXT("Attempting direct assignment anyway."),
                *Params.InputName, *InputType.GetName());
        }

        // Get or create the override pin for this input
        UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
            *ModuleNode,
            AliasedHandle,
            ColorCurveType,
            FGuid(),
            FGuid()
        );

        // Properly remove existing override nodes (not just break links!)
        if (OverridePin.LinkedTo.Num() > 0)
        {
            RemoveOverrideNodesForPin(OverridePin);
        }

        // Create the color curve data interface
        UNiagaraDataInterface* DataInterface = nullptr;
        FNiagaraStackGraphUtilities::SetDataInterfaceValueForFunctionInput(
            OverridePin,
            UNiagaraDataInterfaceColorCurve::StaticClass(),
            AliasedHandle.GetParameterHandleString().ToString(),
            DataInterface,
            FGuid()
        );

        if (!DataInterface)
        {
            OutError = TEXT("Failed to create color curve data interface");
            return false;
        }

        // Cast to color curve DI and populate with keyframes
        UNiagaraDataInterfaceColorCurve* ColorCurveDI = Cast<UNiagaraDataInterfaceColorCurve>(DataInterface);
        if (!ColorCurveDI)
        {
            OutError = TEXT("Failed to cast to UNiagaraDataInterfaceColorCurve");
            return false;
        }

        // Mark the data interface for modification
        ColorCurveDI->Modify();

        // Clear existing keys and add new ones to each channel
        ColorCurveDI->RedCurve.Reset();
        ColorCurveDI->GreenCurve.Reset();
        ColorCurveDI->BlueCurve.Reset();
        ColorCurveDI->AlphaCurve.Reset();

        for (const FNiagaraColorCurveKeyframe& Keyframe : Params.Keyframes)
        {
            ColorCurveDI->RedCurve.AddKey(Keyframe.Time, Keyframe.R);
            ColorCurveDI->GreenCurve.AddKey(Keyframe.Time, Keyframe.G);
            ColorCurveDI->BlueCurve.AddKey(Keyframe.Time, Keyframe.B);
            ColorCurveDI->AlphaCurve.AddKey(Keyframe.Time, Keyframe.A);
        }

        // Update the curve's time ranges and LUT
        ColorCurveDI->UpdateTimeRanges();
        ColorCurveDI->UpdateLUT();
        ColorCurveDI->MarkPackageDirty();
    }

    // Mark system dirty
    MarkSystemDirty(System);

    // Notify graph of changes
    Graph->NotifyGraphChanged();

    // CRITICAL: Force system recompile for runtime to pick up graph changes
    System->RequestCompile(false);
    UE_LOG(LogNiagaraService, Log, TEXT("Requested system recompile after color curve input change"));

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set color curve input '%s' on module '%s' with %d keyframes"),
        *Params.InputName, *Params.ModuleName, Params.Keyframes.Num());

    return true;
}
