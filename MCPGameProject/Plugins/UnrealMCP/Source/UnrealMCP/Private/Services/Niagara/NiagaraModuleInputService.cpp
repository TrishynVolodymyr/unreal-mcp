// NiagaraModuleInputService.cpp - SetModuleInput implementation
// Split from NiagaraModuleService.cpp for file size management

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraScriptSource.h"
#include "NiagaraTypes.h"
#include "NiagaraCommon.h"
#include "EdGraphSchema_Niagara.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"

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

    // Handle enabled state if specified
    if (Params.Enabled.IsSet())
    {
        FNiagaraStackGraphUtilities::SetModuleIsEnabled(*ModuleNode, Params.Enabled.GetValue());
        UE_LOG(LogNiagaraService, Log, TEXT("Set module '%s' enabled state to %s"),
            *Params.ModuleName, Params.Enabled.GetValue() ? TEXT("true") : TEXT("false"));

        // If only setting enabled (no input name), we're done
        if (Params.InputName.IsEmpty())
        {
            MarkSystemDirty(System);
            Graph->NotifyGraphChanged();
            System->RequestCompile(false);
            System->WaitForCompilationComplete();
            RefreshEditors(System);
            return true;
        }
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
        // Handle enum pins - convert display names to internal names
        else if (UEnum* EnumType = Cast<UEnum>(FoundPin->PinType.PinSubCategoryObject.Get()))
        {
            // Try to find the enum value by name
            int64 EnumValue = INDEX_NONE;
            FString InternalName;

            // First try exact match with internal name (e.g., "NewEnumerator0")
            EnumValue = EnumType->GetValueByNameString(ValueStr);

            if (EnumValue == INDEX_NONE)
            {
                // Try to find by short name or display name
                for (int32 i = 0; i < EnumType->NumEnums() - 1; ++i) // -1 to skip MAX value
                {
                    FString EnumName = EnumType->GetNameStringByIndex(i);
                    FString ShortName = EnumName;

                    // Remove enum prefix (e.g., "ESplineCoordinateSpace::World" -> "World")
                    int32 ColonPos;
                    if (EnumName.FindLastChar(':', ColonPos))
                    {
                        ShortName = EnumName.RightChop(ColonPos + 1);
                    }

                    // Also get the display name for user-defined enums
                    FText DisplayNameText = EnumType->GetDisplayNameTextByIndex(i);
                    FString DisplayName = DisplayNameText.ToString();

                    if (ShortName.Equals(ValueStr, ESearchCase::IgnoreCase) ||
                        EnumName.Equals(ValueStr, ESearchCase::IgnoreCase) ||
                        DisplayName.Equals(ValueStr, ESearchCase::IgnoreCase))
                    {
                        EnumValue = i;
                        InternalName = EnumName;
                        break;
                    }
                }
            }
            else
            {
                // Get the internal name for the found value
                InternalName = EnumType->GetNameStringByValue(EnumValue);
            }

            if (EnumValue != INDEX_NONE)
            {
                if (InternalName.IsEmpty())
                {
                    InternalName = EnumType->GetNameStringByValue(EnumValue);
                }
                FoundPin->DefaultValue = InternalName;
                bFoundExposedPin = true;
                UE_LOG(LogNiagaraService, Log, TEXT("Set enum pin '%s' to '%s' (internal: '%s')"),
                    *FoundPin->PinName.ToString(), *ValueStr, *InternalName);
            }
            else
            {
                // Build list of valid enum values for helpful error message
                TArray<FString> ValidValues;
                for (int32 i = 0; i < EnumType->NumEnums() - 1; ++i)
                {
                    FText DisplayText = EnumType->GetDisplayNameTextByIndex(i);
                    FString InternalEnumName = EnumType->GetNameStringByIndex(i);
                    ValidValues.Add(FString::Printf(TEXT("'%s' (internal: %s)"), *DisplayText.ToString(), *InternalEnumName));
                }
                OutError = FString::Printf(TEXT("Enum value '%s' not found in enum '%s'. Valid values: %s"),
                    *ValueStr, *EnumType->GetName(), *FString::Join(ValidValues, TEXT(", ")));
                return false;
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
        // Also check for enum types - they're stored as int32 but need display name conversion
        UEnum* InputEnumType = InputType.GetEnum();
        bool bIsRapidIterationType =
            InputType == FNiagaraTypeDefinition::GetFloatDef() ||
            InputType == FNiagaraTypeDefinition::GetIntDef() ||
            InputType == FNiagaraTypeDefinition::GetBoolDef() ||
            InputType == FNiagaraTypeDefinition::GetVec2Def() ||
            InputType == FNiagaraTypeDefinition::GetVec3Def() ||
            InputType == FNiagaraTypeDefinition::GetVec4Def() ||
            InputType == FNiagaraTypeDefinition::GetColorDef() ||
            InputType == FNiagaraTypeDefinition::GetQuatDef() ||
            InputEnumType != nullptr;  // Enum types are rapid iteration compatible

        if (!bIsRapidIterationType)
        {
            OutError = FString::Printf(TEXT("Input type '%s' for '%s' is not a rapid iteration type. Only Float, Int, Bool, Vec2, Vec3, Vec4, Color, Quat, and Enum types are supported."),
                *InputType.GetName(), *Params.InputName);
            return false;
        }

        // Clean up the value string (remove parentheses and extra spaces for vector/color types)
        FString CleanValueStr = ValueStr;
        CleanValueStr.TrimStartAndEndInline();
        CleanValueStr = CleanValueStr.Replace(TEXT("("), TEXT(""));
        CleanValueStr = CleanValueStr.Replace(TEXT(")"), TEXT(""));
        CleanValueStr = CleanValueStr.Replace(TEXT(" "), TEXT(""));

        // Remove common component prefixes (R=, G=, B=, A=, X=, Y=, Z=, W=) for UE format support
        // This allows parsing "(R=1.0, G=0.7, B=0.2, A=1.0)" or "(X=0, Y=0, Z=100)"
        CleanValueStr = CleanValueStr.Replace(TEXT("R="), TEXT(""));
        CleanValueStr = CleanValueStr.Replace(TEXT("G="), TEXT(""));
        CleanValueStr = CleanValueStr.Replace(TEXT("B="), TEXT(""));
        CleanValueStr = CleanValueStr.Replace(TEXT("A="), TEXT(""));
        CleanValueStr = CleanValueStr.Replace(TEXT("X="), TEXT(""));
        CleanValueStr = CleanValueStr.Replace(TEXT("Y="), TEXT(""));
        CleanValueStr = CleanValueStr.Replace(TEXT("Z="), TEXT(""));
        CleanValueStr = CleanValueStr.Replace(TEXT("W="), TEXT(""));

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
        else if (InputEnumType)
        {
            // Handle enum types - convert display name to internal value
            int32 EnumValue = INDEX_NONE;

            // First try exact match with internal name (e.g., "NewEnumerator0", "NewEnumerator3")
            int64 ValueByName = InputEnumType->GetValueByNameString(CleanValueStr);
            if (ValueByName != INDEX_NONE)
            {
                EnumValue = static_cast<int32>(ValueByName);
            }
            else
            {
                // Try to find by short name or display name
                for (int32 i = 0; i < InputEnumType->NumEnums() - 1; ++i) // -1 to skip MAX value
                {
                    FString EnumName = InputEnumType->GetNameStringByIndex(i);
                    FString ShortName = EnumName;

                    // Remove enum prefix (e.g., "ESubUVAnimationMode::Linear" -> "Linear")
                    int32 ColonPos;
                    if (EnumName.FindLastChar(':', ColonPos))
                    {
                        ShortName = EnumName.RightChop(ColonPos + 1);
                    }

                    // Get the display name for user-defined enums (e.g., "Infinite Loop")
                    FText DisplayNameText = InputEnumType->GetDisplayNameTextByIndex(i);
                    FString DisplayName = DisplayNameText.ToString();

                    // Match against short name, full internal name, or display name
                    if (ShortName.Equals(CleanValueStr, ESearchCase::IgnoreCase) ||
                        EnumName.Equals(CleanValueStr, ESearchCase::IgnoreCase) ||
                        DisplayName.Equals(CleanValueStr, ESearchCase::IgnoreCase))
                    {
                        EnumValue = i;
                        UE_LOG(LogNiagaraService, Log, TEXT("Matched enum value '%s' to index %d (display: '%s', internal: '%s')"),
                            *CleanValueStr, i, *DisplayName, *EnumName);
                        break;
                    }
                }
            }

            if (EnumValue != INDEX_NONE)
            {
                TempVariable.AllocateData();
                TempVariable.SetValue<int32>(EnumValue);
                bValueSet = true;
                UE_LOG(LogNiagaraService, Log, TEXT("Set enum input '%s' to value %d (from '%s')"),
                    *Params.InputName, EnumValue, *CleanValueStr);
            }
            else
            {
                // Build list of valid enum values for helpful error message
                TArray<FString> ValidValues;
                for (int32 i = 0; i < InputEnumType->NumEnums() - 1; ++i)
                {
                    FText DisplayText = InputEnumType->GetDisplayNameTextByIndex(i);
                    FString InternalEnumName = InputEnumType->GetNameStringByIndex(i);
                    ValidValues.Add(FString::Printf(TEXT("'%s' (internal: %s)"), *DisplayText.ToString(), *InternalEnumName));
                }
                OutError = FString::Printf(TEXT("Enum value '%s' not found in enum '%s'. Valid values: %s"),
                    *CleanValueStr, *InputEnumType->GetName(), *FString::Join(ValidValues, TEXT(", ")));
                return false;
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

        // Collect ALL affected scripts - CRITICAL for avoiding ParameterMap traversal errors
        // Niagara expects rapid iteration params to be set on ALL scripts that might reference them
        // This mirrors the behavior of FNiagaraStackGraphUtilities::FindAffectedScripts (not exported)
        TArray<UNiagaraScript*> AffectedScripts;

        // Add system spawn and update scripts
        UNiagaraScript* SystemSpawnScript = System->GetSystemSpawnScript();
        UNiagaraScript* SystemUpdateScript = System->GetSystemUpdateScript();
        if (SystemSpawnScript)
        {
            AffectedScripts.Add(SystemSpawnScript);
        }
        if (SystemUpdateScript)
        {
            AffectedScripts.Add(SystemUpdateScript);
        }

        // Add emitter scripts that contain the module's usage
        TArray<UNiagaraScript*> EmitterScripts;
        EmitterData->GetScripts(EmitterScripts, false);
        for (UNiagaraScript* EmitterScript : EmitterScripts)
        {
            if (EmitterScript && EmitterScript->ContainsUsage(ScriptUsage))
            {
                AffectedScripts.Add(EmitterScript);
            }
        }

        // Set the parameter data on ALL affected scripts' RapidIterationParameters stores
        bool bAddParameterIfMissing = true;
        for (UNiagaraScript* AffectedScript : AffectedScripts)
        {
            if (AffectedScript)
            {
                AffectedScript->Modify();
                AffectedScript->RapidIterationParameters.SetParameterData(
                    RapidIterationVariable.GetData(),
                    RapidIterationVariable,
                    bAddParameterIfMissing
                );
            }
        }

        UE_LOG(LogNiagaraService, Log, TEXT("Set input '%s' on module '%s' via rapid iteration parameter '%s' on %d affected scripts"),
            *Params.InputName, *Params.ModuleName, *RapidIterationVariable.GetName().ToString(), AffectedScripts.Num());
    }

    // Mark system dirty and force recompile to update UI
    MarkSystemDirty(System);
    Graph->NotifyGraphChanged();
    System->RequestCompile(false);
    System->WaitForCompilationComplete();

    // Refresh editors to show updated values
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set input '%s' on module '%s' in emitter '%s' stage '%s' to '%s'"),
        *Params.InputName, *Params.ModuleName, *Params.EmitterName, *Params.Stage, *ValueStr);

    return true;
}
