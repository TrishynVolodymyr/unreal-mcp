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
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"

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
    // Use a multi-pass approach: exact match first, then suffix match, then contains match
    bool bFoundExposedPin = false;
    UEdGraphPin* FoundPin = nullptr;

    // Pass 1: Exact match (highest priority)
    for (UEdGraphPin* Pin : ModuleNode->Pins)
    {
        if (Pin->Direction != EGPD_Input)
        {
            continue;
        }

        FString PinName = Pin->PinName.ToString();
        if (PinName.Equals(Params.InputName, ESearchCase::IgnoreCase))
        {
            FoundPin = Pin;
            break;
        }
    }

    // Pass 2: Suffix match with DOT separator only (e.g., "Drag" matches "Module.Drag")
    // This prioritizes module parameter pins over static switch pins like "Use Linear Drag"
    if (!FoundPin)
    {
        for (UEdGraphPin* Pin : ModuleNode->Pins)
        {
            if (Pin->Direction != EGPD_Input)
            {
                continue;
            }

            FString PinName = Pin->PinName.ToString();
            if (PinName.EndsWith(Params.InputName, ESearchCase::IgnoreCase))
            {
                // Only match if preceded by a dot (module parameter style) or is the full string
                int32 MatchPos = PinName.Len() - Params.InputName.Len();
                if (MatchPos == 0 || (MatchPos > 0 && PinName[MatchPos - 1] == '.'))
                {
                    FoundPin = Pin;
                    break;
                }
            }
        }
    }

    // NOTE: We intentionally do NOT use Contains() matching here as it causes false positives
    // like "Drag" matching "Use Linear Drag". Users should provide exact or dot-suffixed names.

    // If we found an exposed pin, set its value
    if (FoundPin)
    {
        FString TypeHint = Params.ValueType.ToLower();

        if (TypeHint == TEXT("vector") || TypeHint == TEXT("float3"))
        {
            TArray<FString> Components;
            ValueStr.ParseIntoArray(Components, TEXT(","), true);
            if (Components.Num() >= 3)
            {
                FVector Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]));
                FoundPin->DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), Vec.X, Vec.Y, Vec.Z);
                bFoundExposedPin = true;
            }
        }
        else
        {
            FoundPin->DefaultValue = ValueStr;
            bFoundExposedPin = true;
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

        // Find the input by name - support multiple matching modes:
        // 1. Full name match (e.g., "Module.Particles.Lifetime")
        // 2. Suffix match (e.g., "Particles.Lifetime" matches "Module.Particles.Lifetime")
        // 3. Simple name match (e.g., "Lifetime" matches "Module.Particles.Lifetime")
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

            // Try suffix match (user provides "Particles.Lifetime" for "Module.Particles.Lifetime")
            if (InputNameStr.EndsWith(Params.InputName, ESearchCase::IgnoreCase))
            {
                // Ensure it's a proper suffix (preceded by a dot or is the full string)
                int32 MatchPos = InputNameStr.Len() - Params.InputName.Len();
                if (MatchPos == 0 || (MatchPos > 0 && InputNameStr[MatchPos - 1] == '.'))
                {
                    FoundInput = &Input;
                    break;
                }
            }

            // Try simple name match (just the last component after the last dot)
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

        // Get input type
        FNiagaraTypeDefinition InputType = FoundInput->GetType();

        // Check if this is a rapid iteration type (simple types that can be set directly on the parameter store)
        // We implement our own check since FNiagaraStackGraphUtilities::IsRapidIterationType is not exported
        bool bIsRapidIterationType =
            InputType == FNiagaraTypeDefinition::GetFloatDef() ||
            InputType == FNiagaraTypeDefinition::GetIntDef() ||
            InputType == FNiagaraTypeDefinition::GetBoolDef() ||
            InputType == FNiagaraTypeDefinition::GetVec2Def() ||
            InputType == FNiagaraTypeDefinition::GetVec3Def() ||
            InputType == FNiagaraTypeDefinition::GetVec4Def() ||
            InputType == FNiagaraTypeDefinition::GetColorDef() ||
            InputType == FNiagaraTypeDefinition::GetQuatDef();

        if (!bIsRapidIterationType)
        {
            OutError = FString::Printf(TEXT("Input type '%s' for '%s' is not a rapid iteration type. Only Float, Int, Bool, Vec2, Vec3, Vec4, Color, Quat are supported."),
                *InputType.GetName(), *Params.InputName);
            return false;
        }

        // Clean up the value string (remove parentheses and extra spaces for vector/color types)
        FString CleanValueStr = ValueStr;
        CleanValueStr.TrimStartAndEndInline();
        CleanValueStr = CleanValueStr.Replace(TEXT("("), TEXT(""));
        CleanValueStr = CleanValueStr.Replace(TEXT(")"), TEXT(""));
        CleanValueStr = CleanValueStr.Replace(TEXT(" "), TEXT(""));

        // Parse the value into a variable
        FNiagaraVariable TempVariable(InputType, NAME_None);
        bool bValueSet = false;

        if (InputType == FNiagaraTypeDefinition::GetFloatDef())
        {
            float FloatValue = FCString::Atof(*CleanValueStr);
            TempVariable.AllocateData();
            TempVariable.SetValue<float>(FloatValue);
            bValueSet = true;
        }
        else if (InputType == FNiagaraTypeDefinition::GetIntDef())
        {
            int32 IntValue = FCString::Atoi(*CleanValueStr);
            TempVariable.AllocateData();
            TempVariable.SetValue<int32>(IntValue);
            bValueSet = true;
        }
        else if (InputType == FNiagaraTypeDefinition::GetBoolDef())
        {
            bool BoolValue = CleanValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || CleanValueStr.Equals(TEXT("1"));
            TempVariable.AllocateData();
            TempVariable.SetValue<FNiagaraBool>(FNiagaraBool(BoolValue));
            bValueSet = true;
        }
        else if (InputType == FNiagaraTypeDefinition::GetVec2Def())
        {
            TArray<FString> Components;
            CleanValueStr.ParseIntoArray(Components, TEXT(","), true);
            if (Components.Num() >= 2)
            {
                FVector2f Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]));
                TempVariable.AllocateData();
                TempVariable.SetValue<FVector2f>(Vec);
                bValueSet = true;
            }
        }
        else if (InputType == FNiagaraTypeDefinition::GetVec3Def())
        {
            TArray<FString> Components;
            CleanValueStr.ParseIntoArray(Components, TEXT(","), true);
            if (Components.Num() >= 3)
            {
                FVector3f Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]));
                TempVariable.AllocateData();
                TempVariable.SetValue<FVector3f>(Vec);
                bValueSet = true;
            }
        }
        else if (InputType == FNiagaraTypeDefinition::GetVec4Def())
        {
            TArray<FString> Components;
            CleanValueStr.ParseIntoArray(Components, TEXT(","), true);
            if (Components.Num() >= 4)
            {
                FVector4f Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]),
                              FCString::Atof(*Components[2]), FCString::Atof(*Components[3]));
                TempVariable.AllocateData();
                TempVariable.SetValue<FVector4f>(Vec);
                bValueSet = true;
            }
        }
        else if (InputType == FNiagaraTypeDefinition::GetQuatDef())
        {
            TArray<FString> Components;
            CleanValueStr.ParseIntoArray(Components, TEXT(","), true);
            if (Components.Num() >= 4)
            {
                FQuat4f Quat(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]),
                             FCString::Atof(*Components[2]), FCString::Atof(*Components[3]));
                TempVariable.AllocateData();
                TempVariable.SetValue<FQuat4f>(Quat);
                bValueSet = true;
            }
        }
        else if (InputType == FNiagaraTypeDefinition::GetColorDef())
        {
            TArray<FString> Components;
            CleanValueStr.ParseIntoArray(Components, TEXT(","), true);
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
            OutError = FString::Printf(TEXT("Could not parse value '%s' for input type '%s'"),
                *ValueStr, *InputType.GetName());
            return false;
        }

        // Create the aliased module parameter name (ModuleName.InputName format)
        FNiagaraParameterHandle AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
            FoundInput->GetName(),
            FName(*ModuleNode->GetFunctionName())
        );

        // For rapid iteration types, we must use Script->RapidIterationParameters directly
        // NOT override pins - override pins cause graph corruption when other graph changes occur

        // Get emitter unique name for the rapid iteration parameter name
        FString UniqueEmitterName = EmitterHandle.GetInstance().Emitter->GetUniqueEmitterName();

        // Create the input variable with the aliased name
        FNiagaraVariable InputVariable(InputType, FName(*AliasedHandle.GetParameterHandleString().ToString()));

        // Convert to rapid iteration constant name using the exported utility
        FNiagaraVariable RapidIterationVariable = FNiagaraUtilities::ConvertVariableToRapidIterationConstantName(
            InputVariable,
            *UniqueEmitterName,
            ScriptUsage
        );

        // Allocate data for the rapid iteration variable
        RapidIterationVariable.AllocateData();

        // Copy the parsed value data to the rapid iteration variable
        FMemory::Memcpy(RapidIterationVariable.GetData(), TempVariable.GetData(), InputType.GetSize());

        // Set the parameter data on the script's RapidIterationParameters store
        Script->Modify();
        bool bAddParameterIfMissing = true;
        Script->RapidIterationParameters.SetParameterData(
            RapidIterationVariable.GetData(),
            RapidIterationVariable,
            bAddParameterIfMissing
        );

        UE_LOG(LogNiagaraService, Log, TEXT("Set input '%s' on module '%s' via rapid iteration parameter '%s'"),
            *Params.InputName, *Params.ModuleName, *RapidIterationVariable.GetName().ToString());
    }

    // Mark system dirty (but don't force recompile - rapid iteration params work without it)
    MarkSystemDirty(System);

    // Refresh editors to show updated values
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set input '%s' on module '%s' in emitter '%s' stage '%s' to '%s'"),
        *Params.InputName, *Params.ModuleName, *Params.EmitterName, *Params.Stage, *ValueStr);

    return true;
}
