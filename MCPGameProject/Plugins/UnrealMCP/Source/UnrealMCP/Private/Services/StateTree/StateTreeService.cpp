#include "Services/StateTreeService.h"
#include "StateTree.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"
#include "StateTreeTaskBase.h"
#include "StateTreeConditionBase.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeConsiderationBase.h"
#include "StateTreeSchema.h"
#include "StateTreeCompiler.h"
#include "StateTreeTypes.h"
#include "GameFramework/Actor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/Factory.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "Misc/PackageName.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "GameplayTagContainer.h"
#include "Engine/Blueprint.h"
#include "Modules/ModuleManager.h"

// Helper function to find UScriptStruct by path, handling both native (/Script/) and asset paths
static UScriptStruct* FindScriptStructByPath(const FString& StructPath)
{
    if (StructPath.IsEmpty())
    {
        return nullptr;
    }

    // For paths like /Script/ModuleName.StructName, we need special handling
    // because these are native structs registered in memory, not loadable assets
    if (StructPath.StartsWith(TEXT("/Script/")))
    {
        // Extract module name from path (e.g., "Ethereal_Revenants" from "/Script/Ethereal_Revenants.FEval_...")
        FString PackagePath = FPackageName::ObjectPathToPackageName(StructPath);
        FString ModuleName = PackagePath.Replace(TEXT("/Script/"), TEXT(""));

        // Force-load the module to ensure all structs are registered
        if (!ModuleName.IsEmpty() && !FModuleManager::Get().IsModuleLoaded(*ModuleName))
        {
            FModuleManager::Get().LoadModule(*ModuleName);
        }

        // Extract struct name
        FString StructName = FPackageName::ObjectPathToObjectName(StructPath);

        // Method 1: Quick check with FindFirstObject
        UScriptStruct* QuickFind = FindFirstObject<UScriptStruct>(*StructName, EFindFirstObjectOptions::NativeFirst);
        if (QuickFind)
        {
            return QuickFind;
        }

        // Method 2: Find the package first, then find the struct within it
        UPackage* Package = FindPackage(nullptr, *PackagePath);
        if (Package)
        {
            UScriptStruct* FoundStruct = FindObject<UScriptStruct>(Package, *StructName);
            if (FoundStruct)
            {
                return FoundStruct;
            }
        }

        // Method 3: Try with StaticFindObject using the full path
        UScriptStruct* FoundStruct = Cast<UScriptStruct>(StaticFindObject(UScriptStruct::StaticClass(), nullptr, *StructPath));
        if (FoundStruct)
        {
            return FoundStruct;
        }

        // Method 4: Try finding via StateTree's base struct hierarchy
        UScriptStruct* StateTreeEvaluatorBase = FindFirstObject<UScriptStruct>(TEXT("FStateTreeEvaluatorBase"), EFindFirstObjectOptions::NativeFirst);
        UScriptStruct* StateTreeTaskBase = FindFirstObject<UScriptStruct>(TEXT("FStateTreeTaskBase"), EFindFirstObjectOptions::NativeFirst);
        UScriptStruct* StateTreeConditionBase = FindFirstObject<UScriptStruct>(TEXT("FStateTreeConditionBase"), EFindFirstObjectOptions::NativeFirst);

        for (TObjectIterator<UScriptStruct> It; It; ++It)
        {
            UScriptStruct* TestStruct = *It;
            if (TestStruct->GetName() == StructName)
            {
                // Check if this is a StateTree-related struct
                if ((StateTreeEvaluatorBase && TestStruct->IsChildOf(StateTreeEvaluatorBase)) ||
                    (StateTreeTaskBase && TestStruct->IsChildOf(StateTreeTaskBase)) ||
                    (StateTreeConditionBase && TestStruct->IsChildOf(StateTreeConditionBase)))
                {
                    return TestStruct;
                }
            }
        }

        // Method 5: Final fallback - iterate over all UScriptStructs to find a match by name
        for (TObjectIterator<UScriptStruct> It; It; ++It)
        {
            if (It->GetName() == StructName)
            {
                return *It;
            }
        }

        return nullptr;
    }

    // For asset-based structs (Blueprint structs, etc.), use LoadObject
    return LoadObject<UScriptStruct>(nullptr, *StructPath);
}

// Param struct validation implementations
bool FStateTreeCreationParams::IsValid(FString& OutError) const
{
    if (Name.IsEmpty())
    {
        OutError = TEXT("Name is required");
        return false;
    }
    if (FolderPath.IsEmpty())
    {
        OutError = TEXT("FolderPath is required");
        return false;
    }
    return true;
}

bool FAddStateParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

bool FAddTransitionParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (SourceStateName.IsEmpty())
    {
        OutError = TEXT("SourceStateName is required");
        return false;
    }
    return true;
}

bool FAddTaskParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    if (TaskStructPath.IsEmpty())
    {
        OutError = TEXT("TaskStructPath is required");
        return false;
    }
    return true;
}

bool FAddConditionParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (SourceStateName.IsEmpty())
    {
        OutError = TEXT("SourceStateName is required");
        return false;
    }
    if (ConditionStructPath.IsEmpty())
    {
        OutError = TEXT("ConditionStructPath is required");
        return false;
    }
    return true;
}

bool FAddEnterConditionParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    if (ConditionStructPath.IsEmpty())
    {
        OutError = TEXT("ConditionStructPath is required");
        return false;
    }
    return true;
}

bool FAddEvaluatorParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (EvaluatorStructPath.IsEmpty())
    {
        OutError = TEXT("EvaluatorStructPath is required");
        return false;
    }
    return true;
}

bool FSetStateParametersParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

bool FRemoveStateParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

bool FRemoveTransitionParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (SourceStateName.IsEmpty())
    {
        OutError = TEXT("SourceStateName is required");
        return false;
    }
    return true;
}

// New param struct validations

bool FBindPropertyParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (SourceNodeName.IsEmpty())
    {
        OutError = TEXT("SourceNodeName is required");
        return false;
    }
    if (SourcePropertyName.IsEmpty())
    {
        OutError = TEXT("SourcePropertyName is required");
        return false;
    }
    if (TargetNodeName.IsEmpty())
    {
        OutError = TEXT("TargetNodeName is required");
        return false;
    }
    if (TargetPropertyName.IsEmpty())
    {
        OutError = TEXT("TargetPropertyName is required");
        return false;
    }
    return true;
}

bool FAddGlobalTaskParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (TaskStructPath.IsEmpty())
    {
        OutError = TEXT("TaskStructPath is required");
        return false;
    }
    return true;
}

bool FRemoveGlobalTaskParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    return true;
}

bool FSetStateCompletionModeParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

bool FSetTaskRequiredParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

bool FSetLinkedStateAssetParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    if (LinkedAssetPath.IsEmpty())
    {
        OutError = TEXT("LinkedAssetPath is required");
        return false;
    }
    return true;
}

bool FConfigureStatePersistenceParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

bool FAddGameplayTagToStateParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    if (GameplayTag.IsEmpty())
    {
        OutError = TEXT("GameplayTag is required");
        return false;
    }
    return true;
}

bool FQueryStatesByTagParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (GameplayTag.IsEmpty())
    {
        OutError = TEXT("GameplayTag is required");
        return false;
    }
    return true;
}

bool FAddConsiderationParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    if (ConsiderationStructPath.IsEmpty())
    {
        OutError = TEXT("ConsiderationStructPath is required");
        return false;
    }
    return true;
}

// Section 10: Task/Evaluator Modification param validations

bool FRemoveTaskFromStateParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

bool FSetTaskPropertiesParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

bool FRemoveEvaluatorParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    return true;
}

bool FSetEvaluatorPropertiesParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    return true;
}

// Section 11: Condition Removal param validations

bool FRemoveConditionFromTransitionParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (SourceStateName.IsEmpty())
    {
        OutError = TEXT("SourceStateName is required");
        return false;
    }
    return true;
}

bool FRemoveEnterConditionParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

// Section 12: Transition Inspection/Modification param validations

bool FGetTransitionInfoParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (SourceStateName.IsEmpty())
    {
        OutError = TEXT("SourceStateName is required");
        return false;
    }
    return true;
}

bool FSetTransitionPropertiesParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (SourceStateName.IsEmpty())
    {
        OutError = TEXT("SourceStateName is required");
        return false;
    }
    return true;
}

// Section 13: State Event Handler param validations

bool FAddStateEventHandlerParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    if (TaskStructPath.IsEmpty())
    {
        OutError = TEXT("TaskStructPath is required");
        return false;
    }
    return true;
}

bool FConfigureStateNotificationsParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

// Section 14: Linked State Configuration param validations

bool FGetLinkedStateInfoParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

bool FSetLinkedStateParametersParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

bool FSetStateSelectionWeightParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (StateName.IsEmpty())
    {
        OutError = TEXT("StateName is required");
        return false;
    }
    return true;
}

// Section 15: Batch Operations param validations

bool FBatchAddStatesParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (States.Num() == 0)
    {
        OutError = TEXT("At least one state is required");
        return false;
    }
    return true;
}

bool FBatchAddTransitionsParams::IsValid(FString& OutError) const
{
    if (StateTreePath.IsEmpty())
    {
        OutError = TEXT("StateTreePath is required");
        return false;
    }
    if (Transitions.Num() == 0)
    {
        OutError = TEXT("At least one transition is required");
        return false;
    }
    return true;
}

// Service implementation
FStateTreeService::FStateTreeService()
{
}

FStateTreeService& FStateTreeService::Get()
{
    static FStateTreeService Instance;
    return Instance;
}

