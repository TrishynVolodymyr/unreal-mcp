// NiagaraRandomInputService.cpp - Random Range Inputs for Niagara Modules
// SetModuleRandomInput - Uniform random between min and max values

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
#include "EdGraphSchema_Niagara.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"

// ============================================================================
// Helper to find module node by name
// ============================================================================

static UNiagaraNodeFunctionCall* FindModuleNodeByNameForRandom(
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
// Helper to find input variable
// ============================================================================

static bool FindModuleInputVariableForRandom(
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
// Helper to get the appropriate UniformRanged dynamic input script path
// ============================================================================

static FString GetUniformRangedScriptPath(const FNiagaraTypeDefinition& InputType)
{
    // Map input types to their corresponding UniformRanged dynamic input scripts
    if (InputType == FNiagaraTypeDefinition::GetFloatDef())
    {
        return TEXT("/Niagara/DynamicInputs/UniformRange/UniformRangedFloat.UniformRangedFloat");
    }
    else if (InputType == FNiagaraTypeDefinition::GetIntDef())
    {
        return TEXT("/Niagara/DynamicInputs/UniformRange/UniformRangedInt.UniformRangedInt");
    }
    else if (InputType == FNiagaraTypeDefinition::GetVec2Def())
    {
        return TEXT("/Niagara/DynamicInputs/UniformRange/UniformRangedVector2D.UniformRangedVector2D");
    }
    else if (InputType == FNiagaraTypeDefinition::GetVec3Def())
    {
        return TEXT("/Niagara/DynamicInputs/UniformRange/UniformRangedVector.UniformRangedVector");
    }
    else if (InputType == FNiagaraTypeDefinition::GetVec4Def())
    {
        return TEXT("/Niagara/DynamicInputs/UniformRange/UniformRangedVector4.UniformRangedVector4");
    }
    else if (InputType == FNiagaraTypeDefinition::GetColorDef())
    {
        return TEXT("/Niagara/DynamicInputs/UniformRange/UniformRangedLinearColor.UniformRangedLinearColor");
    }

    return TEXT("");
}

// ============================================================================
// Helper to parse value string into FNiagaraVariable
// ============================================================================

static bool ParseValueToVariable(
    const FString& ValueStr,
    const FNiagaraTypeDefinition& InputType,
    FNiagaraVariable& OutVariable,
    FString& OutError)
{
    // Clean up the value string
    FString CleanValueStr = ValueStr;
    CleanValueStr.TrimStartAndEndInline();
    CleanValueStr = CleanValueStr.Replace(TEXT("("), TEXT(""));
    CleanValueStr = CleanValueStr.Replace(TEXT(")"), TEXT(""));
    CleanValueStr = CleanValueStr.Replace(TEXT(" "), TEXT(""));
    // Remove common component prefixes
    CleanValueStr = CleanValueStr.Replace(TEXT("R="), TEXT(""));
    CleanValueStr = CleanValueStr.Replace(TEXT("G="), TEXT(""));
    CleanValueStr = CleanValueStr.Replace(TEXT("B="), TEXT(""));
    CleanValueStr = CleanValueStr.Replace(TEXT("A="), TEXT(""));
    CleanValueStr = CleanValueStr.Replace(TEXT("X="), TEXT(""));
    CleanValueStr = CleanValueStr.Replace(TEXT("Y="), TEXT(""));
    CleanValueStr = CleanValueStr.Replace(TEXT("Z="), TEXT(""));
    CleanValueStr = CleanValueStr.Replace(TEXT("W="), TEXT(""));

    OutVariable = FNiagaraVariable(InputType, NAME_None);

    if (InputType == FNiagaraTypeDefinition::GetFloatDef())
    {
        float FloatValue = FCString::Atof(*CleanValueStr);
        OutVariable.AllocateData();
        OutVariable.SetValue<float>(FloatValue);
        return true;
    }
    else if (InputType == FNiagaraTypeDefinition::GetIntDef())
    {
        int32 IntValue = FCString::Atoi(*CleanValueStr);
        OutVariable.AllocateData();
        OutVariable.SetValue<int32>(IntValue);
        return true;
    }
    else if (InputType == FNiagaraTypeDefinition::GetVec2Def())
    {
        TArray<FString> Components;
        CleanValueStr.ParseIntoArray(Components, TEXT(","), true);
        if (Components.Num() >= 2)
        {
            FVector2f Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]));
            OutVariable.AllocateData();
            OutVariable.SetValue<FVector2f>(Vec);
            return true;
        }
        OutError = TEXT("Vector2D requires at least 2 comma-separated values");
        return false;
    }
    else if (InputType == FNiagaraTypeDefinition::GetVec3Def())
    {
        TArray<FString> Components;
        CleanValueStr.ParseIntoArray(Components, TEXT(","), true);
        if (Components.Num() >= 3)
        {
            FVector3f Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]));
            OutVariable.AllocateData();
            OutVariable.SetValue<FVector3f>(Vec);
            return true;
        }
        OutError = TEXT("Vector3 requires at least 3 comma-separated values");
        return false;
    }
    else if (InputType == FNiagaraTypeDefinition::GetVec4Def())
    {
        TArray<FString> Components;
        CleanValueStr.ParseIntoArray(Components, TEXT(","), true);
        if (Components.Num() >= 4)
        {
            FVector4f Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]),
                          FCString::Atof(*Components[2]), FCString::Atof(*Components[3]));
            OutVariable.AllocateData();
            OutVariable.SetValue<FVector4f>(Vec);
            return true;
        }
        OutError = TEXT("Vector4 requires at least 4 comma-separated values");
        return false;
    }
    else if (InputType == FNiagaraTypeDefinition::GetColorDef())
    {
        TArray<FString> Components;
        CleanValueStr.ParseIntoArray(Components, TEXT(","), true);
        if (Components.Num() >= 4)
        {
            FLinearColor Color(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]),
                               FCString::Atof(*Components[2]), FCString::Atof(*Components[3]));
            OutVariable.AllocateData();
            OutVariable.SetValue<FLinearColor>(Color);
            return true;
        }
        else if (Components.Num() >= 3)
        {
            FLinearColor Color(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]),
                               FCString::Atof(*Components[2]), 1.0f);
            OutVariable.AllocateData();
            OutVariable.SetValue<FLinearColor>(Color);
            return true;
        }
        OutError = TEXT("LinearColor requires at least 3 comma-separated values (RGBA or RGB)");
        return false;
    }

    OutError = FString::Printf(TEXT("Unsupported type '%s' for random input"), *InputType.GetName());
    return false;
}

