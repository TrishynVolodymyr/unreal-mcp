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
#include "NiagaraEditorModule.h"
#include "EdGraphSchema_Niagara.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"

// Helper function to get parameter map input pin (replicates FNiagaraStackGraphUtilities::GetParameterMapInputPin logic)
// This is needed because the original function is not exported from the NiagaraEditor module
static UEdGraphPin* GetParameterMapInputPinLocal(UNiagaraNode& Node)
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

    // Calculate the correct insertion index based on module dependencies
    int32 TargetIndex = Params.Index >= 0 ? Params.Index : INDEX_NONE;

    // If user didn't specify an index, check module dependencies to find the correct position
    if (TargetIndex == INDEX_NONE)
    {
        FVersionedNiagaraScriptData* ModuleScriptData = ModuleScript->GetLatestScriptData();
        if (ModuleScriptData && ModuleScriptData->RequiredDependencies.Num() > 0)
        {
            // Use the same algorithm as FNiagaraStackGraphUtilities::GetOrderedModuleNodes
            // to get modules in correct execution order by tracing parameter map connections
            TArray<UNiagaraNodeFunctionCall*> OrderedModules;
            UNiagaraNode* PreviousNode = OutputNode;
            while (PreviousNode != nullptr)
            {
                UEdGraphPin* PreviousNodeInputPin = GetParameterMapInputPinLocal(*PreviousNode);
                if (PreviousNodeInputPin != nullptr && PreviousNodeInputPin->LinkedTo.Num() == 1)
                {
                    UNiagaraNode* CurrentNode = Cast<UNiagaraNode>(PreviousNodeInputPin->LinkedTo[0]->GetOwningNode());
                    UNiagaraNodeFunctionCall* ModuleNode = Cast<UNiagaraNodeFunctionCall>(CurrentNode);
                    if (ModuleNode != nullptr)
                    {
                        OrderedModules.Insert(ModuleNode, 0);  // Insert at front (building in reverse)
                    }
                    PreviousNode = CurrentNode;
                }
                else
                {
                    PreviousNode = nullptr;
                }
            }

            // Build array with module and its provided dependencies
            TArray<TPair<UNiagaraNodeFunctionCall*, TArray<FName>>> ExistingModulesWithDependencies;
            for (UNiagaraNodeFunctionCall* StackModule : OrderedModules)
            {
                TArray<FName> ProvidedDeps;
                if (FVersionedNiagaraScriptData* StackModuleData = StackModule->GetScriptData())
                {
                    ProvidedDeps = StackModuleData->ProvidedDependencies;
                }
                ExistingModulesWithDependencies.Add(TPair<UNiagaraNodeFunctionCall*, TArray<FName>>(StackModule, ProvidedDeps));

                // Debug: log each module and its provided dependencies
                FString ProvidedDepsStr;
                for (const FName& DepName : ProvidedDeps)
                {
                    ProvidedDepsStr += DepName.ToString() + TEXT(", ");
                }
                UE_LOG(LogNiagaraService, Log, TEXT("  [%d] Module '%s' provides: [%s]"),
                    ExistingModulesWithDependencies.Num() - 1, *StackModule->GetFunctionName(), *ProvidedDepsStr);
            }

            UE_LOG(LogNiagaraService, Log, TEXT("Module '%s' has %d required dependencies"), *ModuleScriptName, ModuleScriptData->RequiredDependencies.Num());

            // Check each required dependency
            for (const FNiagaraModuleDependency& Dependency : ModuleScriptData->RequiredDependencies)
            {
                UE_LOG(LogNiagaraService, Log, TEXT("  Checking dependency Id='%s' Type=%d"), *Dependency.Id.ToString(), (int32)Dependency.Type);

                if (Dependency.Type == ENiagaraModuleDependencyType::PostDependency)
                {
                    // PostDependency means the dependency provider must come AFTER us
                    // So we need to insert BEFORE the dependency provider
                    for (int32 i = 0; i < ExistingModulesWithDependencies.Num(); i++)
                    {
                        if (ExistingModulesWithDependencies[i].Value.Contains(Dependency.Id))
                        {
                            // Found the dependency provider - insert before it
                            if (TargetIndex == INDEX_NONE || i < TargetIndex)
                            {
                                TargetIndex = i;
                            }
                            UE_LOG(LogNiagaraService, Log, TEXT("Module '%s' has PostDependency on '%s', inserting at index %d (before '%s')"),
                                *ModuleScriptName, *Dependency.Id.ToString(), TargetIndex,
                                *ExistingModulesWithDependencies[i].Key->GetFunctionName());
                            break;
                        }
                    }
                }
                else if (Dependency.Type == ENiagaraModuleDependencyType::PreDependency)
                {
                    // PreDependency means the dependency provider must come BEFORE us
                    // So we need to insert AFTER the dependency provider
                    for (int32 i = ExistingModulesWithDependencies.Num() - 1; i >= 0; i--)
                    {
                        if (ExistingModulesWithDependencies[i].Value.Contains(Dependency.Id))
                        {
                            // Found the dependency provider - insert after it
                            int32 InsertAfterIndex = i + 1;
                            if (TargetIndex == INDEX_NONE || InsertAfterIndex > TargetIndex)
                            {
                                TargetIndex = InsertAfterIndex;
                            }
                            UE_LOG(LogNiagaraService, Log, TEXT("Module '%s' has PreDependency on '%s', inserting at index %d (after '%s')"),
                                *ModuleScriptName, *Dependency.Id.ToString(), TargetIndex,
                                *ExistingModulesWithDependencies[i].Key->GetFunctionName());
                            break;
                        }
                    }
                }
            }
        }
    }

    // Add the module using the stack graph utilities
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

    // Mark system dirty (but don't force recompile - rapid iteration params work without it)
    MarkSystemDirty(System);

    // Refresh editors to show updated values
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set input '%s' on module '%s' in emitter '%s' stage '%s' to '%s'"),
        *Params.InputName, *Params.ModuleName, *Params.EmitterName, *Params.Stage, *ValueStr);

    return true;
}