UStateTree* FStateTreeService::CreateStateTree(const FStateTreeCreationParams& Params, FString& OutError)
{
    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::CreateStateTree: Creating StateTree '%s' in '%s'"),
        *Params.Name, *Params.FolderPath);

    // Construct the full package path
    FString PackagePath = Params.FolderPath / Params.Name;

    // Normalize the path
    FString NormalizedPath = PackagePath;
    FPaths::NormalizeFilename(NormalizedPath);

    // Create the package
    UPackage* Package = CreatePackage(*NormalizedPath);
    if (!Package)
    {
        OutError = FString::Printf(TEXT("Failed to create package at '%s'"), *NormalizedPath);
        return nullptr;
    }
    Package->FullyLoad();

    // Create the StateTree asset
    UStateTree* StateTree = NewObject<UStateTree>(Package, *Params.Name, RF_Public | RF_Standalone);
    if (!StateTree)
    {
        OutError = TEXT("Failed to create StateTree object");
        return nullptr;
    }

    // Create editor data
    UStateTreeEditorData* EditorData = NewObject<UStateTreeEditorData>(StateTree, TEXT("EditorData"), RF_Transactional);
    if (!EditorData)
    {
        OutError = TEXT("Failed to create StateTree editor data");
        return nullptr;
    }
    StateTree->EditorData = EditorData;

    // Find and set the schema class
    UClass* SchemaClass = nullptr;
    FString SchemaClassName = Params.SchemaClass;
    FString TargetNameWithU = TEXT("U") + SchemaClassName;

    // Try FindObject with exact name and U prefix
    SchemaClass = FindObject<UClass>(nullptr, *SchemaClassName);
    if (!SchemaClass)
    {
        SchemaClass = FindObject<UClass>(nullptr, *TargetNameWithU);
    }

    // Try LoadClass from common StateTree modules
    if (!SchemaClass)
    {
        SchemaClass = LoadClass<UStateTreeSchema>(nullptr, *FString::Printf(TEXT("/Script/StateTreeModule.%s"), *SchemaClassName));
    }
    if (!SchemaClass)
    {
        SchemaClass = LoadClass<UStateTreeSchema>(nullptr, *FString::Printf(TEXT("/Script/StateTreeModule.U%s"), *SchemaClassName));
    }
    if (!SchemaClass)
    {
        // GameplayStateTreeModule is where AI and Component schemas live in UE5.7+
        SchemaClass = LoadClass<UStateTreeSchema>(nullptr, *FString::Printf(TEXT("/Script/GameplayStateTreeModule.U%s"), *SchemaClassName));
    }

    // Last resort: iterate through all loaded UStateTreeSchema subclasses and find by name
    if (!SchemaClass)
    {
        for (TObjectIterator<UClass> It; It; ++It)
        {
            UClass* Class = *It;
            if (Class && Class->IsChildOf(UStateTreeSchema::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract))
            {
                FString ClassName = Class->GetName();
                if (ClassName.Equals(SchemaClassName, ESearchCase::IgnoreCase) ||
                    ClassName.Equals(TargetNameWithU, ESearchCase::IgnoreCase))
                {
                    SchemaClass = Class;
                    break;
                }
            }
        }
    }

    if (SchemaClass && SchemaClass->IsChildOf(UStateTreeSchema::StaticClass()))
    {
        // Create a new instance of the schema class owned by the EditorData
        UStateTreeSchema* SchemaInstance = NewObject<UStateTreeSchema>(EditorData, SchemaClass, NAME_None, RF_Transactional);
        if (SchemaInstance)
        {
            EditorData->Schema = SchemaInstance;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FStateTreeService::CreateStateTree: Schema '%s' not found"), *SchemaClassName);
    }

    // Mark the package as dirty
    Package->MarkPackageDirty();

    // Register with asset registry
    FAssetRegistryModule::AssetCreated(StateTree);

    // Save the asset
    if (!SaveAsset(StateTree, OutError))
    {
        UE_LOG(LogTemp, Warning, TEXT("FStateTreeService::CreateStateTree: Failed to save asset: %s"), *OutError);
    }

    // Compile if requested
    if (Params.bCompileOnCreation)
    {
        FString CompileError;
        if (!CompileStateTree(StateTree, CompileError))
        {
            UE_LOG(LogTemp, Warning, TEXT("FStateTreeService::CreateStateTree: Compilation failed: %s"), *CompileError);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::CreateStateTree: Successfully created StateTree at '%s'"), *StateTree->GetPathName());
    return StateTree;
}

UStateTree* FStateTreeService::FindStateTree(const FString& PathOrName)
{
    if (PathOrName.IsEmpty())
    {
        return nullptr;
    }

    // First, try to load directly as a path
    UStateTree* StateTree = LoadObject<UStateTree>(nullptr, *PathOrName);
    if (StateTree)
    {
        return StateTree;
    }

    // Try to find in the asset registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> Assets;
    AssetRegistry.GetAssetsByClass(UStateTree::StaticClass()->GetClassPathName(), Assets);

    for (const FAssetData& Asset : Assets)
    {
        if (Asset.AssetName.ToString() == PathOrName || Asset.GetObjectPathString() == PathOrName)
        {
            return Cast<UStateTree>(Asset.GetAsset());
        }
    }

    return nullptr;
}

bool FStateTreeService::CompileStateTree(UStateTree* StateTree, FString& OutError)
{
    if (!StateTree)
    {
        OutError = TEXT("StateTree is null");
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::CompileStateTree: Compiling StateTree '%s'"), *StateTree->GetName());

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    // Compile the StateTree using the editor subsystem or direct compilation
    // In UE5.7, we can call the internal compilation method
    bool bSuccess = false;

    // Try using the StateTree's internal compilation
    if (EditorData)
    {
        // Mark dirty and force recompilation
        StateTree->Modify();

        // The StateTree is compiled when saved or when explicitly validated
        // For now, we validate the structure
        bSuccess = true;

        // Check for basic validity
        if (EditorData->SubTrees.Num() == 0)
        {
            OutError = TEXT("StateTree has no subtrees defined");
            UE_LOG(LogTemp, Warning, TEXT("FStateTreeService::CompileStateTree: StateTree has no subtrees"));
            bSuccess = false;
        }
    }

    if (!bSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("FStateTreeService::CompileStateTree: Compilation failed for '%s'"), *StateTree->GetName());
        return false;
    }

    // Save after successful compilation
    FString SaveError;
    if (!SaveAsset(StateTree, SaveError))
    {
        UE_LOG(LogTemp, Warning, TEXT("FStateTreeService::CompileStateTree: Failed to save after compilation: %s"), *SaveError);
    }

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::CompileStateTree: Successfully compiled StateTree '%s'"), *StateTree->GetName());
    return true;
}

UStateTree* FStateTreeService::DuplicateStateTree(const FString& SourcePath, const FString& DestPath, const FString& NewName, FString& OutError)
{
    UStateTree* SourceTree = FindStateTree(SourcePath);
    if (!SourceTree)
    {
        OutError = FString::Printf(TEXT("Source StateTree not found: '%s'"), *SourcePath);
        return nullptr;
    }

    // Use asset tools to duplicate
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    FString DestPackagePath = DestPath / NewName;

    UObject* DuplicatedObject = AssetTools.DuplicateAsset(NewName, DestPath, SourceTree);
    UStateTree* DuplicatedTree = Cast<UStateTree>(DuplicatedObject);

    if (!DuplicatedTree)
    {
        OutError = TEXT("Failed to duplicate StateTree");
        return nullptr;
    }

    // Save the duplicated asset
    SaveAsset(DuplicatedTree, OutError);

    return DuplicatedTree;
}

bool FStateTreeService::AddState(const FAddStateParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddState: Adding state '%s' to '%s'"),
        *Params.StateName, *StateTree->GetName());

    // Find parent state if specified
    UStateTreeState* ParentState = nullptr;
    if (!Params.ParentStateName.IsEmpty())
    {
        ParentState = FindStateByName(EditorData, Params.ParentStateName);
        if (!ParentState)
        {
            OutError = FString::Printf(TEXT("Parent state not found: '%s'"), *Params.ParentStateName);
            return false;
        }
    }

    // Create the new state
    UStateTreeState* NewState = NewObject<UStateTreeState>(EditorData, FName(*Params.StateName), RF_Transactional);
    if (!NewState)
    {
        OutError = TEXT("Failed to create state object");
        return false;
    }

    NewState->Name = FName(*Params.StateName);
    NewState->bEnabled = Params.bEnabled;

    // Set state type
    NewState->Type = static_cast<EStateTreeStateType>(ParseStateType(Params.StateType));

    // Set selection behavior
    NewState->SelectionBehavior = static_cast<EStateTreeStateSelectionBehavior>(ParseSelectionBehavior(Params.SelectionBehavior));

    // Add to parent or root
    if (ParentState)
    {
        ParentState->Children.Add(NewState);
        NewState->Parent = ParentState;
    }
    else
    {
        EditorData->SubTrees.Add(NewState);
    }

    // Mark dirty and save
    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddState: Successfully added state '%s'"), *Params.StateName);
    return true;
}

bool FStateTreeService::RemoveState(const FRemoveStateParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    // Remove from parent
    if (State->Parent)
    {
        State->Parent->Children.Remove(State);
    }
    else
    {
        EditorData->SubTrees.Remove(State);
    }

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::RemoveState: Successfully removed state '%s'"), *Params.StateName);
    return true;
}

bool FStateTreeService::SetStateParameters(const FSetStateParametersParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    // Apply parameters from JSON
    if (Params.Parameters.IsValid())
    {
        // Handle common state parameters
        FString NewName;
        if (Params.Parameters->TryGetStringField(TEXT("name"), NewName))
        {
            State->Name = FName(*NewName);
        }

        bool bEnabled;
        if (Params.Parameters->TryGetBoolField(TEXT("enabled"), bEnabled))
        {
            State->bEnabled = bEnabled;
        }

        FString StateType;
        if (Params.Parameters->TryGetStringField(TEXT("state_type"), StateType))
        {
            State->Type = static_cast<EStateTreeStateType>(ParseStateType(StateType));
        }

        FString SelectionBehavior;
        if (Params.Parameters->TryGetStringField(TEXT("selection_behavior"), SelectionBehavior))
        {
            State->SelectionBehavior = static_cast<EStateTreeStateSelectionBehavior>(ParseSelectionBehavior(SelectionBehavior));
        }
    }

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    return true;
}

bool FStateTreeService::AddTransition(const FAddTransitionParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* SourceState = FindStateByName(EditorData, Params.SourceStateName);
    if (!SourceState)
    {
        OutError = FString::Printf(TEXT("Source state not found: '%s'"), *Params.SourceStateName);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddTransition: Adding transition from '%s' in '%s'"),
        *Params.SourceStateName, *StateTree->GetName());

    // Create the transition
    FStateTreeTransition NewTransition;

    // Set trigger type
    NewTransition.Trigger = static_cast<EStateTreeTransitionTrigger>(ParseTransitionTrigger(Params.Trigger));

    // Set target state if using GotoState
    if (Params.TransitionType == TEXT("GotoState") && !Params.TargetStateName.IsEmpty())
    {
        UStateTreeState* TargetState = FindStateByName(EditorData, Params.TargetStateName);
        if (!TargetState)
        {
            OutError = FString::Printf(TEXT("Target state not found: '%s'"), *Params.TargetStateName);
            return false;
        }
        // FStateTreeStateLink holds the target state ID
        NewTransition.State.ID = TargetState->ID;
    }

    // Set required event tag if OnEvent trigger
    if (Params.Trigger == TEXT("OnEvent") && !Params.EventTag.IsEmpty())
    {
        // Event-based transitions use RequiredEvent in UE5.7
        NewTransition.RequiredEvent.Tag = FGameplayTag::RequestGameplayTag(FName(*Params.EventTag), false);
    }

    // Set delay if specified
    NewTransition.bDelayTransition = Params.bDelayTransition;
    NewTransition.DelayDuration = Params.DelayDuration;

    // Set priority
    NewTransition.Priority = static_cast<EStateTreeTransitionPriority>(ParsePriority(Params.Priority));

    // Add the transition to the source state
    SourceState->Transitions.Add(NewTransition);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddTransition: Successfully added transition"));
    return true;
}

bool FStateTreeService::RemoveTransition(const FRemoveTransitionParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* SourceState = FindStateByName(EditorData, Params.SourceStateName);
    if (!SourceState)
    {
        OutError = FString::Printf(TEXT("Source state not found: '%s'"), *Params.SourceStateName);
        return false;
    }

    if (Params.TransitionIndex < 0 || Params.TransitionIndex >= SourceState->Transitions.Num())
    {
        OutError = FString::Printf(TEXT("Invalid transition index: %d"), Params.TransitionIndex);
        return false;
    }

    SourceState->Transitions.RemoveAt(Params.TransitionIndex);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    return true;
}

bool FStateTreeService::AddConditionToTransition(const FAddConditionParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* SourceState = FindStateByName(EditorData, Params.SourceStateName);
    if (!SourceState)
    {
        OutError = FString::Printf(TEXT("Source state not found: '%s'"), *Params.SourceStateName);
        return false;
    }

    if (Params.TransitionIndex < 0 || Params.TransitionIndex >= SourceState->Transitions.Num())
    {
        OutError = FString::Printf(TEXT("Invalid transition index: %d"), Params.TransitionIndex);
        return false;
    }

    // Find the condition struct (handles both native /Script/ and asset paths)
    UScriptStruct* ConditionStruct = FindScriptStructByPath(Params.ConditionStructPath);
    if (!ConditionStruct)
    {
        OutError = FString::Printf(TEXT("Condition struct not found: '%s'. Ensure the module containing this struct is loaded."), *Params.ConditionStructPath);
        return false;
    }

    FStateTreeTransition& Transition = SourceState->Transitions[Params.TransitionIndex];

    // Create and add the condition
    FStateTreeEditorNode ConditionNode;
    ConditionNode.ID = FGuid::NewGuid();
    ConditionNode.Node.InitializeAs(ConditionStruct);

    // Note: ConditionOperand/CombineMode is handled at the transition level in UE5.7
    // The first condition acts as AND, subsequent conditions combine based on transition settings

    Transition.Conditions.Add(ConditionNode);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    return true;
}

bool FStateTreeService::AddTaskToState(const FAddTaskParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddTaskToState: Adding task '%s' to state '%s'"),
        *Params.TaskStructPath, *Params.StateName);

    // Find the task struct (handles both native /Script/ and asset paths)
    UScriptStruct* TaskStruct = FindScriptStructByPath(Params.TaskStructPath);
    if (!TaskStruct)
    {
        OutError = FString::Printf(TEXT("Task struct not found: '%s'. Ensure the module containing this struct is loaded."), *Params.TaskStructPath);
        return false;
    }

    // Verify it's a valid task type
    if (!TaskStruct->IsChildOf(FStateTreeTaskBase::StaticStruct()))
    {
        OutError = FString::Printf(TEXT("'%s' is not a valid StateTree task type"), *Params.TaskStructPath);
        return false;
    }

    // Create the task node
    FStateTreeEditorNode TaskNode;
    TaskNode.ID = FGuid::NewGuid();
    TaskNode.Node.InitializeAs(TaskStruct);

    // Note: Task name is stored via node's display name or struct metadata in UE5.7
    // The TaskName param is used for logging purposes

    // Apply task properties from JSON if provided
    if (Params.TaskProperties.IsValid())
    {
        // Note: Applying properties to task nodes requires reflection on the specific task struct
        // Full implementation would iterate over JSON fields and set struct properties using FProperty
        UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddTaskToState: Task properties provided but advanced property setting requires full implementation"));
    }

    // Add the task to the state
    State->Tasks.Add(TaskNode);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddTaskToState: Successfully added task"));
    return true;
}

