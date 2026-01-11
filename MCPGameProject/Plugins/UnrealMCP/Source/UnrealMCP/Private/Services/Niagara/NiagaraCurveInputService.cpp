// NiagaraCurveInputService.cpp - Curve and Color Curve Inputs for Niagara Modules
// SetModuleCurveInput, SetModuleColorCurveInput

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeFunctionCall.h"
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

    // Clear existing connections
    if (CurveOverridePin.LinkedTo.Num() > 0)
    {
        CurveOverridePin.BreakAllPinLinks(true);
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

    // Check that the input type accepts a curve (should be float or compatible)
    FNiagaraTypeDefinition InputType = InputVariable.GetType();
    if (InputType != FNiagaraTypeDefinition::GetFloatDef())
    {
        UE_LOG(LogNiagaraService, Warning, TEXT("Input '%s' type is '%s', curve input typically used for float types"),
            *Params.InputName, *InputType.GetName());
    }

    // Mark system for modification
    System->Modify();
    Graph->Modify();

    // Create the aliased module parameter handle
    FNiagaraParameterHandle AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
        InputVariable.GetName(),
        FName(*ModuleNode->GetFunctionName())
    );

    // Get or create the override pin for this input
    // We need to use the curve data interface type
    FNiagaraTypeDefinition CurveType = FNiagaraTypeDefinition(UNiagaraDataInterfaceCurve::StaticClass());

    UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
        *ModuleNode,
        AliasedHandle,
        CurveType,
        FGuid(),  // No script variable ID
        FGuid()   // No preferred node GUID
    );

    // Check if the pin already has a connection (an existing curve DI)
    if (OverridePin.LinkedTo.Num() > 0)
    {
        // Break existing connections using UEdGraphPin's built-in method
        OverridePin.BreakAllPinLinks(true);
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

    // Cast to curve DI and populate with keyframes
    UNiagaraDataInterfaceCurve* CurveDI = Cast<UNiagaraDataInterfaceCurve>(DataInterface);
    if (!CurveDI)
    {
        OutError = TEXT("Failed to cast to UNiagaraDataInterfaceCurve");
        return false;
    }

    // Mark the data interface for modification (required for proper undo/redo support)
    CurveDI->Modify();

    // Clear existing keys and add new ones
    CurveDI->Curve.Reset();

    for (const FNiagaraCurveKeyframe& Keyframe : Params.Keyframes)
    {
        CurveDI->Curve.AddKey(Keyframe.Time, Keyframe.Value);
    }

    // Update the curve's time ranges and LUT
    CurveDI->UpdateTimeRanges();
    CurveDI->UpdateLUT();

    // Mark package dirty to ensure changes are saved
    CurveDI->MarkPackageDirty();

    // Mark system dirty
    MarkSystemDirty(System);

    // Notify graph of changes
    Graph->NotifyGraphChanged();

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

        // Remove any existing override by breaking pin links
        // Note: This clears the connection but may leave orphan nodes - acceptable for MCP use
        if (OverridePin.LinkedTo.Num() > 0)
        {
            OverridePin.BreakAllPinLinks(true);
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

        // Check if the pin already has a connection (an existing color curve DI)
        if (OverridePin.LinkedTo.Num() > 0)
        {
            OverridePin.BreakAllPinLinks(true);
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

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set color curve input '%s' on module '%s' with %d keyframes"),
        *Params.InputName, *Params.ModuleName, Params.Keyframes.Num());

    return true;
}
