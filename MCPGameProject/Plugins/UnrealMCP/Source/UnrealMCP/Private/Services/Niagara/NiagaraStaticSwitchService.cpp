// NiagaraStaticSwitchService.cpp - Static Switch Module Operations

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraScriptSource.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"

bool FNiagaraService::SetModuleStaticSwitch(const FNiagaraModuleStaticSwitchParams& Params, FString& OutError)
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

    // Mark system for modification
    System->Modify();

    // Find the static switch pin by name using direct pin search
    // We iterate through module input pins and match by name
    UEdGraphPin* SwitchPin = nullptr;
    FString NormalizedSwitchName = Params.SwitchName.Replace(TEXT(" "), TEXT(""));

    for (UEdGraphPin* Pin : ModuleNode->Pins)
    {
        if (Pin->Direction != EGPD_Input)
        {
            continue;
        }

        FString PinName = Pin->PinName.ToString();
        FString NormalizedPinName = PinName.Replace(TEXT(" "), TEXT(""));

        // Try exact match first
        if (PinName.Equals(Params.SwitchName, ESearchCase::IgnoreCase) ||
            NormalizedPinName.Equals(NormalizedSwitchName, ESearchCase::IgnoreCase))
        {
            SwitchPin = Pin;
            break;
        }
    }

    if (!SwitchPin)
    {
        // Build a helpful error message listing available pins
        TArray<FString> AvailablePins;
        for (UEdGraphPin* Pin : ModuleNode->Pins)
        {
            if (Pin->Direction == EGPD_Input)
            {
                AvailablePins.Add(Pin->PinName.ToString());
            }
        }
        OutError = FString::Printf(TEXT("Static switch '%s' not found on module '%s'. Available pins: %s"),
            *Params.SwitchName, *Params.ModuleName, *FString::Join(AvailablePins, TEXT(", ")));
        return false;
    }

    // Determine the pin type and set the value accordingly
    FString ValueToSet = Params.Value;

    // Handle enum pins - convert display names to internal names
    if (UEnum* EnumType = Cast<UEnum>(SwitchPin->PinType.PinSubCategoryObject.Get()))
    {
        int64 EnumValue = INDEX_NONE;
        FString InternalName;

        // First try parsing as an index
        if (Params.Value.IsNumeric())
        {
            int32 Index = FCString::Atoi(*Params.Value);
            if (Index >= 0 && Index < EnumType->NumEnums() - 1) // -1 to skip MAX value
            {
                EnumValue = Index;
                InternalName = EnumType->GetNameStringByIndex(Index);
            }
        }

        // If not a valid index, try name matching
        if (EnumValue == INDEX_NONE)
        {
            // Try exact match with internal name (e.g., "NewEnumerator0")
            EnumValue = EnumType->GetValueByNameString(Params.Value);

            if (EnumValue == INDEX_NONE)
            {
                // Try to find by short name or display name
                for (int32 i = 0; i < EnumType->NumEnums() - 1; ++i)
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

                    if (ShortName.Equals(Params.Value, ESearchCase::IgnoreCase) ||
                        EnumName.Equals(Params.Value, ESearchCase::IgnoreCase) ||
                        DisplayName.Equals(Params.Value, ESearchCase::IgnoreCase))
                    {
                        EnumValue = i;
                        InternalName = EnumName;
                        break;
                    }
                }
            }
            else
            {
                InternalName = EnumType->GetNameStringByValue(EnumValue);
            }
        }

        if (EnumValue == INDEX_NONE)
        {
            // Build list of valid enum values for helpful error message
            TArray<FString> ValidValues;
            for (int32 i = 0; i < EnumType->NumEnums() - 1; ++i)
            {
                FText DisplayText = EnumType->GetDisplayNameTextByIndex(i);
                FString InternalEnumName = EnumType->GetNameStringByIndex(i);
                ValidValues.Add(FString::Printf(TEXT("%d='%s' (internal: %s)"), i, *DisplayText.ToString(), *InternalEnumName));
            }
            OutError = FString::Printf(TEXT("Enum value '%s' not found for switch '%s'. Valid values: %s"),
                *Params.Value, *Params.SwitchName, *FString::Join(ValidValues, TEXT(", ")));
            return false;
        }

        if (InternalName.IsEmpty())
        {
            InternalName = EnumType->GetNameStringByValue(EnumValue);
        }
        ValueToSet = InternalName;

        UE_LOG(LogNiagaraService, Log, TEXT("Set static switch '%s' enum value to '%s' (internal: '%s', index: %lld)"),
            *Params.SwitchName, *Params.Value, *InternalName, EnumValue);
    }
    // Handle bool pins
    else if (SwitchPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
    {
        // Normalize bool values
        if (Params.Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Params.Value == TEXT("1"))
        {
            ValueToSet = TEXT("true");
        }
        else if (Params.Value.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Params.Value == TEXT("0"))
        {
            ValueToSet = TEXT("false");
        }
        else
        {
            OutError = FString::Printf(TEXT("Invalid bool value '%s' for switch '%s'. Use 'true', 'false', '0', or '1'."),
                *Params.Value, *Params.SwitchName);
            return false;
        }

        UE_LOG(LogNiagaraService, Log, TEXT("Set static switch '%s' bool value to '%s'"),
            *Params.SwitchName, *ValueToSet);
    }
    // Handle integer pins
    else if (Params.Value.IsNumeric())
    {
        ValueToSet = Params.Value;
        UE_LOG(LogNiagaraService, Log, TEXT("Set static switch '%s' integer value to '%s'"),
            *Params.SwitchName, *ValueToSet);
    }

    // Set the pin value
    SwitchPin->DefaultValue = ValueToSet;

    // Mark dirty, notify, and recompile - static switches require recompilation
    MarkSystemDirty(System);
    Graph->NotifyGraphChanged();
    System->RequestCompile(false);
    System->WaitForCompilationComplete();
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Successfully set static switch '%s' on module '%s' to '%s'"),
        *Params.SwitchName, *Params.ModuleName, *ValueToSet);

    return true;
}