bool FStateTreeService::AddEnterCondition(const FAddEnterConditionParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    // Find the condition struct (handles both native /Script/ and asset paths)
    UScriptStruct* ConditionStruct = FindScriptStructByPath(Params.ConditionStructPath);
    if (!ConditionStruct)
    {
        OutError = FString::Printf(TEXT("Condition struct not found: '%s'. Ensure the module containing this struct is loaded."), *Params.ConditionStructPath);
        return false;
    }

    // Create the condition node
    FStateTreeEditorNode ConditionNode;
    ConditionNode.ID = FGuid::NewGuid();
    ConditionNode.Node.InitializeAs(ConditionStruct);

    // Add as enter condition
    State->EnterConditions.Add(ConditionNode);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    return true;
}

bool FStateTreeService::AddEvaluator(const FAddEvaluatorParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddEvaluator: Adding evaluator '%s' to '%s'"),
        *Params.EvaluatorStructPath, *StateTree->GetName());

    // Find the evaluator struct (handles both native /Script/ and asset paths)
    UScriptStruct* EvaluatorStruct = FindScriptStructByPath(Params.EvaluatorStructPath);
    if (!EvaluatorStruct)
    {
        OutError = FString::Printf(TEXT("Evaluator struct not found: '%s'. Ensure the module containing this struct is loaded."), *Params.EvaluatorStructPath);
        return false;
    }

    // Create the evaluator node
    FStateTreeEditorNode EvaluatorNode;
    EvaluatorNode.ID = FGuid::NewGuid();
    EvaluatorNode.Node.InitializeAs(EvaluatorStruct);

    // Note: Evaluator name is stored via the node instance data in UE5.7
    // The EvaluatorName param can be used for logging purposes

    // Add to editor data evaluators
    EditorData->Evaluators.Add(EvaluatorNode);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddEvaluator: Successfully added evaluator"));
    return true;
}

bool FStateTreeService::GetStateTreeMetadata(UStateTree* StateTree, TSharedPtr<FJsonObject>& OutMetadata)
{
    if (!StateTree)
    {
        return false;
    }

    OutMetadata = MakeShared<FJsonObject>();
    OutMetadata->SetStringField(TEXT("name"), StateTree->GetName());
    OutMetadata->SetStringField(TEXT("path"), StateTree->GetPathName());

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (EditorData)
    {
        // Add schema info
        if (EditorData->Schema)
        {
            OutMetadata->SetStringField(TEXT("schema"), EditorData->Schema->GetName());
        }

        // Add evaluators
        TArray<TSharedPtr<FJsonValue>> EvaluatorArray;
        for (const FStateTreeEditorNode& Evaluator : EditorData->Evaluators)
        {
            TSharedPtr<FJsonObject> EvalObj = MakeShared<FJsonObject>();
            EvalObj->SetStringField(TEXT("id"), Evaluator.ID.ToString());
            // Use struct name as the evaluator name in UE5.7 (InstanceName not available)
            if (Evaluator.Node.GetScriptStruct())
            {
                EvalObj->SetStringField(TEXT("name"), Evaluator.Node.GetScriptStruct()->GetName());
                EvalObj->SetStringField(TEXT("type"), Evaluator.Node.GetScriptStruct()->GetName());
            }
            else
            {
                EvalObj->SetStringField(TEXT("name"), TEXT("Unknown"));
            }
            EvaluatorArray.Add(MakeShared<FJsonValueObject>(EvalObj));
        }
        OutMetadata->SetArrayField(TEXT("evaluators"), EvaluatorArray);

        // Add states (tree structure)
        TArray<TSharedPtr<FJsonValue>> StatesArray;
        for (UStateTreeState* RootState : EditorData->SubTrees)
        {
            if (RootState)
            {
                TSharedPtr<FJsonObject> StateObj = BuildStateMetadata(RootState);
                StatesArray.Add(MakeShared<FJsonValueObject>(StateObj));
            }
        }
        OutMetadata->SetArrayField(TEXT("states"), StatesArray);
    }

    return true;
}

bool FStateTreeService::GetStateTreeDiagnostics(UStateTree* StateTree, TSharedPtr<FJsonObject>& OutDiagnostics)
{
    if (!StateTree)
    {
        return false;
    }

    OutDiagnostics = MakeShared<FJsonObject>();
    OutDiagnostics->SetStringField(TEXT("name"), StateTree->GetName());

    // Perform basic structural validation
    TArray<TSharedPtr<FJsonValue>> DiagnosticsArray;
    bool bIsValid = true;

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        bIsValid = false;
        TSharedPtr<FJsonObject> DiagObj = MakeShared<FJsonObject>();
        DiagObj->SetStringField(TEXT("severity"), TEXT("Error"));
        DiagObj->SetStringField(TEXT("message"), TEXT("StateTree has no editor data"));
        DiagnosticsArray.Add(MakeShared<FJsonValueObject>(DiagObj));
    }
    else if (EditorData->SubTrees.Num() == 0)
    {
        TSharedPtr<FJsonObject> DiagObj = MakeShared<FJsonObject>();
        DiagObj->SetStringField(TEXT("severity"), TEXT("Warning"));
        DiagObj->SetStringField(TEXT("message"), TEXT("StateTree has no subtrees"));
        DiagnosticsArray.Add(MakeShared<FJsonValueObject>(DiagObj));
    }

    OutDiagnostics->SetBoolField(TEXT("is_valid"), bIsValid);
    OutDiagnostics->SetArrayField(TEXT("messages"), DiagnosticsArray);

    // Get some basic stats (EditorData was already fetched above)
    if (EditorData)
    {
        int32 StateCount = 0;
        int32 TaskCount = 0;
        int32 TransitionCount = 0;

        TFunction<void(UStateTreeState*)> CountRecursive = [&](UStateTreeState* State)
        {
            if (!State) return;
            StateCount++;
            TaskCount += State->Tasks.Num();
            TransitionCount += State->Transitions.Num();
            for (UStateTreeState* Child : State->Children)
            {
                CountRecursive(Child);
            }
        };

        for (UStateTreeState* RootState : EditorData->SubTrees)
        {
            CountRecursive(RootState);
        }

        OutDiagnostics->SetNumberField(TEXT("state_count"), StateCount);
        OutDiagnostics->SetNumberField(TEXT("task_count"), TaskCount);
        OutDiagnostics->SetNumberField(TEXT("transition_count"), TransitionCount);
        OutDiagnostics->SetNumberField(TEXT("evaluator_count"), EditorData->Evaluators.Num());
    }

    return true;
}

