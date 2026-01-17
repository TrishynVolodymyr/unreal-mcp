// NiagaraMetadataService.cpp - Metadata queries
// GetMetadata, GetModuleInputs, GetEmitterModules

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeStaticSwitch.h"
#include "NiagaraNodeParameterMapSet.h"
#include "NiagaraNodeParameterMapGet.h"
#include "NiagaraScriptVariable.h"
#include "NiagaraScriptSource.h"
#include "NiagaraTypes.h"
#include "NiagaraCommon.h"
#include "NiagaraDataInterfaceCurve.h"
#include "NiagaraDataInterfaceColorCurve.h"
#include "Curves/RichCurve.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "EdGraphSchema_Niagara.h"

// Helper to find UNiagaraNodeStaticSwitch in module graph by parameter name
static UNiagaraNodeStaticSwitch* FindStaticSwitchNodeByName(UNiagaraGraph* Graph, const FName& ParameterName)
{
    if (!Graph)
    {
        return nullptr;
    }

    TArray<UNiagaraNodeStaticSwitch*> StaticSwitchNodes;
    Graph->GetNodesOfClass<UNiagaraNodeStaticSwitch>(StaticSwitchNodes);

    for (UNiagaraNodeStaticSwitch* SwitchNode : StaticSwitchNodes)
    {
        if (SwitchNode && SwitchNode->InputParameterName == ParameterName)
        {
            return SwitchNode;
        }
    }

    return nullptr;
}

// Helper to add enum options to JSON object from a static switch node
static void AddStaticSwitchEnumOptions(TSharedPtr<FJsonObject>& InputObj, UNiagaraNodeStaticSwitch* SwitchNode, const FString& CurrentValue)
{
    if (!SwitchNode)
    {
        return;
    }

    // Access SwitchTypeData - it's a public member
    const FStaticSwitchTypeData& SwitchTypeData = SwitchNode->SwitchTypeData;

    if (SwitchTypeData.SwitchType == ENiagaraStaticSwitchType::Enum && SwitchTypeData.Enum)
    {
        UEnum* EnumType = SwitchTypeData.Enum;

        // Store raw value
        InputObj->SetStringField(TEXT("raw_value"), CurrentValue);

        // Try to resolve display name from numeric value
        if (CurrentValue.IsNumeric())
        {
            int32 Index = FCString::Atoi(*CurrentValue);
            if (Index >= 0 && Index < EnumType->NumEnums() - 1)
            {
                FText DisplayNameText = EnumType->GetDisplayNameTextByIndex(Index);
                if (!DisplayNameText.IsEmpty())
                {
                    InputObj->SetStringField(TEXT("value"), DisplayNameText.ToString());
                }
            }
        }

        // Add available options
        TArray<TSharedPtr<FJsonValue>> OptionsArray;
        for (int32 i = 0; i < EnumType->NumEnums() - 1; ++i) // -1 to skip MAX
        {
            TSharedPtr<FJsonObject> OptionObj = MakeShared<FJsonObject>();
            OptionObj->SetNumberField(TEXT("index"), i);

            FText DisplayText = EnumType->GetDisplayNameTextByIndex(i);
            OptionObj->SetStringField(TEXT("display_name"), DisplayText.ToString());

            FString InternalName = EnumType->GetNameStringByIndex(i);
            OptionObj->SetStringField(TEXT("internal_name"), InternalName);

            OptionsArray.Add(MakeShared<FJsonValueObject>(OptionObj));
        }
        InputObj->SetArrayField(TEXT("options"), OptionsArray);
        InputObj->SetStringField(TEXT("input_type"), TEXT("enum"));
    }
    else if (SwitchTypeData.SwitchType == ENiagaraStaticSwitchType::Bool)
    {
        InputObj->SetStringField(TEXT("input_type"), TEXT("bool"));
    }
    else if (SwitchTypeData.SwitchType == ENiagaraStaticSwitchType::Integer)
    {
        InputObj->SetStringField(TEXT("input_type"), TEXT("integer"));

        // For integer switches, try to get custom display names from ScriptVariable metadata
        // Uses the exported GetScriptVariable(FName) overload from NiagaraGraph.h
        UNiagaraGraph* ModuleGraph = SwitchNode->GetNiagaraGraph();
        UNiagaraScriptVariable* ScriptVar = nullptr;
        bool bHasEnumStyleNames = false;

        if (ModuleGraph && !ModuleGraph->IsCompilationCopy())
        {
            // Use the simpler overload that takes just the parameter name (NIAGARAEDITOR_API exported)
            ScriptVar = ModuleGraph->GetScriptVariable(SwitchNode->InputParameterName);
            if (ScriptVar && ScriptVar->Metadata.WidgetCustomization.WidgetType == ENiagaraInputWidgetType::EnumStyle)
            {
                bHasEnumStyleNames = ScriptVar->Metadata.WidgetCustomization.EnumStyleDropdownValues.Num() > 0;
            }
        }

        // Get option values
        TArray<int32> OptionValues = SwitchNode->GetOptionValues();
        if (OptionValues.Num() > 0)
        {
            TArray<TSharedPtr<FJsonValue>> OptionsArray;
            for (int32 i = 0; i < OptionValues.Num(); ++i)
            {
                TSharedPtr<FJsonObject> OptionObj = MakeShared<FJsonObject>();
                OptionObj->SetNumberField(TEXT("index"), i);
                OptionObj->SetNumberField(TEXT("value"), OptionValues[i]);

                // Try to get custom display name from EnumStyleDropdownValues
                FString DisplayName;
                if (bHasEnumStyleNames && ScriptVar->Metadata.WidgetCustomization.EnumStyleDropdownValues.IsValidIndex(i))
                {
                    DisplayName = ScriptVar->Metadata.WidgetCustomization.EnumStyleDropdownValues[i].DisplayName.ToString();
                }

                // Fallback to "Case N" if no custom name
                if (DisplayName.IsEmpty())
                {
                    DisplayName = FString::Printf(TEXT("Case %d"), OptionValues[i]);
                }
                OptionObj->SetStringField(TEXT("display_name"), DisplayName);

                OptionsArray.Add(MakeShared<FJsonValueObject>(OptionObj));
            }
            InputObj->SetArrayField(TEXT("options"), OptionsArray);

            // Also resolve current value display name
            if (bHasEnumStyleNames && CurrentValue.IsNumeric())
            {
                int32 CurrentIndex = FCString::Atoi(*CurrentValue);
                if (ScriptVar->Metadata.WidgetCustomization.EnumStyleDropdownValues.IsValidIndex(CurrentIndex))
                {
                    FString ResolvedName = ScriptVar->Metadata.WidgetCustomization.EnumStyleDropdownValues[CurrentIndex].DisplayName.ToString();
                    if (!ResolvedName.IsEmpty())
                    {
                        InputObj->SetStringField(TEXT("raw_value"), CurrentValue);
                        InputObj->SetStringField(TEXT("value"), ResolvedName);
                    }
                }
            }
        }
    }
}

