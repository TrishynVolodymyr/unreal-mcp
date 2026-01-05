// NiagaraMetadataService.cpp - Metadata queries
// GetMetadata, GetModuleInputs

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraScriptSource.h"
#include "NiagaraTypes.h"
#include "NiagaraCommon.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"

bool FNiagaraService::GetMetadata(const FString& AssetPath, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata)
{
    OutMetadata = MakeShared<FJsonObject>();

    // Try to load as system first
    UNiagaraSystem* System = FindSystem(AssetPath);
    if (System)
    {
        OutMetadata->SetStringField(TEXT("asset_type"), TEXT("NiagaraSystem"));
        OutMetadata->SetStringField(TEXT("asset_path"), AssetPath);
        OutMetadata->SetStringField(TEXT("asset_name"), System->GetName());
        AddSystemMetadata(System, Fields, OutMetadata);
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

bool FNiagaraService::GetModuleInputs(const FString& SystemPath, const FString& EmitterName, const FString& ModuleName, const FString& Stage, TSharedPtr<FJsonObject>& OutInputs)
{
    OutInputs = MakeShared<FJsonObject>();

    // Find the system
    UNiagaraSystem* System = FindSystem(SystemPath);
    if (!System)
    {
        OutInputs->SetBoolField(TEXT("success"), false);
        OutInputs->SetStringField(TEXT("error"), FString::Printf(TEXT("System not found: %s"), *SystemPath));
        return false;
    }

    // Find the emitter handle by name
    int32 EmitterIndex = FindEmitterHandleIndex(System, EmitterName);
    if (EmitterIndex == INDEX_NONE)
    {
        OutInputs->SetBoolField(TEXT("success"), false);
        OutInputs->SetStringField(TEXT("error"), FString::Printf(TEXT("Emitter '%s' not found in system"), *EmitterName));
        return false;
    }

    const FNiagaraEmitterHandle& EmitterHandle = System->GetEmitterHandle(EmitterIndex);
    FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
    if (!EmitterData)
    {
        OutInputs->SetBoolField(TEXT("success"), false);
        OutInputs->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not get emitter data for '%s'"), *EmitterName));
        return false;
    }

    // Convert stage to script usage
    FString Error;
    uint8 UsageValue;
    if (!GetScriptUsageFromStage(Stage, UsageValue, Error))
    {
        OutInputs->SetBoolField(TEXT("success"), false);
        OutInputs->SetStringField(TEXT("error"), Error);
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
        OutInputs->SetBoolField(TEXT("success"), false);
        OutInputs->SetStringField(TEXT("error"), FString::Printf(TEXT("Unsupported stage '%s'"), *Stage));
        return false;
    }

    if (!Script)
    {
        OutInputs->SetBoolField(TEXT("success"), false);
        OutInputs->SetStringField(TEXT("error"), FString::Printf(TEXT("Script not found for stage '%s'"), *Stage));
        return false;
    }

    // Get the script source and graph
    UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
    if (!ScriptSource)
    {
        OutInputs->SetBoolField(TEXT("success"), false);
        OutInputs->SetStringField(TEXT("error"), TEXT("Could not get script source"));
        return false;
    }

    UNiagaraGraph* Graph = ScriptSource->NodeGraph;
    if (!Graph)
    {
        OutInputs->SetBoolField(TEXT("success"), false);
        OutInputs->SetStringField(TEXT("error"), TEXT("Could not get script graph"));
        return false;
    }

    // Find the module node by name
    UNiagaraNodeFunctionCall* ModuleNode = nullptr;
    FString NormalizedSearchName = ModuleName.Replace(TEXT(" "), TEXT(""));
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
        OutInputs->SetBoolField(TEXT("success"), false);
        OutInputs->SetStringField(TEXT("error"), FString::Printf(TEXT("Module '%s' not found in stage '%s'"), *ModuleName, *Stage));
        return false;
    }

    // Build the response
    OutInputs->SetBoolField(TEXT("success"), true);
    OutInputs->SetStringField(TEXT("module_name"), ModuleNode->GetFunctionName());
    OutInputs->SetStringField(TEXT("emitter_name"), EmitterName);
    OutInputs->SetStringField(TEXT("stage"), Stage);

    // Get module inputs using the Stack API (same as SetModuleInput)
    FCompileConstantResolver ConstantResolver(System, ScriptUsage);
    TArray<FNiagaraVariable> ModuleInputs;
    FNiagaraStackGraphUtilities::GetStackFunctionInputs(
        *ModuleNode,
        ModuleInputs,
        ConstantResolver,
        FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly
    );

    // Get emitter unique name for rapid iteration parameter lookup
    FString UniqueEmitterName = EmitterHandle.GetInstance().Emitter->GetUniqueEmitterName();

    // Collect all module inputs and their values
    TArray<TSharedPtr<FJsonValue>> InputsArray;
    for (const FNiagaraVariable& Input : ModuleInputs)
    {
        TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();

        // Extract simple name from "Module.InputName" format
        FString FullName = Input.GetName().ToString();
        FString SimpleName = FullName;
        int32 DotIndex = INDEX_NONE;
        if (FullName.FindLastChar('.', DotIndex))
        {
            SimpleName = FullName.RightChop(DotIndex + 1);
        }

        InputObj->SetStringField(TEXT("name"), SimpleName);
        InputObj->SetStringField(TEXT("full_name"), FullName);

        FNiagaraTypeDefinition InputType = Input.GetType();
        InputObj->SetStringField(TEXT("type"), InputType.GetName());

        // Try to read the actual value from RapidIterationParameters
        FString ValueStr = TEXT("[Unknown]");
        bool bFoundValue = false;

        // Create the aliased handle and rapid iteration variable name
        FNiagaraParameterHandle AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
            Input.GetName(),
            FName(*ModuleNode->GetFunctionName())
        );

        FNiagaraVariable InputVariable(InputType, FName(*AliasedHandle.GetParameterHandleString().ToString()));
        FNiagaraVariable RapidIterationVariable = FNiagaraUtilities::ConvertVariableToRapidIterationConstantName(
            InputVariable,
            *UniqueEmitterName,
            ScriptUsage
        );

        // Try to get the value from the script's RapidIterationParameters using safe FNiagaraVariable::SetData
        const uint8* ParameterData = Script->RapidIterationParameters.GetParameterData(RapidIterationVariable);

        if (ParameterData)
        {
            // Use FNiagaraVariable's safe SetData method, then read from the variable
            FNiagaraVariable TempVar(InputType, RapidIterationVariable.GetName());
            TempVar.SetData(ParameterData);

            // Now read from the variable's safely allocated data
            const uint8* SafeData = TempVar.GetData();
            if (SafeData)
            {
                if (InputType == FNiagaraTypeDefinition::GetFloatDef())
                {
                    float FloatValue = TempVar.GetValue<float>();
                    ValueStr = FString::Printf(TEXT("%.4f"), FloatValue);
                    bFoundValue = true;
                }
                else if (InputType == FNiagaraTypeDefinition::GetIntDef())
                {
                    int32 IntValue = TempVar.GetValue<int32>();
                    ValueStr = FString::Printf(TEXT("%d"), IntValue);
                    bFoundValue = true;
                }
                else if (InputType == FNiagaraTypeDefinition::GetBoolDef())
                {
                    FNiagaraBool BoolValue = TempVar.GetValue<FNiagaraBool>();
                    ValueStr = BoolValue.IsValid() && BoolValue.GetValue() ? TEXT("true") : TEXT("false");
                    bFoundValue = true;
                }
                else if (InputType == FNiagaraTypeDefinition::GetVec2Def())
                {
                    FVector2f Vec = TempVar.GetValue<FVector2f>();
                    ValueStr = FString::Printf(TEXT("(%.4f, %.4f)"), Vec.X, Vec.Y);
                    bFoundValue = true;
                }
                else if (InputType == FNiagaraTypeDefinition::GetVec3Def())
                {
                    FVector3f Vec = TempVar.GetValue<FVector3f>();
                    ValueStr = FString::Printf(TEXT("(%.4f, %.4f, %.4f)"), Vec.X, Vec.Y, Vec.Z);
                    bFoundValue = true;
                }
                else if (InputType == FNiagaraTypeDefinition::GetVec4Def())
                {
                    FVector4f Vec = TempVar.GetValue<FVector4f>();
                    ValueStr = FString::Printf(TEXT("(%.4f, %.4f, %.4f, %.4f)"), Vec.X, Vec.Y, Vec.Z, Vec.W);
                    bFoundValue = true;
                }
                else if (InputType == FNiagaraTypeDefinition::GetColorDef())
                {
                    FLinearColor Color = TempVar.GetValue<FLinearColor>();
                    ValueStr = FString::Printf(TEXT("(R=%.4f, G=%.4f, B=%.4f, A=%.4f)"), Color.R, Color.G, Color.B, Color.A);
                    bFoundValue = true;
                }
                else if (InputType == FNiagaraTypeDefinition::GetQuatDef())
                {
                    FQuat4f Quat = TempVar.GetValue<FQuat4f>();
                    ValueStr = FString::Printf(TEXT("(X=%.4f, Y=%.4f, Z=%.4f, W=%.4f)"), Quat.X, Quat.Y, Quat.Z, Quat.W);
                    bFoundValue = true;
                }
                else
                {
                    // Report the type name for complex/unknown types
                    ValueStr = FString::Printf(TEXT("[RapidIter: %s]"), *InputType.GetName());
                    bFoundValue = true;
                }
            }
        }

        // If not found in rapid iteration params, check if it has a default value
        if (!bFoundValue)
        {
            // Check exposed pins for static switch values
            for (UEdGraphPin* Pin : ModuleNode->Pins)
            {
                if (Pin->Direction == EGPD_Input && Pin->PinName.ToString().Contains(SimpleName, ESearchCase::IgnoreCase))
                {
                    if (!Pin->DefaultValue.IsEmpty())
                    {
                        ValueStr = Pin->DefaultValue;
                        bFoundValue = true;
                    }
                    else if (Pin->LinkedTo.Num() > 0)
                    {
                        ValueStr = TEXT("[Linked]");
                        bFoundValue = true;
                    }
                    break;
                }
            }
        }

        if (!bFoundValue)
        {
            ValueStr = TEXT("[Default/Unset]");
        }

        InputObj->SetStringField(TEXT("value"), ValueStr);
        InputsArray.Add(MakeShared<FJsonValueObject>(InputObj));
    }

    // Also add any exposed static switch pins that weren't in ModuleInputs
    for (UEdGraphPin* Pin : ModuleNode->Pins)
    {
        if (Pin->Direction == EGPD_Input && !Pin->bHidden && Pin->PinName != TEXT("InputMap"))
        {
            // Check if we already have this input
            FString PinName = Pin->PinName.ToString();
            bool bAlreadyAdded = false;
            for (const TSharedPtr<FJsonValue>& ExistingValue : InputsArray)
            {
                TSharedPtr<FJsonObject> ExistingObj = ExistingValue->AsObject();
                if (ExistingObj && ExistingObj->GetStringField(TEXT("name")).Contains(PinName, ESearchCase::IgnoreCase))
                {
                    bAlreadyAdded = true;
                    break;
                }
            }

            if (!bAlreadyAdded)
            {
                TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();
                InputObj->SetStringField(TEXT("name"), PinName);
                InputObj->SetStringField(TEXT("full_name"), PinName);
                InputObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());

                FString Value = Pin->DefaultValue;
                if (Value.IsEmpty() && Pin->LinkedTo.Num() > 0)
                {
                    Value = TEXT("[Linked]");
                }
                else if (Value.IsEmpty())
                {
                    Value = TEXT("[Default]");
                }
                InputObj->SetStringField(TEXT("value"), Value);
                InputsArray.Add(MakeShared<FJsonValueObject>(InputObj));
            }
        }
    }

    OutInputs->SetArrayField(TEXT("inputs"), InputsArray);
    OutInputs->SetNumberField(TEXT("input_count"), InputsArray.Num());

    return true;
}