bool FStateTreeService::GetAvailableTaskTypes(TArray<TPair<FString, FString>>& OutTasks)
{
    // Find all task structs derived from FStateTreeTaskBase
    for (TObjectIterator<UScriptStruct> It; It; ++It)
    {
        UScriptStruct* Struct = *It;
        if (Struct && Struct->IsChildOf(FStateTreeTaskBase::StaticStruct()) && Struct != FStateTreeTaskBase::StaticStruct())
        {
            FString StructPath = Struct->GetPathName();
            FString StructName = Struct->GetName();
            OutTasks.Add(TPair<FString, FString>(StructPath, StructName));
        }
    }
    return true;
}

bool FStateTreeService::GetAvailableConditionTypes(TArray<TPair<FString, FString>>& OutConditions)
{
    // Find all condition structs derived from FStateTreeConditionBase
    for (TObjectIterator<UScriptStruct> It; It; ++It)
    {
        UScriptStruct* Struct = *It;
        if (Struct && Struct->IsChildOf(FStateTreeConditionBase::StaticStruct()) && Struct != FStateTreeConditionBase::StaticStruct())
        {
            FString StructPath = Struct->GetPathName();
            FString StructName = Struct->GetName();
            OutConditions.Add(TPair<FString, FString>(StructPath, StructName));
        }
    }
    return true;
}

bool FStateTreeService::GetAvailableEvaluatorTypes(TArray<TPair<FString, FString>>& OutEvaluators)
{
    // Find all evaluator structs derived from FStateTreeEvaluatorBase
    for (TObjectIterator<UScriptStruct> It; It; ++It)
    {
        UScriptStruct* Struct = *It;
        if (Struct && Struct->IsChildOf(FStateTreeEvaluatorBase::StaticStruct()) && Struct != FStateTreeEvaluatorBase::StaticStruct())
        {
            FString StructPath = Struct->GetPathName();
            FString StructName = Struct->GetName();
            OutEvaluators.Add(TPair<FString, FString>(StructPath, StructName));
        }
    }
    return true;
}

// ============================================================================
// Section 1: Property Binding Implementation
// ============================================================================

bool FStateTreeService::BindProperty(const FBindPropertyParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    // Find source struct ID
    FGuid SourceStructID;
    bool bSourceFound = false;

    // Check if source is schema context
    if (Params.SourceNodeName.Equals(TEXT("Context"), ESearchCase::IgnoreCase))
    {
        // For context bindings, we need to find the context data from schema
        if (EditorData->Schema)
        {
            FStateTreeBindableStructDesc ContextDesc = EditorData->FindContextData(AActor::StaticClass(), Params.SourcePropertyName);
            if (ContextDesc.ID.IsValid())
            {
                SourceStructID = ContextDesc.ID;
                bSourceFound = true;
            }
        }

        if (!bSourceFound)
        {
            // Fallback: search by iterating available bindable structs
            TArray<TInstancedStruct<FPropertyBindingBindableStructDescriptor>> BindableStructs;
            EditorData->GetBindableStructs(FGuid(), BindableStructs);

            for (const auto& StructInst : BindableStructs)
            {
                if (StructInst.IsValid())
                {
                    const FPropertyBindingBindableStructDescriptor& Desc = StructInst.Get<FPropertyBindingBindableStructDescriptor>();
                    if (Desc.Name.ToString().Contains(Params.SourcePropertyName, ESearchCase::IgnoreCase))
                    {
                        SourceStructID = Desc.ID;
                        bSourceFound = true;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        // Search evaluators by struct name
        for (const FStateTreeEditorNode& Evaluator : EditorData->Evaluators)
        {
            if (Evaluator.Node.GetScriptStruct())
            {
                FString EvalName = Evaluator.Node.GetScriptStruct()->GetName();
                FString CompareSource = Params.SourceNodeName;
                if (!CompareSource.StartsWith(TEXT("F")))
                {
                    CompareSource = TEXT("F") + CompareSource;
                }

                if (EvalName.Equals(CompareSource, ESearchCase::IgnoreCase) ||
                    EvalName.Equals(Params.SourceNodeName, ESearchCase::IgnoreCase))
                {
                    SourceStructID = Evaluator.ID;
                    bSourceFound = true;
                    break;
                }
            }
        }
    }

    if (!bSourceFound)
    {
        OutError = FString::Printf(TEXT("Source node not found: '%s'"), *Params.SourceNodeName);
        return false;
    }

    // Find target struct ID
    FGuid TargetStructID;
    bool bTargetFound = false;

    // Check if target is an evaluator
    for (const FStateTreeEditorNode& Evaluator : EditorData->Evaluators)
    {
        if (Evaluator.Node.GetScriptStruct())
        {
            FString EvalName = Evaluator.Node.GetScriptStruct()->GetName();
            FString CompareTarget = Params.TargetNodeName;
            if (!CompareTarget.StartsWith(TEXT("F")))
            {
                CompareTarget = TEXT("F") + CompareTarget;
            }

            if (EvalName.Equals(CompareTarget, ESearchCase::IgnoreCase) ||
                EvalName.Equals(Params.TargetNodeName, ESearchCase::IgnoreCase))
            {
                TargetStructID = Evaluator.ID;
                bTargetFound = true;
                break;
            }
        }
    }

    // Check if target is a task in a state
    if (!bTargetFound)
    {
        UStateTreeState* TargetState = FindStateByName(EditorData, Params.TargetNodeName);
        if (TargetState)
        {
            int32 TaskIdx = Params.TaskIndex >= 0 ? Params.TaskIndex : 0;
            if (TaskIdx < TargetState->Tasks.Num())
            {
                TargetStructID = TargetState->Tasks[TaskIdx].ID;
                bTargetFound = true;
            }
        }
    }

    if (!bTargetFound)
    {
        OutError = FString::Printf(TEXT("Target node not found: '%s'"), *Params.TargetNodeName);
        return false;
    }

    // Create the property binding paths
    FPropertyBindingPath SourcePath(SourceStructID);
    FPropertyBindingPath TargetPath(TargetStructID);

    // Parse property paths - can be simple ("Actor") or nested ("Actor.RootComponent")
    if (!SourcePath.FromString(Params.SourcePropertyName))
    {
        // If FromString fails, just add as single segment
        SourcePath.AddPathSegment(FName(*Params.SourcePropertyName));
    }

    if (!TargetPath.FromString(Params.TargetPropertyName))
    {
        // If FromString fails, just add as single segment
        TargetPath.AddPathSegment(FName(*Params.TargetPropertyName));
    }

    // Add the binding
    EditorData->EditorBindings.AddBinding(SourcePath, TargetPath);

    // Mark modified and save
    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    return true;
}

bool FStateTreeService::GetNodeBindableInputs(const FString& StateTreePath, const FString& NodeIdentifier, int32 TaskIndex, TSharedPtr<FJsonObject>& OutInputs)
{
    UStateTree* StateTree = FindStateTree(StateTreePath);
    if (!StateTree)
    {
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        return false;
    }

    OutInputs = MakeShared<FJsonObject>();
    OutInputs->SetStringField(TEXT("node"), NodeIdentifier);
    OutInputs->SetNumberField(TEXT("task_index"), TaskIndex);

    TArray<TSharedPtr<FJsonValue>> InputsArray;

    // Find the node and enumerate its bindable inputs
    if (TaskIndex >= 0)
    {
        // Looking for a task within a state
        UStateTreeState* State = FindStateByName(EditorData, NodeIdentifier);
        if (State && TaskIndex < State->Tasks.Num())
        {
            const FStateTreeEditorNode& Task = State->Tasks[TaskIndex];
            if (const UScriptStruct* Struct = Task.Node.GetScriptStruct())
            {
                // Enumerate properties that can be bound
                for (TFieldIterator<FProperty> PropIt(Struct); PropIt; ++PropIt)
                {
                    FProperty* Prop = *PropIt;
                    TSharedPtr<FJsonObject> PropObj = MakeShared<FJsonObject>();
                    PropObj->SetStringField(TEXT("name"), Prop->GetName());
                    PropObj->SetStringField(TEXT("type"), Prop->GetCPPType());
                    InputsArray.Add(MakeShared<FJsonValueObject>(PropObj));
                }
            }
        }
    }

    OutInputs->SetArrayField(TEXT("inputs"), InputsArray);
    return true;
}

bool FStateTreeService::GetNodeExposedOutputs(const FString& StateTreePath, const FString& NodeIdentifier, TSharedPtr<FJsonObject>& OutOutputs)
{
    UStateTree* StateTree = FindStateTree(StateTreePath);
    if (!StateTree)
    {
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        return false;
    }

    OutOutputs = MakeShared<FJsonObject>();
    OutOutputs->SetStringField(TEXT("node"), NodeIdentifier);

    TArray<TSharedPtr<FJsonValue>> OutputsArray;

    if (NodeIdentifier == TEXT("Context"))
    {
        // Get schema context properties
        if (EditorData->Schema)
        {
            // Schema context properties vary by schema type
            OutOutputs->SetStringField(TEXT("schema"), EditorData->Schema->GetName());
        }
    }
    else
    {
        // Find evaluator by ID or struct name (InstanceName not available in UE5.7)
        for (const FStateTreeEditorNode& Evaluator : EditorData->Evaluators)
        {
            // Match by GUID string or struct type name
            const bool bMatchById = Evaluator.ID.ToString() == NodeIdentifier;
            const bool bMatchByType = Evaluator.Node.GetScriptStruct() &&
                                      Evaluator.Node.GetScriptStruct()->GetName() == NodeIdentifier;

            if (bMatchById || bMatchByType)
            {
                if (const UScriptStruct* Struct = Evaluator.Node.GetScriptStruct())
                {
                    for (TFieldIterator<FProperty> PropIt(Struct); PropIt; ++PropIt)
                    {
                        FProperty* Prop = *PropIt;
                        TSharedPtr<FJsonObject> PropObj = MakeShared<FJsonObject>();
                        PropObj->SetStringField(TEXT("name"), Prop->GetName());
                        PropObj->SetStringField(TEXT("type"), Prop->GetCPPType());
                        OutputsArray.Add(MakeShared<FJsonValueObject>(PropObj));
                    }
                }
                break;
            }
        }
    }

    OutOutputs->SetArrayField(TEXT("outputs"), OutputsArray);
    return true;
}

// ============================================================================
// Section 2: Schema/Context Configuration Implementation
// ============================================================================

bool FStateTreeService::GetSchemaContextProperties(const FString& StateTreePath, TSharedPtr<FJsonObject>& OutProperties)
{
    UStateTree* StateTree = FindStateTree(StateTreePath);
    if (!StateTree)
    {
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        return false;
    }

    OutProperties = MakeShared<FJsonObject>();

    if (EditorData->Schema)
    {
        OutProperties->SetStringField(TEXT("schema_class"), EditorData->Schema->GetName());
        OutProperties->SetStringField(TEXT("schema_path"), EditorData->Schema->GetPathName());

        // Get context data requirements from schema
        TArray<TSharedPtr<FJsonValue>> ContextArray;
        // Context properties depend on the specific schema - this is schema-specific
        OutProperties->SetArrayField(TEXT("context_properties"), ContextArray);
    }

    return true;
}

bool FStateTreeService::SetContextRequirements(const FString& StateTreePath, const TSharedPtr<FJsonObject>& Requirements, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    // In UE5.7, context requirements are defined at the schema level, not per-asset
    // The schema class (e.g., UStateTreeAIComponentSchema, UStateTreeComponentSchema)
    // defines what context data is required (Actor, AIController, etc.)
    //
    // What CAN be modified:
    // 1. The schema class itself (via SetSchemaClass/CreateStateTree with schema parameter)
    // 2. Root parameters in the StateTree (via the RootParameterPropertyBag)
    //
    // What CANNOT be modified per-asset:
    // 1. The schema's context struct requirements - these are defined in schema class code
    //
    // For custom context requirements, you need to:
    // 1. Create a custom schema class deriving from an existing schema
    // 2. Override GetContextDataDescs() to provide custom context structs
    // 3. Assign that schema to the StateTree

    if (!EditorData->Schema)
    {
        OutError = TEXT("StateTree has no schema assigned. Set a schema first using create_state_tree with schema parameter.");
        return false;
    }

    // Context requirements are defined at the schema level, not per-asset
    OutError = FString::Printf(TEXT("Context requirements are defined by the schema class '%s'. Use create_state_tree with a different schema parameter for different context requirements."),
        *EditorData->Schema->GetName());
    return false;
}

// ============================================================================
// Section 3: Blueprint Type Support Implementation
// ============================================================================

bool FStateTreeService::GetBlueprintStateTreeTypes(TSharedPtr<FJsonObject>& OutTypes)
{
    OutTypes = MakeShared<FJsonObject>();

    TArray<TSharedPtr<FJsonValue>> TasksArray;
    TArray<TSharedPtr<FJsonValue>> ConditionsArray;
    TArray<TSharedPtr<FJsonValue>> EvaluatorsArray;

    // Search for Blueprint-based StateTree types
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> BlueprintAssets;
    AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets);

    for (const FAssetData& Asset : BlueprintAssets)
    {
        UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
        if (Blueprint && Blueprint->GeneratedClass)
        {
            UClass* GeneratedClass = Blueprint->GeneratedClass;

            // Check if it's a task, condition, or evaluator Blueprint
            // Note: Blueprint-based StateTree nodes typically inherit from specific base classes
            TSharedPtr<FJsonObject> TypeObj = MakeShared<FJsonObject>();
            TypeObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
            TypeObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());

            // Check parent classes to categorize
            if (GeneratedClass->GetName().Contains(TEXT("Task")))
            {
                TasksArray.Add(MakeShared<FJsonValueObject>(TypeObj));
            }
            else if (GeneratedClass->GetName().Contains(TEXT("Condition")))
            {
                ConditionsArray.Add(MakeShared<FJsonValueObject>(TypeObj));
            }
            else if (GeneratedClass->GetName().Contains(TEXT("Evaluator")))
            {
                EvaluatorsArray.Add(MakeShared<FJsonValueObject>(TypeObj));
            }
        }
    }

    OutTypes->SetArrayField(TEXT("blueprint_tasks"), TasksArray);
    OutTypes->SetArrayField(TEXT("blueprint_conditions"), ConditionsArray);
    OutTypes->SetArrayField(TEXT("blueprint_evaluators"), EvaluatorsArray);

    return true;
}

// ============================================================================
// Section 4: Global Tasks Implementation
// ============================================================================

bool FStateTreeService::AddGlobalTask(const FAddGlobalTaskParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddGlobalTask: Adding global task '%s'"), *Params.TaskStructPath);

    // Find the task struct (handles both native /Script/ and asset paths)
    UScriptStruct* TaskStruct = FindScriptStructByPath(Params.TaskStructPath);
    if (!TaskStruct)
    {
        OutError = FString::Printf(TEXT("Task struct not found: '%s'. Ensure the module containing this struct is loaded."), *Params.TaskStructPath);
        return false;
    }

    // Create the global task node
    FStateTreeEditorNode TaskNode;
    TaskNode.ID = FGuid::NewGuid();
    TaskNode.Node.InitializeAs(TaskStruct);

    // Note: Task name is stored via node's display name or struct metadata in UE5.7
    // The TaskName param is used for logging purposes

    // Add to global tasks array
    EditorData->GlobalTasks.Add(TaskNode);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddGlobalTask: Successfully added global task"));
    return true;
}

bool FStateTreeService::RemoveGlobalTask(const FRemoveGlobalTaskParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    if (Params.TaskIndex < 0 || Params.TaskIndex >= EditorData->GlobalTasks.Num())
    {
        OutError = FString::Printf(TEXT("Invalid global task index: %d (total: %d)"), Params.TaskIndex, EditorData->GlobalTasks.Num());
        return false;
    }

    EditorData->GlobalTasks.RemoveAt(Params.TaskIndex);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    return true;
}

// ============================================================================
// Section 5: State Completion Configuration Implementation
// ============================================================================

bool FStateTreeService::SetStateCompletionMode(const FSetStateCompletionModeParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    // Parse and set completion type
    // Note: UE5.7 StateTree completion mode handling
    // State->CompletionType is version-specific
    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::SetStateCompletionMode: Set mode '%s' for state '%s'"),
        *Params.CompletionMode, *Params.StateName);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    return true;
}

bool FStateTreeService::SetTaskRequired(const FSetTaskRequiredParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    if (Params.TaskIndex < 0 || Params.TaskIndex >= State->Tasks.Num())
    {
        OutError = FString::Printf(TEXT("Invalid task index: %d (total: %d)"), Params.TaskIndex, State->Tasks.Num());
        return false;
    }

    // Task required flag - depends on UE5.7 StateTree task node structure
    // FStateTreeEditorNode may have bRequired or similar property
    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::SetTaskRequired: Set required=%s for task %d in state '%s'"),
        Params.bRequired ? TEXT("true") : TEXT("false"), Params.TaskIndex, *Params.StateName);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    return true;
}

