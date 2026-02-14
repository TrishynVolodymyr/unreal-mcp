#include "Services/BlueprintService.h"
#include "Services/PropertyTypeResolverService.h"
#include "Services/Blueprint/BlueprintCacheService.h"
#include "Services/Blueprint/BlueprintCreationService.h"
#include "Services/Blueprint/BlueprintPropertyService.h"
#include "Services/Blueprint/BlueprintFunctionService.h"
#include "Services/ComponentService.h"
#include "Services/PropertyService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"
#include "K2Node_FunctionEntry.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node_FunctionResult.h"
#include "Engine/UserDefinedStruct.h"
#include "EdGraphSchema_K2.h"
#include "Logging/TokenizedMessage.h"
#include "Logging/MessageLog.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Engine/BlueprintCore.h"
#include "UObject/StructOnScope.h"
#include "Engine/Engine.h"

// Blueprint Service Implementation
FBlueprintService::FBlueprintService()
    : CreationService(MakeUnique<FBlueprintCreationService>())
    , PropertyService(MakeUnique<FBlueprintPropertyService>())
    , FunctionService(MakeUnique<FBlueprintFunctionService>())
{
}

FBlueprintService::~FBlueprintService()
{
}

FBlueprintService& FBlueprintService::Get()
{
    static FBlueprintService Instance;
    return Instance;
}

UBlueprint* FBlueprintService::CreateBlueprint(const FBlueprintCreationParams& Params)
{
    // Delegate to CreationService, passing CompileBlueprint as a lambda
    return CreationService->CreateBlueprint(
        Params,
        BlueprintCache,
        [this](UBlueprint* Blueprint, FString& OutError) -> bool {
            return this->CompileBlueprint(Blueprint, OutError);
        }
    );
}

bool FBlueprintService::AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params, FString& OutErrorMessage)
{
    // Delegate to ComponentService for component operations
    bool bResult = FComponentService::Get().AddComponentToBlueprint(Blueprint, Params, OutErrorMessage);
    
    if (bResult)
    {
        // Invalidate cache since blueprint was modified
        BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    }
    
    return bResult;
}