bool FNiagaraService::MoveModule(const FNiagaraModuleMoveParams& Params, FString& OutError)
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
        OutError = FString::Printf(TEXT("Unsupported stage '%s' for module move"), *Params.Stage);
        return false;
    }

    if (!Script)
    {
        OutError = FString::Printf(TEXT("Script not found for stage '%s'"), *Params.Stage);
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

    // Find the module node by name
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

    // Mark system as modified before making changes
    System->Modify();
    Graph->Modify();

    // Manual module move implementation:
    // 1. Get ordered list of modules in the chain
    // 2. Find output node for this stage
    // 3. Remove module from current position (reconnect neighbors)
    // 4. Insert at new position

    // Find the output node for this script
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

    // Get ordered modules by tracing parameter map connections backwards from output
    TArray<UNiagaraNodeFunctionCall*> OrderedModules;
    UNiagaraNode* CurrentNode = OutputNode;
    while (CurrentNode != nullptr)
    {
        UEdGraphPin* InputPin = GetParameterMapInputPinLocal(*CurrentNode);
        if (InputPin != nullptr && InputPin->LinkedTo.Num() == 1)
        {
            UNiagaraNode* PreviousNode = Cast<UNiagaraNode>(InputPin->LinkedTo[0]->GetOwningNode());
            UNiagaraNodeFunctionCall* ModuleCallNode = Cast<UNiagaraNodeFunctionCall>(PreviousNode);
            if (ModuleCallNode != nullptr)
            {
                OrderedModules.Insert(ModuleCallNode, 0);  // Insert at front (we're walking backwards)
            }
            CurrentNode = PreviousNode;
        }
        else
        {
            CurrentNode = nullptr;
        }
    }

    // Find current index of module to move
    int32 CurrentIndex = OrderedModules.IndexOfByKey(ModuleNode);
    if (CurrentIndex == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("Module '%s' not found in ordered module list"), *Params.ModuleName);
        return false;
    }

    // Validate new index
    int32 TargetIndex = Params.NewIndex;
    if (TargetIndex < 0 || TargetIndex >= OrderedModules.Num())
    {
        OutError = FString::Printf(TEXT("Invalid target index %d. Valid range is 0-%d"), TargetIndex, OrderedModules.Num() - 1);
        return false;
    }

    // If moving to same position, nothing to do
    if (CurrentIndex == TargetIndex)
    {
        UE_LOG(LogNiagaraService, Log, TEXT("Module '%s' is already at index %d"), *Params.ModuleName, TargetIndex);
        return true;
    }

    // Step 1: Get module's parameter map input and output pins
    UEdGraphPin* ModuleInputPin = GetParameterMapInputPinLocal(*ModuleNode);
    UEdGraphPin* ModuleOutputPin = nullptr;
    TArray<UEdGraphPin*> OutputPins;
    ModuleNode->GetOutputPins(OutputPins);
    for (UEdGraphPin* Pin : OutputPins)
    {
        if (Pin)
        {
            const UEdGraphSchema_Niagara* NiagaraSchema = Cast<UEdGraphSchema_Niagara>(Pin->GetSchema());
            if (NiagaraSchema)
            {
                FNiagaraTypeDefinition PinDef = NiagaraSchema->PinToTypeDefinition(Pin);
                if (PinDef == FNiagaraTypeDefinition::GetParameterMapDef())
                {
                    ModuleOutputPin = Pin;
                    break;
                }
            }
        }
    }

    if (!ModuleInputPin || !ModuleOutputPin)
    {
        OutError = TEXT("Could not find parameter map pins on module");
        return false;
    }

    // Step 2: Get the nodes connected before and after the module
    UEdGraphPin* PreviousOutputPin = (ModuleInputPin->LinkedTo.Num() > 0) ? ModuleInputPin->LinkedTo[0] : nullptr;
    UEdGraphPin* NextInputPin = (ModuleOutputPin->LinkedTo.Num() > 0) ? ModuleOutputPin->LinkedTo[0] : nullptr;

    // Step 3: Disconnect module from chain
    if (PreviousOutputPin)
    {
        ModuleInputPin->BreakLinkTo(PreviousOutputPin);
    }
    if (NextInputPin)
    {
        ModuleOutputPin->BreakLinkTo(NextInputPin);
    }

    // Step 4: Reconnect the gap (connect previous directly to next)
    if (PreviousOutputPin && NextInputPin)
    {
        const UEdGraphSchema* Schema = Graph->GetSchema();
        Schema->TryCreateConnection(PreviousOutputPin, NextInputPin);
    }

    // Step 5: Reorder the array
    OrderedModules.RemoveAt(CurrentIndex);
    OrderedModules.Insert(ModuleNode, TargetIndex);

    // Step 6: Find insertion point in chain
    // Get the pin to connect our module's INPUT to (output from module before us)
    UEdGraphPin* InsertAfterOutputPin = nullptr;
    if (TargetIndex > 0)
    {
        UNiagaraNodeFunctionCall* NodeBefore = OrderedModules[TargetIndex - 1];
        TArray<UEdGraphPin*> BeforeOutputPins;
        NodeBefore->GetOutputPins(BeforeOutputPins);
        for (UEdGraphPin* Pin : BeforeOutputPins)
        {
            if (Pin)
            {
                const UEdGraphSchema_Niagara* NiagaraSchema = Cast<UEdGraphSchema_Niagara>(Pin->GetSchema());
                if (NiagaraSchema && NiagaraSchema->PinToTypeDefinition(Pin) == FNiagaraTypeDefinition::GetParameterMapDef())
                {
                    InsertAfterOutputPin = Pin;
                    break;
                }
            }
        }
    }

    // Get the pin to connect our module's OUTPUT to (input of module after us, or output node)
    UEdGraphPin* InsertBeforeInputPin = nullptr;
    if (TargetIndex < OrderedModules.Num() - 1)
    {
        UNiagaraNodeFunctionCall* NodeAfter = OrderedModules[TargetIndex + 1];
        InsertBeforeInputPin = GetParameterMapInputPinLocal(*NodeAfter);
    }
    else
    {
        // Last module connects to output node
        InsertBeforeInputPin = GetParameterMapInputPinLocal(*OutputNode);
    }

    // Step 7: Break the connection at the insertion point
    if (InsertAfterOutputPin && InsertBeforeInputPin)
    {
        InsertAfterOutputPin->BreakLinkTo(InsertBeforeInputPin);
    }

    // Step 8: Insert module at new position
    const UEdGraphSchema* Schema = Graph->GetSchema();
    if (InsertAfterOutputPin)
    {
        Schema->TryCreateConnection(InsertAfterOutputPin, ModuleInputPin);
    }
    if (InsertBeforeInputPin)
    {
        Schema->TryCreateConnection(ModuleOutputPin, InsertBeforeInputPin);
    }

    // Mark system dirty
    MarkSystemDirty(System);

    // Notify graph of changes
    Graph->NotifyGraphChanged();

    // Request compilation
    System->RequestCompile(false);
    System->WaitForCompilationComplete();

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Moved module '%s' to index %d in emitter '%s' stage '%s'"),
        *Params.ModuleName, Params.NewIndex, *Params.EmitterName, *Params.Stage);

    return true;
}