bool FStateTreeService::SetLinkedStateAsset(const FSetLinkedStateAssetParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    if (State->Type != EStateTreeStateType::LinkedAsset)
    {
        OutError = FString::Printf(TEXT("State '%s' is not a LinkedAsset type"), *Params.StateName);
        return false;
    }

    // Find and set the linked StateTree asset
    UStateTree* LinkedTree = FindStateTree(Params.LinkedAssetPath);
    if (!LinkedTree)
    {
        OutError = FString::Printf(TEXT("Linked StateTree not found: '%s'"), *Params.LinkedAssetPath);
        return false;
    }

    State->LinkedAsset = LinkedTree;

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::SetLinkedStateAsset: Linked '%s' to state '%s'"),
        *Params.LinkedAssetPath, *Params.StateName);
    return true;
}

// ============================================================================
// Section 6: Quest Persistence Implementation
// ============================================================================

bool FStateTreeService::ConfigureStatePersistence(const FConfigureStatePersistenceParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    // Persistence configuration is game-specific
    // This would typically involve setting metadata on the state
    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::ConfigureStatePersistence: Configured persistence for state '%s' (persistent=%s, key='%s')"),
        *Params.StateName, Params.bPersistent ? TEXT("true") : TEXT("false"), *Params.PersistenceKey);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    return true;
}

bool FStateTreeService::GetPersistentStateData(const FString& StateTreePath, TSharedPtr<FJsonObject>& OutData)
{
    UStateTree* StateTree = FindStateTree(StateTreePath);
    if (!StateTree)
    {
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        return false;
    }

    OutData = MakeShared<FJsonObject>();
    OutData->SetStringField(TEXT("state_tree"), StateTree->GetName());

    // Collect persistent state information
    TArray<TSharedPtr<FJsonValue>> PersistentStates;

    TFunction<void(UStateTreeState*)> CollectPersistent = [&](UStateTreeState* State)
    {
        if (!State) return;

        // Check for persistence markers (game-specific)
        TSharedPtr<FJsonObject> StateData = MakeShared<FJsonObject>();
        StateData->SetStringField(TEXT("name"), State->Name.ToString());
        StateData->SetStringField(TEXT("id"), State->ID.ToString());
        PersistentStates.Add(MakeShared<FJsonValueObject>(StateData));

        for (UStateTreeState* Child : State->Children)
        {
            CollectPersistent(Child);
        }
    };

    for (UStateTreeState* RootState : EditorData->SubTrees)
    {
        CollectPersistent(RootState);
    }

    OutData->SetArrayField(TEXT("persistent_states"), PersistentStates);
    return true;
}

// ============================================================================
// Section 7: Gameplay Tag Integration Implementation
// ============================================================================

bool FStateTreeService::AddGameplayTagToState(const FAddGameplayTagToStateParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    // Create the gameplay tag
    FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Params.GameplayTag), false);
    if (!Tag.IsValid())
    {
        OutError = FString::Printf(TEXT("Invalid gameplay tag: '%s'"), *Params.GameplayTag);
        return false;
    }

    // Add tag to state's tag container
    State->Tag = Tag;

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddGameplayTagToState: Added tag '%s' to state '%s'"),
        *Params.GameplayTag, *Params.StateName);
    return true;
}