// Helper to extract keyframes from a FRichCurve to JSON array
static void ExtractCurveKeyframes(const FRichCurve& Curve, TArray<TSharedPtr<FJsonValue>>& OutKeyframes)
{
    const TArray<FRichCurveKey>& Keys = Curve.GetConstRefOfKeys();
    for (const FRichCurveKey& Key : Keys)
    {
        TSharedPtr<FJsonObject> KeyObj = MakeShared<FJsonObject>();
        KeyObj->SetNumberField(TEXT("time"), Key.Time);
        KeyObj->SetNumberField(TEXT("value"), Key.Value);
        OutKeyframes.Add(MakeShared<FJsonValueObject>(KeyObj));
    }
}

// Helper to find curve DataInterface from a dynamic input function node
static UNiagaraDataInterface* FindCurveDataInterfaceFromDynamicNode(UNiagaraNodeFunctionCall* DynamicNode)
{
    if (!DynamicNode)
    {
        return nullptr;
    }

    // Look through the dynamic node's input pins for a DataInterface connection
    for (UEdGraphPin* Pin : DynamicNode->Pins)
    {
        if (Pin->Direction != EGPD_Input)
        {
            continue;
        }

        // Check if this pin is connected to a NiagaraNodeInput
        if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0])
        {
            UEdGraphNode* ConnectedNode = Pin->LinkedTo[0]->GetOwningNode();
            if (UNiagaraNodeInput* InputNode = Cast<UNiagaraNodeInput>(ConnectedNode))
            {
                // Check if this is a DataInterface
                if (InputNode->Input.IsDataInterface())
                {
                    // Access the private DataInterface UPROPERTY via reflection
                    FProperty* DIProperty = UNiagaraNodeInput::StaticClass()->FindPropertyByName(TEXT("DataInterface"));
                    if (DIProperty)
                    {
                        FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(DIProperty);
                        if (ObjProp)
                        {
                            UNiagaraDataInterface* DI = Cast<UNiagaraDataInterface>(ObjProp->GetObjectPropertyValue_InContainer(InputNode));
                            // Check if it's a curve type
                            if (DI && (DI->IsA<UNiagaraDataInterfaceCurve>() || DI->IsA<UNiagaraDataInterfaceColorCurve>()))
                            {
                                return DI;
                            }
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

// Helper to read a value from a pin (either from DefaultValue or linked ParameterMapGet)
static FString ReadPinValue(UEdGraphPin* Pin, const FNiagaraTypeDefinition& ExpectedType)
{
    if (!Pin)
    {
        return TEXT("");
    }

    // Check if pin has a default value
    if (!Pin->DefaultValue.IsEmpty())
    {
        const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
        FNiagaraVariable ValueVariable = NiagaraSchema->PinToNiagaraVariable(Pin, false);

        if (ValueVariable.IsDataAllocated())
        {
            FNiagaraTypeDefinition ActualType = ValueVariable.GetType();

            if (ActualType == FNiagaraTypeDefinition::GetFloatDef())
            {
                return FString::Printf(TEXT("%.4f"), ValueVariable.GetValue<float>());
            }
            else if (ActualType == FNiagaraTypeDefinition::GetIntDef())
            {
                return FString::Printf(TEXT("%d"), ValueVariable.GetValue<int32>());
            }
            else if (ActualType == FNiagaraTypeDefinition::GetVec2Def())
            {
                FVector2f Vec = ValueVariable.GetValue<FVector2f>();
                return FString::Printf(TEXT("(%.4f, %.4f)"), Vec.X, Vec.Y);
            }
            else if (ActualType == FNiagaraTypeDefinition::GetVec3Def())
            {
                FVector3f Vec = ValueVariable.GetValue<FVector3f>();
                return FString::Printf(TEXT("(%.4f, %.4f, %.4f)"), Vec.X, Vec.Y, Vec.Z);
            }
            else if (ActualType == FNiagaraTypeDefinition::GetVec4Def())
            {
                FVector4f Vec = ValueVariable.GetValue<FVector4f>();
                return FString::Printf(TEXT("(%.4f, %.4f, %.4f, %.4f)"), Vec.X, Vec.Y, Vec.Z, Vec.W);
            }
            else if (ActualType == FNiagaraTypeDefinition::GetColorDef())
            {
                FLinearColor Color = ValueVariable.GetValue<FLinearColor>();
                return FString::Printf(TEXT("(R=%.4f, G=%.4f, B=%.4f, A=%.4f)"), Color.R, Color.G, Color.B, Color.A);
            }
        }
        // Fall back to raw pin value
        return Pin->DefaultValue;
    }

    // Check if linked to a parameter map get node
    if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0])
    {
        UEdGraphNode* LinkedNode = Pin->LinkedTo[0]->GetOwningNode();
        if (UNiagaraNodeParameterMapGet* GetNode = Cast<UNiagaraNodeParameterMapGet>(LinkedNode))
        {
            // Get the variable being linked from
            FNiagaraVariable LinkedVar = UEdGraphSchema_Niagara::PinToNiagaraVariable(Pin->LinkedTo[0]);
            return FString::Printf(TEXT("[Linked: %s]"), *LinkedVar.GetName().ToString());
        }
    }

    return TEXT("");
}

// Helper to extract value from FNiagaraVariable to string
static FString NiagaraVariableToString(const FNiagaraVariable& Variable)
{
    if (!Variable.IsDataAllocated())
    {
        return TEXT("");
    }

    FNiagaraTypeDefinition Type = Variable.GetType();

    if (Type == FNiagaraTypeDefinition::GetFloatDef())
    {
        return FString::Printf(TEXT("%.4f"), Variable.GetValue<float>());
    }
    else if (Type == FNiagaraTypeDefinition::GetIntDef())
    {
        return FString::Printf(TEXT("%d"), Variable.GetValue<int32>());
    }
    else if (Type == FNiagaraTypeDefinition::GetVec2Def())
    {
        FVector2f Vec = Variable.GetValue<FVector2f>();
        return FString::Printf(TEXT("(%.4f, %.4f)"), Vec.X, Vec.Y);
    }
    else if (Type == FNiagaraTypeDefinition::GetVec3Def())
    {
        FVector3f Vec = Variable.GetValue<FVector3f>();
        return FString::Printf(TEXT("(%.4f, %.4f, %.4f)"), Vec.X, Vec.Y, Vec.Z);
    }
    else if (Type == FNiagaraTypeDefinition::GetVec4Def())
    {
        FVector4f Vec = Variable.GetValue<FVector4f>();
        return FString::Printf(TEXT("(%.4f, %.4f, %.4f, %.4f)"), Vec.X, Vec.Y, Vec.Z, Vec.W);
    }
    else if (Type == FNiagaraTypeDefinition::GetColorDef())
    {
        FLinearColor Color = Variable.GetValue<FLinearColor>();
        return FString::Printf(TEXT("(R=%.4f, G=%.4f, B=%.4f, A=%.4f)"), Color.R, Color.G, Color.B, Color.A);
    }

    return TEXT("");
}

// Helper function to get ParameterMap pin from a collection of pins
// Replicates FNiagaraStackGraphUtilities::GetParameterMapPin logic (not exported)
static UEdGraphPin* GetParameterMapPinFromArray_Meta(const TArray<UEdGraphPin*>& Pins)
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

// Helper to get ParameterMapInput pin (replicates unexported FNiagaraStackGraphUtilities::GetParameterMapInputPin)
static UEdGraphPin* GetParameterMapInputPinLocal_Meta(UNiagaraNode& Node)
{
    TArray<UEdGraphPin*> InputPins;
    Node.GetInputPins(InputPins);
    return GetParameterMapPinFromArray_Meta(InputPins);
}

// Helper to find the ParameterMapSet node connected to a function call node
// Replicates FNiagaraStackGraphUtilities::GetStackFunctionOverrideNode logic (not exported)
static UNiagaraNodeParameterMapSet* GetOverrideNodeForFunctionCall(UNiagaraNodeFunctionCall& FunctionCallNode)
{
    UEdGraphPin* ParameterMapInput = GetParameterMapInputPinLocal_Meta(FunctionCallNode);
    if (ParameterMapInput != nullptr && ParameterMapInput->LinkedTo.Num() == 1)
    {
        return Cast<UNiagaraNodeParameterMapSet>(ParameterMapInput->LinkedTo[0]->GetOwningNode());
    }
    return nullptr;
}

// Helper to extract min/max values from UniformRanged dynamic input nodes
// Optional parameters for RapidIterationParameters lookup:
// - Script: The Niagara script containing the parameter
// - UniqueEmitterName: The emitter's unique name
// - ScriptUsage: The script usage type
static void ExtractUniformRangedValues(
    UNiagaraNodeFunctionCall* DynamicInputNode,
    UNiagaraSystem* System,
    TSharedPtr<FJsonObject>& InputObj,
    UNiagaraScript* Script = nullptr,
    const FString& UniqueEmitterName = TEXT(""),
    ENiagaraScriptUsage ScriptUsage = ENiagaraScriptUsage::ParticleSpawnScript)
{
    if (!DynamicInputNode || !System)
    {
        return;
    }

    // Check if this is a UniformRanged dynamic input
    FString FunctionName = DynamicInputNode->GetFunctionName();
    if (!FunctionName.Contains(TEXT("UniformRanged"), ESearchCase::IgnoreCase))
    {
        return;
    }

    // Add the random type info
    InputObj->SetStringField(TEXT("random_type"), FunctionName);

    FString MinValueStr, MaxValueStr;

    // Determine the value type based on the function name
    FNiagaraTypeDefinition ValueType;
    if (FunctionName.Contains(TEXT("Float")))
    {
        ValueType = FNiagaraTypeDefinition::GetFloatDef();
    }
    else if (FunctionName.Contains(TEXT("Vector2D")) || FunctionName.Contains(TEXT("Vec2")))
    {
        ValueType = FNiagaraTypeDefinition::GetVec2Def();
    }
    else if (FunctionName.Contains(TEXT("Vector")) || FunctionName.Contains(TEXT("Vec3")))
    {
        ValueType = FNiagaraTypeDefinition::GetVec3Def();
    }
    else if (FunctionName.Contains(TEXT("Color")) || FunctionName.Contains(TEXT("LinearColor")))
    {
        ValueType = FNiagaraTypeDefinition::GetColorDef();
    }
    else
    {
        ValueType = FNiagaraTypeDefinition::GetFloatDef(); // Default to float
    }

    // APPROACH 1: Check DynamicInputNode's direct input pins for Minimum/Maximum values
    // Dynamic inputs have input pins for their parameters (Minimum, Maximum)
    for (UEdGraphPin* Pin : DynamicInputNode->Pins)
    {
        if (Pin->Direction != EGPD_Input)
        {
            continue;
        }

        FString PinName = Pin->PinName.ToString();
        bool bIsMin = PinName.Contains(TEXT("Minimum"), ESearchCase::IgnoreCase) ||
                      PinName.Equals(TEXT("Min"), ESearchCase::IgnoreCase);
        bool bIsMax = PinName.Contains(TEXT("Maximum"), ESearchCase::IgnoreCase) ||
                      PinName.Equals(TEXT("Max"), ESearchCase::IgnoreCase);

        if (!bIsMin && !bIsMax)
        {
            continue;
        }

        FString ValueStr;

        // First check DefaultValue on the pin itself
        if (!Pin->DefaultValue.IsEmpty())
        {
            ValueStr = Pin->DefaultValue;
        }
        // If no DefaultValue, check if pin is linked to another node
        else if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0])
        {
            UEdGraphPin* LinkedPin = Pin->LinkedTo[0];
            // If linked to a ParameterMapGet output, try to get the value from there
            if (!LinkedPin->DefaultValue.IsEmpty())
            {
                ValueStr = LinkedPin->DefaultValue;
            }
        }

        if (!ValueStr.IsEmpty())
        {
            if (bIsMin && MinValueStr.IsEmpty())
            {
                MinValueStr = ValueStr;
            }
            else if (bIsMax && MaxValueStr.IsEmpty())
            {
                MaxValueStr = ValueStr;
            }
        }
    }

    // APPROACH 2: Search ALL ParameterMapSet nodes in the graph for override pins
    // Override pins for dynamic input parameters are named like "Module.<DynamicInputName>.<ParamName>"
    // e.g., "Module.UniformRangedFloat003.Minimum"
    if (MinValueStr.IsEmpty() || MaxValueStr.IsEmpty())
    {
        UNiagaraGraph* Graph = Cast<UNiagaraGraph>(DynamicInputNode->GetGraph());
        if (Graph)
        {
            TArray<UNiagaraNodeParameterMapSet*> MapSetNodes;
            Graph->GetNodesOfClass<UNiagaraNodeParameterMapSet>(MapSetNodes);

            for (UNiagaraNodeParameterMapSet* MapSetNode : MapSetNodes)
            {
                for (UEdGraphPin* Pin : MapSetNode->Pins)
                {
                    if (Pin->Direction != EGPD_Input)
                    {
                        continue;
                    }

                    FString PinName = Pin->PinName.ToString();

                    // Check if this pin belongs to our dynamic input
                    // Pin names are like "Module.UniformRangedFloat003.Minimum"
                    if (!PinName.Contains(FunctionName, ESearchCase::IgnoreCase))
                    {
                        continue;
                    }

                    bool bIsMin = PinName.Contains(TEXT("Minimum"), ESearchCase::IgnoreCase);
                    bool bIsMax = PinName.Contains(TEXT("Maximum"), ESearchCase::IgnoreCase);

                    if (!bIsMin && !bIsMax)
                    {
                        continue;
                    }

                    // Get value from pin's DefaultValue
                    if (!Pin->DefaultValue.IsEmpty())
                    {
                        if (bIsMin && MinValueStr.IsEmpty())
                        {
                            MinValueStr = Pin->DefaultValue;
                        }
                        else if (bIsMax && MaxValueStr.IsEmpty())
                        {
                            MaxValueStr = Pin->DefaultValue;
                        }
                    }
                }

                // Stop searching if we found both values
                if (!MinValueStr.IsEmpty() && !MaxValueStr.IsEmpty())
                {
                    break;
                }
            }
        }
    }

    // APPROACH 3: Try to read from RapidIterationParameters if Script info is provided
    // Dynamic input parameters can be stored as rapid iteration constants
    if ((MinValueStr.IsEmpty() || MaxValueStr.IsEmpty()) && Script && !UniqueEmitterName.IsEmpty())
    {
        // Try to read Minimum value
        if (MinValueStr.IsEmpty())
        {
            FNiagaraParameterHandle MinHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
                FName(TEXT("Minimum")),
                FName(*FunctionName)
            );

            FNiagaraVariable MinVar(ValueType, FName(*MinHandle.GetParameterHandleString().ToString()));
            FNiagaraVariable RapidIterMin = FNiagaraUtilities::ConvertVariableToRapidIterationConstantName(
                MinVar,
                *UniqueEmitterName,
                ScriptUsage
            );

            const uint8* MinData = Script->RapidIterationParameters.GetParameterData(RapidIterMin);
            if (MinData)
            {
                FNiagaraVariable TempVar(ValueType, RapidIterMin.GetName());
                TempVar.SetData(MinData);
                const uint8* SafeData = TempVar.GetData();
                if (SafeData)
                {
                    if (ValueType == FNiagaraTypeDefinition::GetFloatDef())
                    {
                        float Val = TempVar.GetValue<float>();
                        MinValueStr = FString::Printf(TEXT("%.4f"), Val);
                    }
                    else if (ValueType == FNiagaraTypeDefinition::GetVec2Def())
                    {
                        FVector2f Vec = TempVar.GetValue<FVector2f>();
                        MinValueStr = FString::Printf(TEXT("(%.4f, %.4f)"), Vec.X, Vec.Y);
                    }
                    else if (ValueType == FNiagaraTypeDefinition::GetVec3Def())
                    {
                        FVector3f Vec = TempVar.GetValue<FVector3f>();
                        MinValueStr = FString::Printf(TEXT("(%.4f, %.4f, %.4f)"), Vec.X, Vec.Y, Vec.Z);
                    }
                    else if (ValueType == FNiagaraTypeDefinition::GetColorDef())
                    {
                        FLinearColor Color = TempVar.GetValue<FLinearColor>();
                        MinValueStr = FString::Printf(TEXT("(R=%.4f, G=%.4f, B=%.4f, A=%.4f)"), Color.R, Color.G, Color.B, Color.A);
                    }
                }
            }
        }

        // Try to read Maximum value
        if (MaxValueStr.IsEmpty())
        {
            FNiagaraParameterHandle MaxHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
                FName(TEXT("Maximum")),
                FName(*FunctionName)
            );

            FNiagaraVariable MaxVar(ValueType, FName(*MaxHandle.GetParameterHandleString().ToString()));
            FNiagaraVariable RapidIterMax = FNiagaraUtilities::ConvertVariableToRapidIterationConstantName(
                MaxVar,
                *UniqueEmitterName,
                ScriptUsage
            );

            const uint8* MaxData = Script->RapidIterationParameters.GetParameterData(RapidIterMax);
            if (MaxData)
            {
                FNiagaraVariable TempVar(ValueType, RapidIterMax.GetName());
                TempVar.SetData(MaxData);
                const uint8* SafeData = TempVar.GetData();
                if (SafeData)
                {
                    if (ValueType == FNiagaraTypeDefinition::GetFloatDef())
                    {
                        float Val = TempVar.GetValue<float>();
                        MaxValueStr = FString::Printf(TEXT("%.4f"), Val);
                    }
                    else if (ValueType == FNiagaraTypeDefinition::GetVec2Def())
                    {
                        FVector2f Vec = TempVar.GetValue<FVector2f>();
                        MaxValueStr = FString::Printf(TEXT("(%.4f, %.4f)"), Vec.X, Vec.Y);
                    }
                    else if (ValueType == FNiagaraTypeDefinition::GetVec3Def())
                    {
                        FVector3f Vec = TempVar.GetValue<FVector3f>();
                        MaxValueStr = FString::Printf(TEXT("(%.4f, %.4f, %.4f)"), Vec.X, Vec.Y, Vec.Z);
                    }
                    else if (ValueType == FNiagaraTypeDefinition::GetColorDef())
                    {
                        FLinearColor Color = TempVar.GetValue<FLinearColor>();
                        MaxValueStr = FString::Printf(TEXT("(R=%.4f, G=%.4f, B=%.4f, A=%.4f)"), Color.R, Color.G, Color.B, Color.A);
                    }
                }
            }
        }
    }

    // APPROACH 4: Read default values from the dynamic input script's graph
    // When a dynamic input hasn't been modified, its defaults are stored in the script asset's graph
    // as UNiagaraScriptVariable objects with DefaultValueVariant containing the actual values
    if (MinValueStr.IsEmpty() || MaxValueStr.IsEmpty())
    {
        UNiagaraScript* DynamicInputScript = DynamicInputNode->FunctionScript;
        if (DynamicInputScript)
        {
            UNiagaraScriptSource* DynamicScriptSource = Cast<UNiagaraScriptSource>(DynamicInputScript->GetLatestSource());
            if (DynamicScriptSource && DynamicScriptSource->NodeGraph)
            {
                UNiagaraGraph* DynamicGraph = DynamicScriptSource->NodeGraph;

                // Helper lambda to read default value from a script variable
                auto ReadScriptVariableDefault = [&ValueType](UNiagaraScriptVariable* ScriptVar) -> FString
                {
                    if (!ScriptVar)
                    {
                        return TEXT("");
                    }

                    // Check if the default mode is Value (not Binding or Custom)
                    if (ScriptVar->DefaultMode != ENiagaraDefaultMode::Value)
                    {
                        return TEXT("");
                    }

                    const uint8* DefaultData = ScriptVar->GetDefaultValueData();
                    if (!DefaultData)
                    {
                        return TEXT("");
                    }

                    // Create a temp variable to safely read the value
                    FNiagaraVariable TempVar(ValueType, ScriptVar->Variable.GetName());
                    TempVar.SetData(DefaultData);

                    if (ValueType == FNiagaraTypeDefinition::GetFloatDef())
                    {
                        float Val = TempVar.GetValue<float>();
                        return FString::Printf(TEXT("%.4f"), Val);
                    }
                    else if (ValueType == FNiagaraTypeDefinition::GetVec2Def())
                    {
                        FVector2f Vec = TempVar.GetValue<FVector2f>();
                        return FString::Printf(TEXT("(%.4f, %.4f)"), Vec.X, Vec.Y);
                    }
                    else if (ValueType == FNiagaraTypeDefinition::GetVec3Def())
                    {
                        FVector3f Vec = TempVar.GetValue<FVector3f>();
                        return FString::Printf(TEXT("(%.4f, %.4f, %.4f)"), Vec.X, Vec.Y, Vec.Z);
                    }
                    else if (ValueType == FNiagaraTypeDefinition::GetColorDef())
                    {
                        FLinearColor Color = TempVar.GetValue<FLinearColor>();
                        return FString::Printf(TEXT("(R=%.4f, G=%.4f, B=%.4f, A=%.4f)"), Color.R, Color.G, Color.B, Color.A);
                    }

                    return TEXT("");
                };

                // Try to find and read Minimum
                if (MinValueStr.IsEmpty())
                {
                    // Try different common naming patterns for Minimum parameter
                    UNiagaraScriptVariable* MinScriptVar = DynamicGraph->GetScriptVariable(FName(TEXT("Minimum")));
                    if (!MinScriptVar)
                    {
                        MinScriptVar = DynamicGraph->GetScriptVariable(FName(TEXT("Module.Minimum")));
                    }
                    if (MinScriptVar)
                    {
                        MinValueStr = ReadScriptVariableDefault(MinScriptVar);
                    }
                }

                // Try to find and read Maximum
                if (MaxValueStr.IsEmpty())
                {
                    // Try different common naming patterns for Maximum parameter
                    UNiagaraScriptVariable* MaxScriptVar = DynamicGraph->GetScriptVariable(FName(TEXT("Maximum")));
                    if (!MaxScriptVar)
                    {
                        MaxScriptVar = DynamicGraph->GetScriptVariable(FName(TEXT("Module.Maximum")));
                    }
                    if (MaxScriptVar)
                    {
                        MaxValueStr = ReadScriptVariableDefault(MaxScriptVar);
                    }
                }
            }
        }
    }

    // Add the extracted values to the JSON object
    if (!MinValueStr.IsEmpty())
    {
        InputObj->SetStringField(TEXT("random_min"), MinValueStr);
    }
    if (!MaxValueStr.IsEmpty())
    {
        InputObj->SetStringField(TEXT("random_max"), MaxValueStr);
    }
}