// ============================================================================
// Helper to set min/max values on the dynamic input's nested inputs
// ============================================================================

static bool SetMinMaxOnDynamicInput(
    UNiagaraNodeFunctionCall* DynamicInputNode,
    UNiagaraSystem* System,
    const FNiagaraTypeDefinition& ValueType,
    const FString& MinValueStr,
    const FString& MaxValueStr,
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
    FCompileConstantResolver ConstantResolver(System, ENiagaraScriptUsage::ParticleSpawnScript);
    TArray<FNiagaraVariable> FunctionInputs;
    FNiagaraStackGraphUtilities::GetStackFunctionInputs(
        *DynamicInputNode,
        FunctionInputs,
        ConstantResolver,
        FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly
    );

    // Find Minimum and Maximum inputs
    FNiagaraVariable* MinInput = nullptr;
    FNiagaraVariable* MaxInput = nullptr;

    for (FNiagaraVariable& Input : FunctionInputs)
    {
        FString InputNameStr = Input.GetName().ToString();
        if (InputNameStr.Contains(TEXT("Minimum"), ESearchCase::IgnoreCase) ||
            InputNameStr.Contains(TEXT("Min"), ESearchCase::IgnoreCase))
        {
            MinInput = &Input;
        }
        else if (InputNameStr.Contains(TEXT("Maximum"), ESearchCase::IgnoreCase) ||
                 InputNameStr.Contains(TEXT("Max"), ESearchCase::IgnoreCase))
        {
            MaxInput = &Input;
        }
    }

    if (!MinInput || !MaxInput)
    {
        // List available inputs for debugging
        TArray<FString> AvailableInputs;
        for (const FNiagaraVariable& Input : FunctionInputs)
        {
            AvailableInputs.Add(FString::Printf(TEXT("%s (%s)"),
                *Input.GetName().ToString(), *Input.GetType().GetName()));
        }
        OutError = FString::Printf(TEXT("Could not find Minimum/Maximum inputs on dynamic input '%s'. Available inputs: %s"),
            *DynamicInputNode->GetFunctionName(), *FString::Join(AvailableInputs, TEXT(", ")));
        return false;
    }

    // Parse min value
    FNiagaraVariable MinVariable;
    if (!ParseValueToVariable(MinValueStr, ValueType, MinVariable, OutError))
    {
        OutError = FString::Printf(TEXT("Failed to parse min value '%s': %s"), *MinValueStr, *OutError);
        return false;
    }

    // Parse max value
    FNiagaraVariable MaxVariable;
    if (!ParseValueToVariable(MaxValueStr, ValueType, MaxVariable, OutError))
    {
        OutError = FString::Printf(TEXT("Failed to parse max value '%s': %s"), *MaxValueStr, *OutError);
        return false;
    }

    // Set the min value via override pin
    {
        FNiagaraParameterHandle MinAliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
            MinInput->GetName(),
            FName(*DynamicInputNode->GetFunctionName())
        );

        UEdGraphPin& MinOverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
            *DynamicInputNode,
            MinAliasedHandle,
            ValueType,
            FGuid(),
            FGuid()
        );

        // Set the default value on the pin
        if (ValueType == FNiagaraTypeDefinition::GetFloatDef())
        {
            float FloatVal = MinVariable.GetValue<float>();
            MinOverridePin.DefaultValue = FString::SanitizeFloat(FloatVal);
        }
        else if (ValueType == FNiagaraTypeDefinition::GetIntDef())
        {
            int32 IntVal = MinVariable.GetValue<int32>();
            MinOverridePin.DefaultValue = FString::FromInt(IntVal);
        }
        else if (ValueType == FNiagaraTypeDefinition::GetVec2Def())
        {
            FVector2f Vec = MinVariable.GetValue<FVector2f>();
            MinOverridePin.DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f)"), Vec.X, Vec.Y);
        }
        else if (ValueType == FNiagaraTypeDefinition::GetVec3Def())
        {
            FVector3f Vec = MinVariable.GetValue<FVector3f>();
            MinOverridePin.DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), Vec.X, Vec.Y, Vec.Z);
        }
        else if (ValueType == FNiagaraTypeDefinition::GetVec4Def())
        {
            FVector4f Vec = MinVariable.GetValue<FVector4f>();
            MinOverridePin.DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f,W=%f)"), Vec.X, Vec.Y, Vec.Z, Vec.W);
        }
        else if (ValueType == FNiagaraTypeDefinition::GetColorDef())
        {
            FLinearColor Color = MinVariable.GetValue<FLinearColor>();
            MinOverridePin.DefaultValue = FString::Printf(TEXT("(R=%f,G=%f,B=%f,A=%f)"), Color.R, Color.G, Color.B, Color.A);
        }
    }

    // Set the max value via override pin
    {
        FNiagaraParameterHandle MaxAliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
            MaxInput->GetName(),
            FName(*DynamicInputNode->GetFunctionName())
        );

        UEdGraphPin& MaxOverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
            *DynamicInputNode,
            MaxAliasedHandle,
            ValueType,
            FGuid(),
            FGuid()
        );

        // Set the default value on the pin
        if (ValueType == FNiagaraTypeDefinition::GetFloatDef())
        {
            float FloatVal = MaxVariable.GetValue<float>();
            MaxOverridePin.DefaultValue = FString::SanitizeFloat(FloatVal);
        }
        else if (ValueType == FNiagaraTypeDefinition::GetIntDef())
        {
            int32 IntVal = MaxVariable.GetValue<int32>();
            MaxOverridePin.DefaultValue = FString::FromInt(IntVal);
        }
        else if (ValueType == FNiagaraTypeDefinition::GetVec2Def())
        {
            FVector2f Vec = MaxVariable.GetValue<FVector2f>();
            MaxOverridePin.DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f)"), Vec.X, Vec.Y);
        }
        else if (ValueType == FNiagaraTypeDefinition::GetVec3Def())
        {
            FVector3f Vec = MaxVariable.GetValue<FVector3f>();
            MaxOverridePin.DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), Vec.X, Vec.Y, Vec.Z);
        }
        else if (ValueType == FNiagaraTypeDefinition::GetVec4Def())
        {
            FVector4f Vec = MaxVariable.GetValue<FVector4f>();
            MaxOverridePin.DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f,W=%f)"), Vec.X, Vec.Y, Vec.Z, Vec.W);
        }
        else if (ValueType == FNiagaraTypeDefinition::GetColorDef())
        {
            FLinearColor Color = MaxVariable.GetValue<FLinearColor>();
            MaxOverridePin.DefaultValue = FString::Printf(TEXT("(R=%f,G=%f,B=%f,A=%f)"), Color.R, Color.G, Color.B, Color.A);
        }
    }

    return true;
}