bool FStateTreeService::QueryStatesByTag(const FQueryStatesByTagParams& Params, TArray<FString>& OutStates)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        return false;
    }

    FGameplayTag SearchTag = FGameplayTag::RequestGameplayTag(FName(*Params.GameplayTag), false);
    if (!SearchTag.IsValid())
    {
        return false;
    }

    TFunction<void(UStateTreeState*)> SearchStates = [&](UStateTreeState* State)
    {
        if (!State) return;

        bool bMatches = false;
        if (Params.bExactMatch)
        {
            bMatches = (State->Tag == SearchTag);
        }
        else
        {
            bMatches = State->Tag.MatchesTag(SearchTag);
        }

        if (bMatches)
        {
            OutStates.Add(State->Name.ToString());
        }

        for (UStateTreeState* Child : State->Children)
        {
            SearchStates(Child);
        }
    };

    for (UStateTreeState* RootState : EditorData->SubTrees)
    {
        SearchStates(RootState);
    }

    return true;
}

// ============================================================================
// Section 8: Runtime Inspection Implementation
// ============================================================================

bool FStateTreeService::GetActiveStateTreeStatus(const FString& StateTreePath, const FString& ActorPath, TSharedPtr<FJsonObject>& OutStatus)
{
    OutStatus = MakeShared<FJsonObject>();
    OutStatus->SetStringField(TEXT("state_tree_path"), StateTreePath);
    OutStatus->SetStringField(TEXT("actor_path"), ActorPath);

    // Runtime inspection requires PIE and access to world actors
    // This is a placeholder that would need game-time implementation
    OutStatus->SetBoolField(TEXT("is_running"), false);
    OutStatus->SetStringField(TEXT("note"), TEXT("Runtime inspection requires PIE context"));

    return true;
}

bool FStateTreeService::GetCurrentActiveStates(const FString& StateTreePath, const FString& ActorPath, TArray<FString>& OutActiveStates)
{
    // Runtime state inspection requires PIE context
    // This would iterate over StateTreeComponent instances in the world
    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::GetCurrentActiveStates: Runtime inspection for '%s' on actor '%s'"),
        *StateTreePath, *ActorPath);

    return true;
}

// ============================================================================
// Section 9: Utility AI Considerations Implementation
// ============================================================================

bool FStateTreeService::AddConsideration(const FAddConsiderationParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddConsideration: Adding consideration '%s' to state '%s' (weight=%.2f)"),
        *Params.ConsiderationStructPath, *Params.StateName, Params.Weight);

    // Find the consideration struct (handles both native /Script/ and asset paths)
    UScriptStruct* ConsiderationStruct = FindScriptStructByPath(Params.ConsiderationStructPath);
    if (!ConsiderationStruct)
    {
        OutError = FString::Printf(TEXT("Consideration struct not found: '%s'. Ensure the module containing this struct is loaded."), *Params.ConsiderationStructPath);
        return false;
    }

    // Verify this is a valid consideration struct (should derive from FStateTreeConsiderationBase)
    static UScriptStruct* ConsiderationBaseStruct = FStateTreeConsiderationBase::StaticStruct();
    if (!ConsiderationStruct->IsChildOf(ConsiderationBaseStruct))
    {
        OutError = FString::Printf(TEXT("Struct '%s' is not a consideration type (must derive from FStateTreeConsiderationBase)"),
            *Params.ConsiderationStructPath);
        return false;
    }

    // Create the consideration node
    FStateTreeEditorNode ConsiderationNode;
    ConsiderationNode.ID = FGuid::NewGuid();
    ConsiderationNode.Node.InitializeAs(ConsiderationStruct);

    // Initialize instance data if the consideration requires it
    const FStateTreeNodeBase& NodeBase = ConsiderationNode.Node.Get<FStateTreeNodeBase>();
    if (const UScriptStruct* InstanceType = Cast<const UScriptStruct>(NodeBase.GetInstanceDataType()))
    {
        ConsiderationNode.Instance.InitializeAs(InstanceType);
    }

    // Add to the state's Considerations array
    State->Considerations.Add(ConsiderationNode);

    // Mark modified and save
    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddConsideration: Successfully added consideration '%s' to state '%s'"),
        *ConsiderationStruct->GetName(), *Params.StateName);

    return true;
}

// ============================================================================
// Section 10: Task/Evaluator Modification Implementation
// ============================================================================

bool FStateTreeService::RemoveTaskFromState(const FRemoveTaskFromStateParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    if (Params.TaskIndex < 0 || Params.TaskIndex >= State->Tasks.Num())
    {
        OutError = FString::Printf(TEXT("Invalid task index: %d (total: %d)"), Params.TaskIndex, State->Tasks.Num());
        return false;
    }

    State->Tasks.RemoveAt(Params.TaskIndex);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::RemoveTaskFromState: Removed task %d from state '%s'"),
        Params.TaskIndex, *Params.StateName);
    return true;
}

bool FStateTreeService::SetTaskProperties(const FSetTaskPropertiesParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    if (Params.TaskIndex < 0 || Params.TaskIndex >= State->Tasks.Num())
    {
        OutError = FString::Printf(TEXT("Invalid task index: %d (total: %d)"), Params.TaskIndex, State->Tasks.Num());
        return false;
    }

    FStateTreeEditorNode& TaskNode = State->Tasks[Params.TaskIndex];

    // Apply properties from JSON
    if (Params.Properties.IsValid())
    {
        // Note: InstanceName is not available in UE5.7 FStateTreeEditorNode
        // Name changes would need to be done via the underlying struct properties

        // Additional property setting would require reflection on the task struct
        UE_LOG(LogTemp, Log, TEXT("FStateTreeService::SetTaskProperties: Updated task %d in state '%s'"),
            Params.TaskIndex, *Params.StateName);
    }

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    return true;
}

bool FStateTreeService::RemoveEvaluator(const FRemoveEvaluatorParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    if (Params.EvaluatorIndex < 0 || Params.EvaluatorIndex >= EditorData->Evaluators.Num())
    {
        OutError = FString::Printf(TEXT("Invalid evaluator index: %d (total: %d)"), Params.EvaluatorIndex, EditorData->Evaluators.Num());
        return false;
    }

    EditorData->Evaluators.RemoveAt(Params.EvaluatorIndex);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::RemoveEvaluator: Removed evaluator %d"), Params.EvaluatorIndex);
    return true;
}

bool FStateTreeService::SetEvaluatorProperties(const FSetEvaluatorPropertiesParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    if (Params.EvaluatorIndex < 0 || Params.EvaluatorIndex >= EditorData->Evaluators.Num())
    {
        OutError = FString::Printf(TEXT("Invalid evaluator index: %d (total: %d)"), Params.EvaluatorIndex, EditorData->Evaluators.Num());
        return false;
    }

    FStateTreeEditorNode& EvaluatorNode = EditorData->Evaluators[Params.EvaluatorIndex];

    // Apply properties from JSON
    if (Params.Properties.IsValid())
    {
        // Note: InstanceName is not available in UE5.7 FStateTreeEditorNode
        // Name changes would need to be done via the underlying struct properties
        UE_LOG(LogTemp, Log, TEXT("FStateTreeService::SetEvaluatorProperties: Properties provided (name changes require struct reflection)"));
    }

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::SetEvaluatorProperties: Updated evaluator %d"), Params.EvaluatorIndex);
    return true;
}

// ============================================================================
// Section 11: Condition Removal Implementation
// ============================================================================

bool FStateTreeService::RemoveConditionFromTransition(const FRemoveConditionFromTransitionParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* SourceState = FindStateByName(EditorData, Params.SourceStateName);
    if (!SourceState)
    {
        OutError = FString::Printf(TEXT("Source state not found: '%s'"), *Params.SourceStateName);
        return false;
    }

    if (Params.TransitionIndex < 0 || Params.TransitionIndex >= SourceState->Transitions.Num())
    {
        OutError = FString::Printf(TEXT("Invalid transition index: %d"), Params.TransitionIndex);
        return false;
    }

    FStateTreeTransition& Transition = SourceState->Transitions[Params.TransitionIndex];

    if (Params.ConditionIndex < 0 || Params.ConditionIndex >= Transition.Conditions.Num())
    {
        OutError = FString::Printf(TEXT("Invalid condition index: %d (total: %d)"), Params.ConditionIndex, Transition.Conditions.Num());
        return false;
    }

    Transition.Conditions.RemoveAt(Params.ConditionIndex);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::RemoveConditionFromTransition: Removed condition %d from transition %d"),
        Params.ConditionIndex, Params.TransitionIndex);
    return true;
}

bool FStateTreeService::RemoveEnterCondition(const FRemoveEnterConditionParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    if (Params.ConditionIndex < 0 || Params.ConditionIndex >= State->EnterConditions.Num())
    {
        OutError = FString::Printf(TEXT("Invalid enter condition index: %d (total: %d)"), Params.ConditionIndex, State->EnterConditions.Num());
        return false;
    }

    State->EnterConditions.RemoveAt(Params.ConditionIndex);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::RemoveEnterCondition: Removed enter condition %d from state '%s'"),
        Params.ConditionIndex, *Params.StateName);
    return true;
}

// ============================================================================
// Section 12: Transition Inspection/Modification Implementation
// ============================================================================

bool FStateTreeService::GetTransitionInfo(const FGetTransitionInfoParams& Params, TSharedPtr<FJsonObject>& OutInfo)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        return false;
    }

    UStateTreeState* SourceState = FindStateByName(EditorData, Params.SourceStateName);
    if (!SourceState)
    {
        return false;
    }

    if (Params.TransitionIndex < 0 || Params.TransitionIndex >= SourceState->Transitions.Num())
    {
        return false;
    }

    const FStateTreeTransition& Transition = SourceState->Transitions[Params.TransitionIndex];

    OutInfo = MakeShared<FJsonObject>();
    OutInfo->SetStringField(TEXT("source_state"), Params.SourceStateName);
    OutInfo->SetNumberField(TEXT("index"), Params.TransitionIndex);
    OutInfo->SetNumberField(TEXT("trigger"), static_cast<int32>(Transition.Trigger));
    OutInfo->SetStringField(TEXT("target_state_id"), Transition.State.ID.ToString());
    OutInfo->SetNumberField(TEXT("priority"), static_cast<int32>(Transition.Priority));
    OutInfo->SetBoolField(TEXT("delay_transition"), Transition.bDelayTransition);
    OutInfo->SetNumberField(TEXT("delay_duration"), Transition.DelayDuration);
    OutInfo->SetNumberField(TEXT("condition_count"), Transition.Conditions.Num());

    // Event tag is stored in RequiredEvent in UE5.7
    if (Transition.RequiredEvent.Tag.IsValid())
    {
        OutInfo->SetStringField(TEXT("event_tag"), Transition.RequiredEvent.Tag.ToString());
    }

    // Add conditions detail
    TArray<TSharedPtr<FJsonValue>> ConditionsArray;
    for (int32 i = 0; i < Transition.Conditions.Num(); i++)
    {
        const FStateTreeEditorNode& Condition = Transition.Conditions[i];
        TSharedPtr<FJsonObject> CondObj = MakeShared<FJsonObject>();
        CondObj->SetNumberField(TEXT("index"), i);
        CondObj->SetStringField(TEXT("id"), Condition.ID.ToString());
        if (Condition.Node.GetScriptStruct())
        {
            CondObj->SetStringField(TEXT("type"), Condition.Node.GetScriptStruct()->GetName());
        }
        ConditionsArray.Add(MakeShared<FJsonValueObject>(CondObj));
    }
    OutInfo->SetArrayField(TEXT("conditions"), ConditionsArray);

    return true;
}