// Helper to add curve data from a DataInterface to a JSON object
static void AddCurveDataToJson(TSharedPtr<FJsonObject>& InputObj, UNiagaraDataInterface* DataInterface)
{
    if (!DataInterface)
    {
        return;
    }

    // Check for float curve
    if (UNiagaraDataInterfaceCurve* FloatCurveDI = Cast<UNiagaraDataInterfaceCurve>(DataInterface))
    {
        InputObj->SetStringField(TEXT("curve_type"), TEXT("Float"));
        TArray<TSharedPtr<FJsonValue>> Keyframes;
        ExtractCurveKeyframes(FloatCurveDI->Curve, Keyframes);
        if (Keyframes.Num() > 0)
        {
            InputObj->SetArrayField(TEXT("keyframes"), Keyframes);
        }
        return;
    }

    // Check for color curve
    if (UNiagaraDataInterfaceColorCurve* ColorCurveDI = Cast<UNiagaraDataInterfaceColorCurve>(DataInterface))
    {
        InputObj->SetStringField(TEXT("curve_type"), TEXT("Color"));

        TArray<TSharedPtr<FJsonValue>> ColorKeyframes;

        // Get all unique time values across all channels
        TSet<float> TimeValues;
        for (const FRichCurveKey& Key : ColorCurveDI->RedCurve.GetConstRefOfKeys()) TimeValues.Add(Key.Time);
        for (const FRichCurveKey& Key : ColorCurveDI->GreenCurve.GetConstRefOfKeys()) TimeValues.Add(Key.Time);
        for (const FRichCurveKey& Key : ColorCurveDI->BlueCurve.GetConstRefOfKeys()) TimeValues.Add(Key.Time);
        for (const FRichCurveKey& Key : ColorCurveDI->AlphaCurve.GetConstRefOfKeys()) TimeValues.Add(Key.Time);

        // Sort and create combined keyframes
        TArray<float> SortedTimes = TimeValues.Array();
        SortedTimes.Sort();

        for (float Time : SortedTimes)
        {
            TSharedPtr<FJsonObject> KeyObj = MakeShared<FJsonObject>();
            KeyObj->SetNumberField(TEXT("time"), Time);
            KeyObj->SetNumberField(TEXT("r"), ColorCurveDI->RedCurve.Eval(Time));
            KeyObj->SetNumberField(TEXT("g"), ColorCurveDI->GreenCurve.Eval(Time));
            KeyObj->SetNumberField(TEXT("b"), ColorCurveDI->BlueCurve.Eval(Time));
            KeyObj->SetNumberField(TEXT("a"), ColorCurveDI->AlphaCurve.Eval(Time));
            ColorKeyframes.Add(MakeShared<FJsonValueObject>(KeyObj));
        }

        if (ColorKeyframes.Num() > 0)
        {
            InputObj->SetArrayField(TEXT("keyframes"), ColorKeyframes);
        }
        return;
    }
}

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
    case ENiagaraScriptUsage::EmitterSpawnScript:
        Script = EmitterData->EmitterSpawnScriptProps.Script;
        break;
    case ENiagaraScriptUsage::EmitterUpdateScript:
        Script = EmitterData->EmitterUpdateScriptProps.Script;
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

    // Find the module node by name - prioritize exact matches
    UNiagaraNodeFunctionCall* ModuleNode = nullptr;
    UNiagaraNodeFunctionCall* PartialMatchNode = nullptr;
    FString NormalizedSearchName = ModuleName.Replace(TEXT(" "), TEXT(""));
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
            // Track partial match as fallback (only if search name contains node name, not vice versa)
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

        FString ValueStr = TEXT("[Unknown]");
        bool bFoundValue = false;
        UNiagaraDataInterface* FoundDataInterface = nullptr;  // For curve extraction
        FString ValueMode = TEXT("Local");

        // FIRST: Check if this input's pin is connected to something
        // This takes priority over RapidIterationParameters because connections override local values
        UEdGraphPin* MatchingPin = nullptr;
        for (UEdGraphPin* Pin : ModuleNode->Pins)
        {
            if (Pin->Direction != EGPD_Input)
            {
                continue;
            }

            // Match pin by name (handle both "Module.X" and "X" formats)
            FString PinNameStr = Pin->PinName.ToString();
            if (PinNameStr.Contains(SimpleName, ESearchCase::IgnoreCase) ||
                SimpleName.Contains(PinNameStr, ESearchCase::IgnoreCase))
            {
                MatchingPin = Pin;
                break;
            }
        }

        // Check if the pin is connected to something
        if (MatchingPin && MatchingPin->LinkedTo.Num() > 0 && MatchingPin->LinkedTo[0] && MatchingPin->LinkedTo[0]->GetOwningNode())
        {
            // Pin is linked - determine the value mode from the linked node
            UEdGraphNode* LinkedNode = MatchingPin->LinkedTo[0]->GetOwningNode();
            FString LinkedClassName = LinkedNode->GetClass()->GetName();

            if (UNiagaraNodeFunctionCall* DynamicNode = Cast<UNiagaraNodeFunctionCall>(LinkedNode))
            {
                // Dynamic input (curve, random range, etc.)
                ValueMode = TEXT("Dynamic");
                ValueStr = FString::Printf(TEXT("[Dynamic: %s]"), *DynamicNode->GetFunctionName());
                bFoundValue = true;

                // Extract min/max values from UniformRanged dynamic inputs
                ExtractUniformRangedValues(DynamicNode, System, InputObj, Script, UniqueEmitterName, ScriptUsage);

                // Check if this dynamic input has a curve DataInterface
                UNiagaraDataInterface* CurveDI = FindCurveDataInterfaceFromDynamicNode(DynamicNode);
                if (CurveDI)
                {
                    FoundDataInterface = CurveDI;
                }
            }
            else if (LinkedClassName.Contains(TEXT("ParameterMapGet")))
            {
                // Linked to another parameter
                ValueMode = TEXT("Linked");
                FNiagaraVariable LinkedVar = UEdGraphSchema_Niagara::PinToNiagaraVariable(MatchingPin->LinkedTo[0]);
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
                if (UNiagaraNodeInput* InputNode = Cast<UNiagaraNodeInput>(LinkedNode))
                {
                    // Use FNiagaraVariable::IsDataInterface() which is exported from Niagara runtime
                    if (InputNode->Input.IsDataInterface())
                    {
                        // Access the private DataInterface UPROPERTY via reflection
                        FProperty* DIProperty = UNiagaraNodeInput::StaticClass()->FindPropertyByName(TEXT("DataInterface"));
                        if (DIProperty)
                        {
                            FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(DIProperty);
                            if (ObjProp)
                            {
                                FoundDataInterface = Cast<UNiagaraDataInterface>(ObjProp->GetObjectPropertyValue_InContainer(InputNode));
                            }
                        }

                        if (FoundDataInterface)
                        {
                            ValueStr = FString::Printf(TEXT("[DataInterface: %s]"), *FoundDataInterface->GetClass()->GetName());
                        }
                        else
                        {
                            ValueStr = TEXT("[DataInterface: Unset]");
                        }
                    }
                    else
                    {
                        ValueStr = FString::Printf(TEXT("[Input: %s]"), *InputNode->Input.GetName().ToString());
                    }
                }
                else
                {
                    ValueStr = FString::Printf(TEXT("[%s]"), *LinkedClassName);
                }
                bFoundValue = true;
            }
            else
            {
                ValueMode = TEXT("Linked");
                ValueStr = FString::Printf(TEXT("[Linked: %s]"), *LinkedClassName);
                bFoundValue = true;
            }
        }

        // CHECK OVERRIDE PINS: If no value found yet, check for override pins via parameter map nodes
        // Override pins are created in a NiagaraNodeParameterMapSet node connected to the module
        if (!bFoundValue)
        {
            FNiagaraParameterHandle AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
                Input.GetName(),
                FName(*ModuleNode->GetFunctionName())
            );
            FString AliasedHandleStr = AliasedHandle.GetParameterHandleString().ToString();

            // Find parameter map set nodes in the graph that might have override pins for this module
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                UNiagaraNodeParameterMapSet* MapSetNode = Cast<UNiagaraNodeParameterMapSet>(Node);
                if (!MapSetNode)
                {
                    continue;
                }

                // Check if this map set node is connected to our module (via parameter map output)
                bool bConnectedToModule = false;
                for (UEdGraphPin* Pin : MapSetNode->Pins)
                {
                    if (Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0)
                    {
                        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                        {
                            if (LinkedPin && LinkedPin->GetOwningNode() == ModuleNode)
                            {
                                bConnectedToModule = true;
                                break;
                            }
                        }
                    }
                    if (bConnectedToModule)
                    {
                        break;
                    }
                }

                if (!bConnectedToModule)
                {
                    continue;
                }

                // Look through this map set node's input pins for one matching our aliased handle
                for (UEdGraphPin* Pin : MapSetNode->Pins)
                {
                    if (Pin->Direction != EGPD_Input)
                    {
                        continue;
                    }

                    FString PinName = Pin->PinName.ToString();
                    // Check if this pin matches our aliased handle (e.g., "ScaleSpriteSize.Uniform Curve Sprite Scale")
                    if (PinName.Contains(AliasedHandleStr, ESearchCase::IgnoreCase) ||
                        AliasedHandleStr.Contains(PinName, ESearchCase::IgnoreCase) ||
                        PinName.EndsWith(SimpleName, ESearchCase::IgnoreCase))
                    {
                        if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0])
                        {
                            UEdGraphNode* LinkedNode = Pin->LinkedTo[0]->GetOwningNode();
                            if (UNiagaraNodeInput* InputNode = Cast<UNiagaraNodeInput>(LinkedNode))
                            {
                                if (InputNode->Input.IsDataInterface())
                                {
                                    ValueMode = TEXT("DataInterface");
                                    FProperty* DIProperty = UNiagaraNodeInput::StaticClass()->FindPropertyByName(TEXT("DataInterface"));
                                    if (DIProperty)
                                    {
                                        FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(DIProperty);
                                        if (ObjProp)
                                        {
                                            FoundDataInterface = Cast<UNiagaraDataInterface>(ObjProp->GetObjectPropertyValue_InContainer(InputNode));
                                        }
                                    }

                                    if (FoundDataInterface)
                                    {
                                        ValueStr = FString::Printf(TEXT("[DataInterface: %s]"), *FoundDataInterface->GetClass()->GetName());
                                        bFoundValue = true;
                                    }
                                }
                            }
                            else if (UNiagaraNodeFunctionCall* DynamicNode = Cast<UNiagaraNodeFunctionCall>(LinkedNode))
                            {
                                ValueMode = TEXT("Dynamic");
                                ValueStr = FString::Printf(TEXT("[Dynamic: %s]"), *DynamicNode->GetFunctionName());
                                bFoundValue = true;

                                // Extract min/max values from UniformRanged dynamic inputs
                                ExtractUniformRangedValues(DynamicNode, System, InputObj, Script, UniqueEmitterName, ScriptUsage);

                                UNiagaraDataInterface* CurveDI = FindCurveDataInterfaceFromDynamicNode(DynamicNode);
                                if (CurveDI)
                                {
                                    FoundDataInterface = CurveDI;
                                }
                            }
                            else if (UNiagaraNodeParameterMapGet* GetNode = Cast<UNiagaraNodeParameterMapGet>(LinkedNode))
                            {
                                // Linked to a parameter (e.g., Particles.NormalizedAge)
                                ValueMode = TEXT("Linked");
                                FNiagaraVariable LinkedVar = UEdGraphSchema_Niagara::PinToNiagaraVariable(Pin->LinkedTo[0]);
                                ValueStr = FString::Printf(TEXT("[Linked: %s]"), *LinkedVar.GetName().ToString());
                                bFoundValue = true;
                            }
                        }
                        if (bFoundValue)
                        {
                            break;
                        }
                    }
                }
                if (bFoundValue)
                {
                    break;
                }
            }
        }

        // SECOND: If pin is NOT connected, try RapidIterationParameters for local values
        if (!bFoundValue)
        {
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

            // Try to get the value from the script's RapidIterationParameters
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
        }

        // THIRD: If still not found, check pin default values (pin connections already handled above)
        if (!bFoundValue && MatchingPin)
        {
            if (!MatchingPin->DefaultValue.IsEmpty())
            {
                // Local value - try to read properly using PinToNiagaraVariable
                const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
                FNiagaraVariable ValueVariable = NiagaraSchema->PinToNiagaraVariable(MatchingPin, false);

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
                    ValueStr = MatchingPin->DefaultValue;

                    // Try to resolve enum display names
                    bool bEnumResolved = false;
                    if (UEnum* EnumType = Cast<UEnum>(MatchingPin->PinType.PinSubCategoryObject.Get()))
                    {
                        int64 EnumValue = INDEX_NONE;

                        // First try to parse as numeric index (common for static switches)
                        if (ValueStr.IsNumeric())
                        {
                            int32 Index = FCString::Atoi(*ValueStr);
                            if (Index >= 0 && Index < EnumType->NumEnums() - 1) // -1 to skip MAX value
                            {
                                EnumValue = Index;
                            }
                        }

                        // If not numeric, try to find by name
                        if (EnumValue == INDEX_NONE)
                        {
                            EnumValue = EnumType->GetValueByNameString(ValueStr);
                        }

                        // Get display name if we found a valid value
                        if (EnumValue != INDEX_NONE)
                        {
                            FText DisplayNameText = EnumType->GetDisplayNameTextByValue(EnumValue);
                            if (!DisplayNameText.IsEmpty())
                            {
                                // Store raw value for debugging
                                InputObj->SetStringField(TEXT("raw_value"), ValueStr);
                                ValueStr = DisplayNameText.ToString();
                            }
                        }

                        // Add available options for enum types
                        TArray<TSharedPtr<FJsonValue>> OptionsArray;
                        for (int32 i = 0; i < EnumType->NumEnums() - 1; ++i) // -1 to skip MAX value
                        {
                            TSharedPtr<FJsonObject> OptionObj = MakeShared<FJsonObject>();
                            OptionObj->SetNumberField(TEXT("index"), i);

                            FText DisplayText = EnumType->GetDisplayNameTextByIndex(i);
                            OptionObj->SetStringField(TEXT("display_name"), DisplayText.ToString());

                            FString InternalName = EnumType->GetNameStringByIndex(i);
                            OptionObj->SetStringField(TEXT("internal_name"), InternalName);

                            OptionsArray.Add(MakeShared<FJsonValueObject>(OptionObj));
                        }
                        InputObj->SetArrayField(TEXT("options"), OptionsArray);
                        InputObj->SetStringField(TEXT("input_type"), TEXT("enum"));
                        bEnumResolved = true;
                    }

                    // Fallback for Niagara static switches - check module's internal graph
                    if (!bEnumResolved && MatchingPin->PinType.PinCategory.ToString() == TEXT("Type"))
                    {
                        // Get the module's internal graph to find static switch nodes
                        UNiagaraGraph* ModuleGraph = ModuleNode->GetCalledGraph();
                        if (ModuleGraph)
                        {
                            // Find static switch by pin name
                            UNiagaraNodeStaticSwitch* SwitchNode = FindStaticSwitchNodeByName(ModuleGraph, MatchingPin->PinName);
                            if (SwitchNode)
                            {
                                // Use helper to add enum options from the static switch node
                                AddStaticSwitchEnumOptions(InputObj, SwitchNode, ValueStr);
                                // Update ValueStr if helper resolved it
                                if (InputObj->HasField(TEXT("value")))
                                {
                                    ValueStr = InputObj->GetStringField(TEXT("value"));
                                }
                            }
                        }
                    }
                    bFoundValue = true;
                }
            }
        }

        if (!bFoundValue)
        {
            ValueStr = TEXT("[Default/Unset]");
        }

        InputObj->SetStringField(TEXT("value"), ValueStr);
        InputObj->SetStringField(TEXT("value_mode"), ValueMode);

        // Extract curve keyframes if we found a curve DataInterface
        if (FoundDataInterface)
        {
            AddCurveDataToJson(InputObj, FoundDataInterface);
        }

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
                            // Extract min/max values from UniformRanged dynamic inputs
                            ExtractUniformRangedValues(DynamicNode, System, InputObj, Script, UniqueEmitterName, ScriptUsage);
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
                bool bEnumResolved = false;
                if (UEnum* EnumType = Cast<UEnum>(Pin->PinType.PinSubCategoryObject.Get()))
                {
                    int64 EnumValue = INDEX_NONE;

                    // First try to parse as numeric index (common for static switches)
                    if (Value.IsNumeric())
                    {
                        int32 Index = FCString::Atoi(*Value);
                        if (Index >= 0 && Index < EnumType->NumEnums() - 1) // -1 to skip MAX value
                        {
                            EnumValue = Index;
                        }
                    }

                    // If not numeric, try to find by name
                    if (EnumValue == INDEX_NONE)
                    {
                        EnumValue = EnumType->GetValueByNameString(Value);
                    }

                    // Get display name if we found a valid value
                    if (EnumValue != INDEX_NONE)
                    {
                        FText DisplayNameText = EnumType->GetDisplayNameTextByValue(EnumValue);
                        if (!DisplayNameText.IsEmpty())
                        {
                            // Store both raw and display value
                            InputObj->SetStringField(TEXT("raw_value"), Value);
                            Value = DisplayNameText.ToString();
                        }
                    }

                    // Add available options for enum types
                    TArray<TSharedPtr<FJsonValue>> OptionsArray;
                    for (int32 i = 0; i < EnumType->NumEnums() - 1; ++i) // -1 to skip MAX value
                    {
                        TSharedPtr<FJsonObject> OptionObj = MakeShared<FJsonObject>();
                        OptionObj->SetNumberField(TEXT("index"), i);

                        FText DisplayText = EnumType->GetDisplayNameTextByIndex(i);
                        OptionObj->SetStringField(TEXT("display_name"), DisplayText.ToString());

                        FString InternalName = EnumType->GetNameStringByIndex(i);
                        OptionObj->SetStringField(TEXT("internal_name"), InternalName);

                        OptionsArray.Add(MakeShared<FJsonValueObject>(OptionObj));
                    }
                    InputObj->SetArrayField(TEXT("options"), OptionsArray);
                    InputObj->SetStringField(TEXT("input_type"), TEXT("enum"));
                    bEnumResolved = true;
                }

                // Fallback for Niagara static switches - check module's internal graph
                // Static switches in Niagara don't expose UEnum via PinSubCategoryObject
                if (!bEnumResolved && Pin->PinType.PinCategory.ToString() == TEXT("Type"))
                {
                    // Get the module's internal graph to find static switch nodes
                    UNiagaraGraph* ModuleGraph = ModuleNode->GetCalledGraph();
                    if (ModuleGraph)
                    {
                        // Find static switch by pin name
                        UNiagaraNodeStaticSwitch* SwitchNode = FindStaticSwitchNodeByName(ModuleGraph, Pin->PinName);
                        if (SwitchNode)
                        {
                            // Use helper to add enum options from the static switch node
                            AddStaticSwitchEnumOptions(InputObj, SwitchNode, Value);
                            // Update Value if helper resolved it
                            if (InputObj->HasField(TEXT("value")))
                            {
                                Value = InputObj->GetStringField(TEXT("value"));
                            }
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

    // Get Emitter Spawn modules (emitter-level initialization)
    TArray<TSharedPtr<FJsonValue>> EmitterSpawnModules = ExtractModulesFromScript(
        EmitterData->EmitterSpawnScriptProps.Script,
        ENiagaraScriptUsage::EmitterSpawnScript
    );
    StagesObj->SetArrayField(TEXT("EmitterSpawn"), EmitterSpawnModules);

    // Get Emitter Update modules (spawn rate, spawn burst, etc.)
    TArray<TSharedPtr<FJsonValue>> EmitterUpdateModules = ExtractModulesFromScript(
        EmitterData->EmitterUpdateScriptProps.Script,
        ENiagaraScriptUsage::EmitterUpdateScript
    );
    StagesObj->SetArrayField(TEXT("EmitterUpdate"), EmitterUpdateModules);

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
    int32 TotalModules = EmitterSpawnModules.Num() + EmitterUpdateModules.Num() + SpawnModules.Num() + UpdateModules.Num() + EventModulesArray.Num();
    OutModules->SetNumberField(TEXT("total_module_count"), TotalModules);
    OutModules->SetNumberField(TEXT("emitter_spawn_count"), EmitterSpawnModules.Num());
    OutModules->SetNumberField(TEXT("emitter_update_count"), EmitterUpdateModules.Num());
    OutModules->SetNumberField(TEXT("spawn_count"), SpawnModules.Num());
    OutModules->SetNumberField(TEXT("update_count"), UpdateModules.Num());
    if (EventModulesArray.Num() > 0)
    {
        OutModules->SetNumberField(TEXT("event_count"), EventModulesArray.Num());
    }

    return true;
}
