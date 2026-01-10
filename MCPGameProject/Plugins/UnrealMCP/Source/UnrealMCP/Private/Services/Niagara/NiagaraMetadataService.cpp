// NiagaraMetadataService.cpp - Metadata queries
// GetMetadata, GetModuleInputs, GetEmitterModules

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraScriptSource.h"
#include "NiagaraTypes.h"
#include "NiagaraCommon.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
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

        // If not found in rapid iteration params, check module pins directly
        FString ValueMode = TEXT("Local");
        if (!bFoundValue)
        {
            // Search for matching input pin on the module node
            for (UEdGraphPin* Pin : ModuleNode->Pins)
            {
                if (Pin->Direction != EGPD_Input)
                {
                    continue;
                }

                // Match pin by name (handle both "Module.X" and "X" formats)
                FString PinNameStr = Pin->PinName.ToString();
                if (!PinNameStr.Contains(SimpleName, ESearchCase::IgnoreCase) &&
                    !SimpleName.Contains(PinNameStr, ESearchCase::IgnoreCase))
                {
                    continue;
                }

                if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0] && Pin->LinkedTo[0]->GetOwningNode())
                {
                    // Pin is linked - determine the value mode from the linked node
                    UEdGraphNode* LinkedNode = Pin->LinkedTo[0]->GetOwningNode();
                    FString LinkedClassName = LinkedNode->GetClass()->GetName();

                    if (UNiagaraNodeFunctionCall* DynamicNode = Cast<UNiagaraNodeFunctionCall>(LinkedNode))
                    {
                        // Dynamic input (curve, random range, etc.)
                        ValueMode = TEXT("Dynamic");
                        ValueStr = FString::Printf(TEXT("[Dynamic: %s]"), *DynamicNode->GetFunctionName());
                        bFoundValue = true;
                    }
                    else if (LinkedClassName.Contains(TEXT("ParameterMapGet")))
                    {
                        // Linked to another parameter
                        ValueMode = TEXT("Linked");
                        FNiagaraVariable LinkedVar = UEdGraphSchema_Niagara::PinToNiagaraVariable(Pin->LinkedTo[0]);
                        ValueStr = FString::Printf(TEXT("[Linked: %s]"), *LinkedVar.GetName().ToString());
                        bFoundValue = true;
                    }
                    else if (LinkedClassName.Contains(TEXT("CustomHlsl")))
                    {
                        // Custom expression
                        ValueMode = TEXT("Expression");
                        ValueStr = TEXT("[Expression]");
                        bFoundValue = true;
                    }
                    else if (LinkedClassName.Contains(TEXT("NiagaraNodeInput")))
                    {
                        // Data interface or object asset
                        ValueMode = TEXT("DataInterface");
                        ValueStr = FString::Printf(TEXT("[%s]"), *LinkedClassName);
                        bFoundValue = true;
                    }
                    else
                    {
                        ValueMode = TEXT("Linked");
                        ValueStr = FString::Printf(TEXT("[Linked: %s]"), *LinkedClassName);
                        bFoundValue = true;
                    }
                }
                else if (!Pin->DefaultValue.IsEmpty())
                {
                    // Local value - try to read properly using PinToNiagaraVariable
                    const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
                    FNiagaraVariable ValueVariable = NiagaraSchema->PinToNiagaraVariable(Pin, false);

                    if (ValueVariable.IsDataAllocated())
                    {
                        // Use the actual variable type, not InputType (they may differ)
                        FNiagaraTypeDefinition ActualType = ValueVariable.GetType();

                        // Read the value from the variable using its actual type
                        if (ActualType == FNiagaraTypeDefinition::GetFloatDef())
                        {
                            float FloatValue = ValueVariable.GetValue<float>();
                            ValueStr = FString::Printf(TEXT("%.4f"), FloatValue);
                            bFoundValue = true;
                        }
                        else if (ActualType == FNiagaraTypeDefinition::GetIntDef())
                        {
                            int32 IntValue = ValueVariable.GetValue<int32>();
                            ValueStr = FString::Printf(TEXT("%d"), IntValue);
                            bFoundValue = true;
                        }
                        else if (ActualType == FNiagaraTypeDefinition::GetBoolDef())
                        {
                            FNiagaraBool BoolValue = ValueVariable.GetValue<FNiagaraBool>();
                            ValueStr = BoolValue.IsValid() && BoolValue.GetValue() ? TEXT("true") : TEXT("false");
                            bFoundValue = true;
                        }
                        else if (ActualType == FNiagaraTypeDefinition::GetVec2Def())
                        {
                            FVector2f Vec = ValueVariable.GetValue<FVector2f>();
                            ValueStr = FString::Printf(TEXT("(%.4f, %.4f)"), Vec.X, Vec.Y);
                            bFoundValue = true;
                        }
                        else if (ActualType == FNiagaraTypeDefinition::GetVec3Def())
                        {
                            FVector3f Vec = ValueVariable.GetValue<FVector3f>();
                            ValueStr = FString::Printf(TEXT("(%.4f, %.4f, %.4f)"), Vec.X, Vec.Y, Vec.Z);
                            bFoundValue = true;
                        }
                        else if (ActualType == FNiagaraTypeDefinition::GetColorDef())
                        {
                            FLinearColor Color = ValueVariable.GetValue<FLinearColor>();
                            ValueStr = FString::Printf(TEXT("(R=%.4f, G=%.4f, B=%.4f, A=%.4f)"), Color.R, Color.G, Color.B, Color.A);
                            bFoundValue = true;
                        }
                    }

                    if (!bFoundValue)
                    {
                        // Fall back to pin default value string, resolve enum names
                        ValueStr = Pin->DefaultValue;

                        // Try to resolve enum display names
                        if (UEnum* EnumType = Cast<UEnum>(Pin->PinType.PinSubCategoryObject.Get()))
                        {
                            int64 EnumValue = EnumType->GetValueByNameString(ValueStr);
                            if (EnumValue != INDEX_NONE)
                            {
                                FText DisplayNameText = EnumType->GetDisplayNameTextByValue(EnumValue);
                                if (!DisplayNameText.IsEmpty())
                                {
                                    ValueStr = DisplayNameText.ToString();
                                }
                            }
                        }
                        bFoundValue = true;
                    }
                }

                if (bFoundValue)
                {
                    break;
                }
            }
        }

        if (!bFoundValue)
        {
            ValueStr = TEXT("[Default/Unset]");
        }

        InputObj->SetStringField(TEXT("value"), ValueStr);
        InputObj->SetStringField(TEXT("value_mode"), ValueMode);
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
                FString PinValueMode = TEXT("Local");

                if (Value.IsEmpty() && Pin->LinkedTo.Num() > 0)
                {
                    PinValueMode = TEXT("Linked");
                    // Try to determine what it's linked to
                    if (Pin->LinkedTo[0] && Pin->LinkedTo[0]->GetOwningNode())
                    {
                        UEdGraphNode* LinkedNode = Pin->LinkedTo[0]->GetOwningNode();
                        if (UNiagaraNodeFunctionCall* DynamicNode = Cast<UNiagaraNodeFunctionCall>(LinkedNode))
                        {
                            PinValueMode = TEXT("Dynamic");
                            Value = FString::Printf(TEXT("[Dynamic: %s]"), *DynamicNode->GetFunctionName());
                        }
                        else
                        {
                            Value = TEXT("[Linked]");
                        }
                    }
                    else
                    {
                        Value = TEXT("[Linked]");
                    }
                }
                else if (Value.IsEmpty())
                {
                    Value = TEXT("[Default]");
                }

                // Resolve enum display names for better readability
                // Enum pins have PinSubCategoryObject set to the UEnum*
                if (UEnum* EnumType = Cast<UEnum>(Pin->PinType.PinSubCategoryObject.Get()))
                {
                    // Try to find the enum value by name and get its display name
                    int64 EnumValue = EnumType->GetValueByNameString(Value);
                    if (EnumValue != INDEX_NONE)
                    {
                        FText DisplayNameText = EnumType->GetDisplayNameTextByValue(EnumValue);
                        if (!DisplayNameText.IsEmpty())
                        {
                            Value = DisplayNameText.ToString();
                        }
                    }
                }

                InputObj->SetStringField(TEXT("value"), Value);
                InputObj->SetStringField(TEXT("value_mode"), PinValueMode);
                InputsArray.Add(MakeShared<FJsonValueObject>(InputObj));
            }
        }
    }

    OutInputs->SetArrayField(TEXT("inputs"), InputsArray);
    OutInputs->SetNumberField(TEXT("input_count"), InputsArray.Num());

    return true;
}

// Helper function to get parameter map input pin (same as in NiagaraServiceHelpers.cpp)
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
    int32 TotalModules = SpawnModules.Num() + UpdateModules.Num() + EventModulesArray.Num();
    OutModules->SetNumberField(TEXT("total_module_count"), TotalModules);
    OutModules->SetNumberField(TEXT("spawn_count"), SpawnModules.Num());
    OutModules->SetNumberField(TEXT("update_count"), UpdateModules.Num());
    if (EventModulesArray.Num() > 0)
    {
        OutModules->SetNumberField(TEXT("event_count"), EventModulesArray.Num());
    }

    return true;
}