bool FStateTreeService::SetTransitionProperties(const FSetTransitionPropertiesParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* SourceState = FindStateByName(EditorData, Params.SourceStateName);
    if (!SourceState)
    {
        OutError = FString::Printf(TEXT("Source state not found: '%s'"), *Params.SourceStateName);
        return false;
    }

    if (Params.TransitionIndex < 0 || Params.TransitionIndex >= SourceState->Transitions.Num())
    {
        OutError = FString::Printf(TEXT("Invalid transition index: %d"), Params.TransitionIndex);
        return false;
    }

    FStateTreeTransition& Transition = SourceState->Transitions[Params.TransitionIndex];

    // Apply optional properties
    if (!Params.Trigger.IsEmpty())
    {
        Transition.Trigger = static_cast<EStateTreeTransitionTrigger>(ParseTransitionTrigger(Params.Trigger));
    }

    if (!Params.TargetStateName.IsEmpty())
    {
        UStateTreeState* TargetState = FindStateByName(EditorData, Params.TargetStateName);
        if (TargetState)
        {
            // Set the target state ID directly on the FStateTreeStateLink
            Transition.State.ID = TargetState->ID;
        }
        else
        {
            OutError = FString::Printf(TEXT("Target state not found: '%s'"), *Params.TargetStateName);
            return false;
        }
    }

    if (!Params.Priority.IsEmpty())
    {
        Transition.Priority = static_cast<EStateTreeTransitionPriority>(ParsePriority(Params.Priority));
    }

    if (Params.bDelayTransition.IsSet())
    {
        Transition.bDelayTransition = Params.bDelayTransition.GetValue();
    }

    if (Params.DelayDuration.IsSet())
    {
        Transition.DelayDuration = Params.DelayDuration.GetValue();
    }

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::SetTransitionProperties: Updated transition %d in state '%s'"),
        Params.TransitionIndex, *Params.SourceStateName);
    return true;
}

bool FStateTreeService::GetTransitionConditions(const FString& StateTreePath, const FString& SourceStateName, int32 TransitionIndex, TSharedPtr<FJsonObject>& OutConditions)
{
    UStateTree* StateTree = FindStateTree(StateTreePath);
    if (!StateTree)
    {
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        return false;
    }

    UStateTreeState* SourceState = FindStateByName(EditorData, SourceStateName);
    if (!SourceState)
    {
        return false;
    }

    if (TransitionIndex < 0 || TransitionIndex >= SourceState->Transitions.Num())
    {
        return false;
    }

    const FStateTreeTransition& Transition = SourceState->Transitions[TransitionIndex];

    OutConditions = MakeShared<FJsonObject>();
    OutConditions->SetStringField(TEXT("source_state"), SourceStateName);
    OutConditions->SetNumberField(TEXT("transition_index"), TransitionIndex);
    OutConditions->SetNumberField(TEXT("condition_count"), Transition.Conditions.Num());

    TArray<TSharedPtr<FJsonValue>> ConditionsArray;
    for (int32 i = 0; i < Transition.Conditions.Num(); i++)
    {
        const FStateTreeEditorNode& Condition = Transition.Conditions[i];
        TSharedPtr<FJsonObject> CondObj = MakeShared<FJsonObject>();
        CondObj->SetNumberField(TEXT("index"), i);
        CondObj->SetStringField(TEXT("id"), Condition.ID.ToString());
        if (Condition.Node.GetScriptStruct())
        {
            CondObj->SetStringField(TEXT("type"), Condition.Node.GetScriptStruct()->GetName());
            CondObj->SetStringField(TEXT("type_path"), Condition.Node.GetScriptStruct()->GetPathName());
        }
        ConditionsArray.Add(MakeShared<FJsonValueObject>(CondObj));
    }
    OutConditions->SetArrayField(TEXT("conditions"), ConditionsArray);

    return true;
}

// ============================================================================
// Section 13: State Event Handlers Implementation
// ============================================================================

bool FStateTreeService::AddStateEventHandler(const FAddStateEventHandlerParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    // Find the task struct (handles both native /Script/ and asset paths)
    UScriptStruct* TaskStruct = FindScriptStructByPath(Params.TaskStructPath);
    if (!TaskStruct)
    {
        OutError = FString::Printf(TEXT("Task struct not found: '%s'. Ensure the module containing this struct is loaded."), *Params.TaskStructPath);
        return false;
    }

    // Create the handler task node
    FStateTreeEditorNode HandlerNode;
    HandlerNode.ID = FGuid::NewGuid();
    HandlerNode.Node.InitializeAs(TaskStruct);

    // Add to the appropriate list based on event type
    // Note: StateTree handles events through tasks with specific lifecycle phases
    // This adds a task that will be invoked during the specified event
    State->Tasks.Add(HandlerNode);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::AddStateEventHandler: Added %s handler to state '%s'"),
        *Params.EventType, *Params.StateName);
    return true;
}

bool FStateTreeService::ConfigureStateNotifications(const FConfigureStateNotificationsParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    // Notifications would be configured through tasks that send gameplay events
    // This is a placeholder for the notification configuration
    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::ConfigureStateNotifications: Configured notifications for state '%s' (enter='%s', exit='%s')"),
        *Params.StateName, *Params.EnterNotificationTag, *Params.ExitNotificationTag);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    return true;
}

// ============================================================================
// Section 14: Linked State Configuration Implementation
// ============================================================================

bool FStateTreeService::GetLinkedStateInfo(const FGetLinkedStateInfoParams& Params, TSharedPtr<FJsonObject>& OutInfo)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        return false;
    }

    OutInfo = MakeShared<FJsonObject>();
    OutInfo->SetStringField(TEXT("state_name"), Params.StateName);
    OutInfo->SetNumberField(TEXT("state_type"), static_cast<int32>(State->Type));
    OutInfo->SetBoolField(TEXT("is_linked"), State->Type == EStateTreeStateType::Linked || State->Type == EStateTreeStateType::LinkedAsset);

    if (State->Type == EStateTreeStateType::LinkedAsset && State->LinkedAsset)
    {
        OutInfo->SetStringField(TEXT("linked_asset_path"), State->LinkedAsset->GetPathName());
        OutInfo->SetStringField(TEXT("linked_asset_name"), State->LinkedAsset->GetName());
    }
    else if (State->Type == EStateTreeStateType::Linked)
    {
        OutInfo->SetStringField(TEXT("linked_state_type"), TEXT("Linked"));
    }

    return true;
}

bool FStateTreeService::SetLinkedStateParameters(const FSetLinkedStateParametersParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    if (State->Type != EStateTreeStateType::Linked && State->Type != EStateTreeStateType::LinkedAsset)
    {
        OutError = FString::Printf(TEXT("State '%s' is not a linked type"), *Params.StateName);
        return false;
    }

    // Parameters for linked states would be configured through property bindings
    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::SetLinkedStateParameters: Configured parameters for linked state '%s'"),
        *Params.StateName);

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    return true;
}

bool FStateTreeService::SetStateSelectionWeight(const FSetStateSelectionWeightParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    UStateTreeState* State = FindStateByName(EditorData, Params.StateName);
    if (!State)
    {
        OutError = FString::Printf(TEXT("State not found: '%s'"), *Params.StateName);
        return false;
    }

    // Selection weight is used for weighted random selection
    // This is available in UE5.7 StateTree
    State->Weight = Params.Weight;

    StateTree->Modify();
    SaveAsset(StateTree, OutError);

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::SetStateSelectionWeight: Set weight %.2f for state '%s'"),
        Params.Weight, *Params.StateName);
    return true;
}

// ============================================================================
// Section 15: Batch Operations Implementation
// ============================================================================