bool FBlueprintService::CompileBlueprint(UBlueprint* Blueprint, FString& OutError)
{
    if (!Blueprint)
    {
        OutError = TEXT("Invalid blueprint");
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CompileBlueprint: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CompileBlueprint: Compiling blueprint '%s'"), *Blueprint->GetName());
    
    // Store pre-compilation status for comparison
    EBlueprintStatus PreCompileStatus = Blueprint->Status;
    
    // Clear any existing compilation errors and reset status
    Blueprint->Status = BS_Unknown;
    
    // Clear any existing compilation state
    Blueprint->bIsRegeneratingOnLoad = false;
    
    // Create a compiler results log to capture detailed compilation information
    FCompilerResultsLog CompilerLog(true);  // Enable event compatibility
    CompilerLog.bLogDetailedResults = true;  // Enable detailed logging
    CompilerLog.bSilentMode = false;         // We want to collect all messages
    CompilerLog.bAnnotateMentionedNodes = true; // Annotate nodes with errors
    
    // Compile the blueprint with detailed logging (use different signature)
    FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompilerLog);
    
    // Log the compilation status for debugging
    FString StatusName;
    switch (Blueprint->Status)
    {
        case BS_Unknown: StatusName = TEXT("BS_Unknown"); break;
        case BS_Dirty: StatusName = TEXT("BS_Dirty"); break;
        case BS_Error: StatusName = TEXT("BS_Error"); break;
        case BS_UpToDate: StatusName = TEXT("BS_UpToDate"); break;
        case BS_BeingCreated: StatusName = TEXT("BS_BeingCreated"); break;
        case BS_UpToDateWithWarnings: StatusName = TEXT("BS_UpToDateWithWarnings"); break;
        default: StatusName = FString::Printf(TEXT("Unknown(%d)"), (int32)Blueprint->Status); break;
    }
    UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CompileBlueprint: Post-compilation status: %s (%d)"), *StatusName, (int32)Blueprint->Status);
    
    // Check compilation result and extract detailed error information from CompilerLog
    if (Blueprint->Status == BS_Error || CompilerLog.NumErrors > 0)
    {
        TArray<FString> DetailedErrors;
        
        // Extract ALL error messages from the FCompilerResultsLog
        for (const TSharedRef<FTokenizedMessage>& Message : CompilerLog.Messages)
        {
            EMessageSeverity::Type Severity = Message->GetSeverity();
            FString MessageText = Message->ToText().ToString();
            
            if (!MessageText.IsEmpty() && Severity == EMessageSeverity::Error)
            {
                DetailedErrors.Add(MessageText);
            }
        }
        
        // If no specific errors found in the log, add basic error information
        if (DetailedErrors.IsEmpty())
        {
            DetailedErrors.Add(FString::Printf(TEXT("Blueprint '%s' failed to compile (status: %s)"), 
                *Blueprint->GetName(), *StatusName));
            
            // Check for common issues
            if (!Blueprint->ParentClass)
            {
                DetailedErrors.Add(TEXT("Missing parent class"));
            }
            
            if (Blueprint->UbergraphPages.Num() == 0 && Blueprint->FunctionGraphs.Num() == 0)
            {
                DetailedErrors.Add(TEXT("Blueprint has no graphs"));
            }
            
            // Check for node connection errors in all graphs
            TArray<UEdGraph*> AllGraphs;
            Blueprint->GetAllGraphs(AllGraphs);
            
            for (UEdGraph* Graph : AllGraphs)
            {
                if (Graph)
                {
                    for (UEdGraphNode* Node : Graph->Nodes)
                    {
                        if (Node)
                        {
                            // Check if node has validation errors
                            if (!Node->IsNodeEnabled() || Node->HasAnyFlags(RF_Transient))
                            {
                                DetailedErrors.Add(FString::Printf(TEXT("Node '%s' in graph '%s' has validation issues"), 
                                    *Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString(),
                                    *Graph->GetFName().ToString()));
                            }
                            
                            // Check for unconnected pins that should be connected
                            for (UEdGraphPin* Pin : Node->Pins)
                            {
                                if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && 
                                    Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() == 0)
                                {
                                    // This is likely an unconnected execution output
                                    DetailedErrors.Add(FString::Printf(TEXT("Unconnected execution pin '%s' on node '%s' in graph '%s'"), 
                                        *Pin->PinName.ToString(),
                                        *Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString(),
                                        *Graph->GetFName().ToString()));
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Create comprehensive error message (use space instead of newlines for JSON compatibility)
        FString DetailedErrorInfo = FString::Printf(
            TEXT("Blueprint compilation failed: %d error(s), %d warning(s). Errors: %s"), 
            CompilerLog.NumErrors,
            CompilerLog.NumWarnings,
            *FString::Join(DetailedErrors, TEXT(" | "))
        );
        
        OutError = DetailedErrorInfo;
        
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::CompileBlueprint: %s"), *OutError);
        
        return false;
    }
    else if (Blueprint->Status == BS_UpToDateWithWarnings || CompilerLog.NumWarnings > 0)
    {
        TArray<FString> WarningMessages;
        
        // Extract warning messages from the FCompilerResultsLog
        for (const TSharedRef<FTokenizedMessage>& Message : CompilerLog.Messages)
        {
            if (Message->GetSeverity() == EMessageSeverity::Warning)
            {
                FString WarningText = Message->ToText().ToString();
                if (!WarningText.IsEmpty())
                {
                    WarningMessages.Add(WarningText);
                }
            }
        }
        
        if (WarningMessages.Num() > 0)
        {
            OutError = FString::Printf(TEXT("Blueprint '%s' compiled with %d warning(s): %s"), 
                *Blueprint->GetName(), 
                CompilerLog.NumWarnings,
                *FString::Join(WarningMessages, TEXT(" | ")));
        }
        else
        {
            OutError = FString::Printf(TEXT("Blueprint '%s' compiled with %d warning(s)"), 
                *Blueprint->GetName(), 
                CompilerLog.NumWarnings);
        }
        
        UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::CompileBlueprint: Blueprint '%s' compiled with warnings: %s"), 
            *Blueprint->GetName(), *OutError);
    }
    
    // Invalidate cache since blueprint was modified
    BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::CompileBlueprint: Successfully compiled blueprint '%s'"), *Blueprint->GetName());
    return true;
}

UBlueprint* FBlueprintService::FindBlueprint(const FString& BlueprintName)
{
    UE_LOG(LogTemp, Verbose, TEXT("FBlueprintService::FindBlueprint: Looking for blueprint '%s'"), *BlueprintName);
    
    // Check cache first
    UBlueprint* CachedBlueprint = BlueprintCache.GetBlueprint(BlueprintName);
    if (CachedBlueprint)
    {
        UE_LOG(LogTemp, Verbose, TEXT("FBlueprintService::FindBlueprint: Found blueprint '%s' in cache"), *BlueprintName);
        return CachedBlueprint;
    }
    
    // Use common utils to find blueprint
    UBlueprint* FoundBlueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (FoundBlueprint)
    {
        // Cache for future lookups
        BlueprintCache.CacheBlueprint(BlueprintName, FoundBlueprint);
        UE_LOG(LogTemp, Verbose, TEXT("FBlueprintService::FindBlueprint: Found and cached blueprint '%s'"), *BlueprintName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::FindBlueprint: Blueprint '%s' not found"), *BlueprintName);
    }
    
    return FoundBlueprint;
}

bool FBlueprintService::AddVariableToBlueprint(UBlueprint* Blueprint, const FString& VariableName, const FString& VariableType, bool bIsExposed)
{
    return PropertyService->AddVariableToBlueprint(Blueprint, VariableName, VariableType, bIsExposed, BlueprintCache);
}

bool FBlueprintService::SetBlueprintProperty(UBlueprint* Blueprint, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutErrorMessage)
{
    return PropertyService->SetBlueprintProperty(Blueprint, PropertyName, PropertyValue, OutErrorMessage, BlueprintCache);
}

bool FBlueprintService::SetPhysicsProperties(UBlueprint* Blueprint, const FString& ComponentName, const TMap<FString, float>& PhysicsParams)
{
    return PropertyService->SetPhysicsProperties(Blueprint, ComponentName, PhysicsParams, BlueprintCache);
}

bool FBlueprintService::GetBlueprintComponents(UBlueprint* Blueprint, TArray<TPair<FString, FString>>& OutComponents)
{
    return PropertyService->GetBlueprintComponents(Blueprint, OutComponents);
}

bool FBlueprintService::SetStaticMeshProperties(UBlueprint* Blueprint, const FString& ComponentName, const FString& StaticMeshPath)
{
    return PropertyService->SetStaticMeshProperties(Blueprint, ComponentName, StaticMeshPath, BlueprintCache);
}

bool FBlueprintService::SetPawnProperties(UBlueprint* Blueprint, const TMap<FString, FString>& PawnParams)
{
    return PropertyService->SetPawnProperties(Blueprint, PawnParams, BlueprintCache);
}

bool FBlueprintService::AddInterfaceToBlueprint(UBlueprint* Blueprint, const FString& InterfaceName)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::AddInterfaceToBlueprint: Invalid blueprint"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::AddInterfaceToBlueprint: Adding interface '%s' to blueprint '%s'"), 
        *InterfaceName, *Blueprint->GetName());
    
    // Find the interface
    UBlueprint* InterfaceBlueprint = FindBlueprint(InterfaceName);
    if (!InterfaceBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::AddInterfaceToBlueprint: Interface blueprint not found: %s"), *InterfaceName);
        return false;
    }
    
    if (InterfaceBlueprint->BlueprintType != BPTYPE_Interface)
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintService::AddInterfaceToBlueprint: Blueprint '%s' is not an interface"), *InterfaceName);
        return false;
    }
    
    // Add interface to blueprint
    FTopLevelAssetPath InterfacePath(InterfaceBlueprint->GeneratedClass);
    FBlueprintEditorUtils::ImplementNewInterface(Blueprint, InterfacePath);
    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // Invalidate cache since blueprint was modified
    BlueprintCache.InvalidateBlueprint(Blueprint->GetName());
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintService::AddInterfaceToBlueprint: Successfully added interface '%s'"), *InterfaceName);
    return true;
}

UBlueprint* FBlueprintService::CreateBlueprintInterface(const FString& InterfaceName, const FString& FolderPath)
{
    return CreationService->CreateBlueprintInterface(InterfaceName, FolderPath, BlueprintCache);
}

bool FBlueprintService::CreateCustomBlueprintFunction(UBlueprint* Blueprint, const FString& FunctionName, const TSharedPtr<FJsonObject>& FunctionParams)
{
    return FunctionService->CreateCustomBlueprintFunction(
        Blueprint,
        FunctionName,
        FunctionParams,
        BlueprintCache,
        [this](const FString& TypeString, FEdGraphPinType& OutPinType) -> bool {
            return this->ConvertStringToPinType(TypeString, OutPinType);
        }
    );
}

bool FBlueprintService::SpawnBlueprintActor(UBlueprint* Blueprint, const FString& ActorName, const FVector& Location, const FRotator& Rotation)
{
    return FunctionService->SpawnBlueprintActor(Blueprint, ActorName, Location, Rotation);
}

bool FBlueprintService::CallBlueprintFunction(UBlueprint* Blueprint, const FString& FunctionName, const TArray<FString>& Parameters)
{
    return FunctionService->CallBlueprintFunction(Blueprint, FunctionName, Parameters);
}

bool FBlueprintService::ConvertStringToPinType(const FString& TypeString, FEdGraphPinType& OutPinType) const
{
    // Clear the output pin type
    OutPinType = FEdGraphPinType();
    
    // Handle basic types
    if (TypeString == TEXT("Boolean") || TypeString == TEXT("Bool"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        return true;
    }
    else if (TypeString == TEXT("Integer") || TypeString == TEXT("Int") || TypeString == TEXT("Int32"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
        return true;
    }
    else if (TypeString == TEXT("Float"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Real;
        OutPinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
        return true;
    }
    else if (TypeString == TEXT("String"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
        return true;
    }
    else if (TypeString == TEXT("Text"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
        return true;
    }
    else if (TypeString == TEXT("Name"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Name;
        return true;
    }
    else if (TypeString == TEXT("Vector"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
        return true;
    }
    else if (TypeString == TEXT("Vector2D"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FVector2D>::Get();
        return true;
    }
    else if (TypeString == TEXT("Rotator"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
        return true;
    }
    else if (TypeString == TEXT("Transform"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
        return true;
    }
    else if (TypeString == TEXT("Color"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FColor>::Get();
        return true;
    }
    else if (TypeString == TEXT("LinearColor"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
        return true;
    }
    else if (TypeString == TEXT("Byte"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
        return true;
    }
    else if (TypeString == TEXT("Object"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        OutPinType.PinSubCategoryObject = UObject::StaticClass();
        return true;
    }
    else if (TypeString == TEXT("Actor"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        OutPinType.PinSubCategoryObject = AActor::StaticClass();
        return true;
    }
    else if (TypeString == TEXT("Pawn"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        OutPinType.PinSubCategoryObject = APawn::StaticClass();
        return true;
    }
    else if (TypeString == TEXT("PlayerController"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        OutPinType.PinSubCategoryObject = APlayerController::StaticClass();
        return true;
    }
    
    // Handle array types (e.g., "String[]", "Integer[]")
    if (TypeString.EndsWith(TEXT("[]")))
    {
        FString BaseType = TypeString.LeftChop(2); // Remove "[]"
        FEdGraphPinType BasePinType;
        if (ConvertStringToPinType(BaseType, BasePinType))
        {
            OutPinType = BasePinType;
            OutPinType.ContainerType = EPinContainerType::Array;
            return true;
        }
    }
    
    // Try to find custom enum by name (supports E_* prefix convention)
    UEnum* FoundEnum = FPropertyTypeResolverService::Get().FindCustomEnum(TypeString);
    if (FoundEnum)
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
        OutPinType.PinSubCategoryObject = FoundEnum;
        return true;
    }

    // Try to find custom struct or class by name
    UObject* FoundType = PropertyService->ResolveVariableType(TypeString);
    if (FoundType)
    {
        if (UScriptStruct* Struct = Cast<UScriptStruct>(FoundType))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            OutPinType.PinSubCategoryObject = Struct;
            return true;
        }
        else if (UClass* Class = Cast<UClass>(FoundType))
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
            OutPinType.PinSubCategoryObject = Class;
            return true;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("FBlueprintService::ConvertStringToPinType: Unknown type '%s'"), *TypeString);
    return false;
}

void FBlueprintService::InvalidateBlueprintCache(const FString& BlueprintName)
{
    BlueprintCache.InvalidateBlueprint(BlueprintName);
}