// ============================================================================
// Set Module Random Input
// ============================================================================

bool FNiagaraService::SetModuleRandomInput(const FNiagaraModuleRandomInputParams& Params, FString& OutError)
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
    UNiagaraNodeFunctionCall* ModuleNode = FindModuleNodeByNameForRandom(Graph, Params.ModuleName);
    if (!ModuleNode)
    {
        OutError = FString::Printf(TEXT("Module '%s' not found in stage '%s'"), *Params.ModuleName, *Params.Stage);
        return false;
    }

    // Find the input variable
    FNiagaraVariable InputVariable;
    if (!FindModuleInputVariableForRandom(ModuleNode, System, ScriptUsage, Params.InputName, InputVariable, OutError))
    {
        OutError = FString::Printf(TEXT("Input '%s' not found on module '%s'. %s"), *Params.InputName, *Params.ModuleName, *OutError);
        return false;
    }

    // Get the input type
    FNiagaraTypeDefinition InputType = InputVariable.GetType();

    // Get the appropriate dynamic input script path
    FString DynamicInputPath = GetUniformRangedScriptPath(InputType);
    if (DynamicInputPath.IsEmpty())
    {
        OutError = FString::Printf(TEXT("No UniformRanged dynamic input available for type '%s'. Supported types: Float, Int, Vec2, Vec3, Vec4, LinearColor"),
            *InputType.GetName());
        return false;
    }

    // Load the dynamic input script
    UNiagaraScript* DynamicInputScript = LoadObject<UNiagaraScript>(nullptr, *DynamicInputPath);
    if (!DynamicInputScript)
    {
        OutError = FString::Printf(TEXT("Failed to load UniformRanged dynamic input script: %s"), *DynamicInputPath);
        return false;
    }

    UE_LOG(LogNiagaraService, Log, TEXT("Loaded dynamic input script: %s"), *DynamicInputScript->GetPathName());

    // Mark system for modification
    System->Modify();
    Graph->Modify();

    // Create the aliased module parameter handle
    FNiagaraParameterHandle AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
        InputVariable.GetName(),
        FName(*ModuleNode->GetFunctionName())
    );

    // Get or create override pin for the input
    UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
        *ModuleNode,
        AliasedHandle,
        InputType,
        FGuid(),
        FGuid()
    );

    // Remove any existing override by breaking pin links
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

    // Now configure the min/max values on the Dynamic Input's nested inputs
    if (!SetMinMaxOnDynamicInput(DynamicInputNode, System, InputType, Params.MinValue, Params.MaxValue, OutError))
    {
        // OutError already set
        return false;
    }

    UE_LOG(LogNiagaraService, Log, TEXT("Successfully configured Dynamic Input '%s' with Min=%s, Max=%s"),
        *DynamicInputNode->GetFunctionName(), *Params.MinValue, *Params.MaxValue);

    // Mark system dirty
    MarkSystemDirty(System);

    // Notify graph of changes
    Graph->NotifyGraphChanged();

    // CRITICAL: Force system recompile for runtime to pick up graph changes
    System->RequestCompile(false);

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set random input '%s' on module '%s' with range [%s, %s]"),
        *Params.InputName, *Params.ModuleName, *Params.MinValue, *Params.MaxValue);

    return true;
}