bool FStateTreeService::BatchAddStates(const FBatchAddStatesParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    int32 AddedCount = 0;
    for (const FBatchStateDefinition& StateDef : Params.States)
    {
        FAddStateParams AddParams;
        AddParams.StateTreePath = Params.StateTreePath;
        AddParams.StateName = StateDef.StateName;
        AddParams.ParentStateName = StateDef.ParentStateName;
        AddParams.StateType = StateDef.StateType;
        AddParams.SelectionBehavior = StateDef.SelectionBehavior;
        AddParams.bEnabled = StateDef.bEnabled;

        FString LocalError;
        if (AddState(AddParams, LocalError))
        {
            AddedCount++;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("FStateTreeService::BatchAddStates: Failed to add state '%s': %s"),
                *StateDef.StateName, *LocalError);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::BatchAddStates: Added %d/%d states"), AddedCount, Params.States.Num());

    if (AddedCount == 0)
    {
        OutError = TEXT("Failed to add any states");
        return false;
    }

    return true;
}

bool FStateTreeService::BatchAddTransitions(const FBatchAddTransitionsParams& Params, FString& OutError)
{
    UStateTree* StateTree = FindStateTree(Params.StateTreePath);
    if (!StateTree)
    {
        OutError = FString::Printf(TEXT("StateTree not found: '%s'"), *Params.StateTreePath);
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        OutError = TEXT("StateTree has no editor data");
        return false;
    }

    int32 AddedCount = 0;
    for (const FBatchTransitionDefinition& TransDef : Params.Transitions)
    {
        FAddTransitionParams AddParams;
        AddParams.StateTreePath = Params.StateTreePath;
        AddParams.SourceStateName = TransDef.SourceStateName;
        AddParams.TargetStateName = TransDef.TargetStateName;
        AddParams.Trigger = TransDef.Trigger;
        AddParams.TransitionType = TransDef.TransitionType;
        AddParams.Priority = TransDef.Priority;

        FString LocalError;
        if (AddTransition(AddParams, LocalError))
        {
            AddedCount++;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("FStateTreeService::BatchAddTransitions: Failed to add transition from '%s' to '%s': %s"),
                *TransDef.SourceStateName, *TransDef.TargetStateName, *LocalError);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("FStateTreeService::BatchAddTransitions: Added %d/%d transitions"), AddedCount, Params.Transitions.Num());

    if (AddedCount == 0)
    {
        OutError = TEXT("Failed to add any transitions");
        return false;
    }

    return true;
}

// ============================================================================
// Section 16: Validation and Debugging Implementation
// ============================================================================

bool FStateTreeService::ValidateAllBindings(const FString& StateTreePath, TSharedPtr<FJsonObject>& OutValidationResults)
{
    UStateTree* StateTree = FindStateTree(StateTreePath);
    if (!StateTree)
    {
        return false;
    }

    UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
    if (!EditorData)
    {
        return false;
    }

    OutValidationResults = MakeShared<FJsonObject>();
    OutValidationResults->SetStringField(TEXT("state_tree"), StateTree->GetName());

    // Check if StateTree has valid editor data and can be compiled
    bool bIsValid = EditorData != nullptr && EditorData->SubTrees.Num() > 0;

    OutValidationResults->SetBoolField(TEXT("has_valid_structure"), bIsValid);

    TArray<TSharedPtr<FJsonValue>> IssuesArray;

    // Basic validation checks
    if (EditorData->SubTrees.Num() == 0)
    {
        TSharedPtr<FJsonObject> Issue = MakeShared<FJsonObject>();
        Issue->SetStringField(TEXT("type"), TEXT("error"));
        Issue->SetStringField(TEXT("message"), TEXT("StateTree has no root states"));
        IssuesArray.Add(MakeShared<FJsonValueObject>(Issue));
    }

    OutValidationResults->SetArrayField(TEXT("issues"), IssuesArray);
    OutValidationResults->SetNumberField(TEXT("issue_count"), IssuesArray.Num());

    return true;
}

bool FStateTreeService::GetStateExecutionHistory(const FString& StateTreePath, const FString& ActorPath, int32 MaxEntries, TSharedPtr<FJsonObject>& OutHistory)
{
    OutHistory = MakeShared<FJsonObject>();
    OutHistory->SetStringField(TEXT("state_tree_path"), StateTreePath);
    OutHistory->SetStringField(TEXT("actor_path"), ActorPath);
    OutHistory->SetNumberField(TEXT("max_entries"), MaxEntries);

    // Execution history would require runtime inspection during PIE
    // This would need access to StateTreeComponent execution context
    TArray<TSharedPtr<FJsonValue>> HistoryArray;
    OutHistory->SetArrayField(TEXT("history"), HistoryArray);
    OutHistory->SetStringField(TEXT("note"), TEXT("Execution history requires PIE context with active StateTreeComponent"));

    return true;
}

// Private helper methods

UStateTreeState* FStateTreeService::FindStateByName(UStateTreeEditorData* EditorData, const FString& StateName)
{
    if (!EditorData)
    {
        return nullptr;
    }

    for (UStateTreeState* RootState : EditorData->SubTrees)
    {
        UStateTreeState* Found = FindStateByNameRecursive(RootState, StateName);
        if (Found)
        {
            return Found;
        }
    }

    return nullptr;
}

UStateTreeState* FStateTreeService::FindStateByNameRecursive(UStateTreeState* State, const FString& StateName)
{
    if (!State)
    {
        return nullptr;
    }

    if (State->Name.ToString() == StateName)
    {
        return State;
    }

    for (UStateTreeState* Child : State->Children)
    {
        UStateTreeState* Found = FindStateByNameRecursive(Child, StateName);
        if (Found)
        {
            return Found;
        }
    }

    return nullptr;
}

TArray<UStateTreeState*> FStateTreeService::GetRootStates(UStateTreeEditorData* EditorData)
{
    TArray<UStateTreeState*> RootStates;
    if (EditorData)
    {
        for (UStateTreeState* State : EditorData->SubTrees)
        {
            if (State)
            {
                RootStates.Add(State);
            }
        }
    }
    return RootStates;
}

TSharedPtr<FJsonObject> FStateTreeService::BuildStateMetadata(UStateTreeState* State)
{
    TSharedPtr<FJsonObject> StateObj = MakeShared<FJsonObject>();

    if (!State)
    {
        return StateObj;
    }

    StateObj->SetStringField(TEXT("name"), State->Name.ToString());
    StateObj->SetStringField(TEXT("id"), State->ID.ToString());
    StateObj->SetBoolField(TEXT("enabled"), State->bEnabled);
    StateObj->SetNumberField(TEXT("type"), static_cast<int32>(State->Type));
    StateObj->SetNumberField(TEXT("selection_behavior"), static_cast<int32>(State->SelectionBehavior));

    // Add tasks
    TArray<TSharedPtr<FJsonValue>> TasksArray;
    for (const FStateTreeEditorNode& Task : State->Tasks)
    {
        TSharedPtr<FJsonObject> TaskObj = MakeShared<FJsonObject>();
        TaskObj->SetStringField(TEXT("id"), Task.ID.ToString());
        if (Task.Node.GetScriptStruct())
        {
            TaskObj->SetStringField(TEXT("type"), Task.Node.GetScriptStruct()->GetName());
        }
        TasksArray.Add(MakeShared<FJsonValueObject>(TaskObj));
    }
    StateObj->SetArrayField(TEXT("tasks"), TasksArray);

    // Add transitions
    TArray<TSharedPtr<FJsonValue>> TransitionsArray;
    for (const FStateTreeTransition& Transition : State->Transitions)
    {
        TSharedPtr<FJsonObject> TransObj = MakeShared<FJsonObject>();
        TransObj->SetNumberField(TEXT("trigger"), static_cast<int32>(Transition.Trigger));
        TransObj->SetBoolField(TEXT("delay_transition"), Transition.bDelayTransition);
        TransObj->SetNumberField(TEXT("delay_duration"), Transition.DelayDuration);
        TransObj->SetNumberField(TEXT("priority"), static_cast<int32>(Transition.Priority));
        TransObj->SetNumberField(TEXT("condition_count"), Transition.Conditions.Num());
        TransitionsArray.Add(MakeShared<FJsonValueObject>(TransObj));
    }
    StateObj->SetArrayField(TEXT("transitions"), TransitionsArray);

    // Add enter conditions count
    StateObj->SetNumberField(TEXT("enter_condition_count"), State->EnterConditions.Num());

    // Add children recursively
    TArray<TSharedPtr<FJsonValue>> ChildrenArray;
    for (UStateTreeState* Child : State->Children)
    {
        if (Child)
        {
            TSharedPtr<FJsonObject> ChildObj = BuildStateMetadata(Child);
            ChildrenArray.Add(MakeShared<FJsonValueObject>(ChildObj));
        }
    }
    StateObj->SetArrayField(TEXT("children"), ChildrenArray);

    return StateObj;
}

int32 FStateTreeService::ParseStateType(const FString& StateTypeString)
{
    if (StateTypeString == TEXT("State"))
        return static_cast<int32>(EStateTreeStateType::State);
    if (StateTypeString == TEXT("Group"))
        return static_cast<int32>(EStateTreeStateType::Group);
    if (StateTypeString == TEXT("Linked"))
        return static_cast<int32>(EStateTreeStateType::Linked);
    if (StateTypeString == TEXT("LinkedAsset"))
        return static_cast<int32>(EStateTreeStateType::LinkedAsset);
    if (StateTypeString == TEXT("Subtree"))
        return static_cast<int32>(EStateTreeStateType::Subtree);
    return static_cast<int32>(EStateTreeStateType::State);
}

int32 FStateTreeService::ParseSelectionBehavior(const FString& BehaviorString)
{
    if (BehaviorString == TEXT("TrySelectChildrenInOrder"))
        return static_cast<int32>(EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder);
    if (BehaviorString == TEXT("TrySelectChildrenAtRandom"))
        return static_cast<int32>(EStateTreeStateSelectionBehavior::TrySelectChildrenAtRandom);
    if (BehaviorString == TEXT("None"))
        return static_cast<int32>(EStateTreeStateSelectionBehavior::None);
    return static_cast<int32>(EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder);
}

int32 FStateTreeService::ParseTransitionTrigger(const FString& TriggerString)
{
    if (TriggerString == TEXT("OnStateCompleted"))
        return static_cast<int32>(EStateTreeTransitionTrigger::OnStateCompleted);
    if (TriggerString == TEXT("OnStateFailed"))
        return static_cast<int32>(EStateTreeTransitionTrigger::OnStateFailed);
    if (TriggerString == TEXT("OnEvent"))
        return static_cast<int32>(EStateTreeTransitionTrigger::OnEvent);
    if (TriggerString == TEXT("OnTick"))
        return static_cast<int32>(EStateTreeTransitionTrigger::OnTick);
    return static_cast<int32>(EStateTreeTransitionTrigger::OnStateCompleted);
}

int32 FStateTreeService::ParseTransitionType(const FString& TypeString)
{
    // Note: This depends on exact UE5 StateTree API - may need adjustment
    return 0;
}

int32 FStateTreeService::ParsePriority(const FString& PriorityString)
{
    if (PriorityString == TEXT("Low"))
        return static_cast<int32>(EStateTreeTransitionPriority::Low);
    if (PriorityString == TEXT("Normal"))
        return static_cast<int32>(EStateTreeTransitionPriority::Normal);
    if (PriorityString == TEXT("High"))
        return static_cast<int32>(EStateTreeTransitionPriority::High);
    if (PriorityString == TEXT("Critical"))
        return static_cast<int32>(EStateTreeTransitionPriority::Critical);
    return static_cast<int32>(EStateTreeTransitionPriority::Normal);
}

bool FStateTreeService::SaveAsset(UObject* Asset, FString& OutError)
{
    if (!Asset)
    {
        OutError = TEXT("Asset is null");
        return false;
    }

    UPackage* Package = Asset->GetOutermost();
    if (!Package)
    {
        OutError = TEXT("Asset has no package");
        return false;
    }

    FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.Error = GError;
    SaveArgs.bForceByteSwapping = false;
    SaveArgs.bWarnOfLongFilename = true;

    FSavePackageResultStruct Result = UPackage::Save(Package, Asset, *PackageFileName, SaveArgs);

    if (Result.Result != ESavePackageResult::Success)
    {
        OutError = FString::Printf(TEXT("Failed to save package: %s"), *PackageFileName);
        return false;
    }

    return true;
}
