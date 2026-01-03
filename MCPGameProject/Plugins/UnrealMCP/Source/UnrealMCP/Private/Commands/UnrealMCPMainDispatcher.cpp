#include "Commands/UnrealMCPMainDispatcher.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/BlueprintCommandRegistration.h"
#include "Commands/BlueprintNodeCommandRegistration.h"
#include "Commands/BlueprintActionCommandRegistration.h"
#include "Commands/GraphManipulationCommandRegistration.h"
#include "Commands/ProjectCommandRegistration.h"
#include "Commands/DataTableCommandRegistration.h"
#include "Commands/EditorCommandRegistration.h"
#include "Commands/UMGCommandRegistration.h"
#include "Commands/MaterialCommandRegistration.h"
#include "Commands/NiagaraCommandRegistration.h"
#include "Services/BlueprintActionService.h"
// Legacy adapter removed
#include "Services/BlueprintService.h"
#include "Services/ProjectService.h"
#include "Services/DataTableService.h"
#include "Services/EditorService.h"
#include "Services/MaterialService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FUnrealMCPMainDispatcher& FUnrealMCPMainDispatcher::Get()
{
    static FUnrealMCPMainDispatcher Instance;
    return Instance;
}

TSharedPtr<FJsonObject> FUnrealMCPMainDispatcher::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Error, TEXT("FUnrealMCPMainDispatcher::HandleCommand: Dispatcher not initialized"));
        return CreateErrorResponse(TEXT("Command dispatcher not initialized"));
    }
    
    if (CommandType.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FUnrealMCPMainDispatcher::HandleCommand: Empty command type"));
        return CreateErrorResponse(TEXT("Empty command type"));
    }
    
    // Legacy adapter removed - all commands now use the new system
    
    // Convert JSON object to string for registry
    FString ParametersString = JsonObjectToString(Params);
    
    // Execute command through registry
    FString ResultString = FUnrealMCPCommandRegistry::Get().ExecuteCommand(CommandType, ParametersString);
    
    // Convert result back to JSON object
    TSharedPtr<FJsonObject> ResultObject = JsonStringToObject(ResultString);
    if (!ResultObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FUnrealMCPMainDispatcher::HandleCommand: Failed to parse command result as JSON"));
        return CreateErrorResponse(TEXT("Failed to parse command result"));
    }
    
    return ResultObject;
}

FString FUnrealMCPMainDispatcher::HandleCommandString(const FString& CommandType, const FString& Parameters)
{
    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Error, TEXT("FUnrealMCPMainDispatcher::HandleCommandString: Dispatcher not initialized"));
        TSharedPtr<FJsonObject> ErrorObj = CreateErrorResponse(TEXT("Command dispatcher not initialized"));
        return JsonObjectToString(ErrorObj);
    }
    
    if (CommandType.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FUnrealMCPMainDispatcher::HandleCommandString: Empty command type"));
        TSharedPtr<FJsonObject> ErrorObj = CreateErrorResponse(TEXT("Empty command type"));
        return JsonObjectToString(ErrorObj);
    }
    
    // Execute command through registry
    return FUnrealMCPCommandRegistry::Get().ExecuteCommand(CommandType, Parameters);
}

void FUnrealMCPMainDispatcher::RegisterAllCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FUnrealMCPMainDispatcher::RegisterAllCommands: Registering all command types"));
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    
    // Register Blueprint commands
    FBlueprintCommandRegistration::RegisterAllBlueprintCommands();
    
    // Register Blueprint Node commands
    FBlueprintNodeCommandRegistration::RegisterAllBlueprintNodeCommands();
    
    // Register Graph Manipulation commands (low-level node operations)
    FGraphManipulationCommandRegistration::RegisterAllGraphManipulationCommands();
    
    // Register Blueprint Action commands (new architecture)
    TSharedPtr<IBlueprintActionService> BlueprintActionService = MakeShared<FBlueprintActionService>();
    FBlueprintActionCommandRegistration::RegisterCommands(Registry, BlueprintActionService);
    
    // Register Project commands
    TSharedPtr<IProjectService> ProjectService = MakeShared<FProjectService>();
    FProjectCommandRegistration::RegisterCommands(Registry, ProjectService);
    
    // Register DataTable commands
    FDataTableCommandRegistration::RegisterAllCommands();
    
    // Register Editor commands
    FEditorCommandRegistration::RegisterAllCommands();
    
    // Register UMG commands
    FUMGCommandRegistration::RegisterAllUMGCommands();

    // Register Material commands
    FMaterialCommandRegistration::RegisterAllCommands();

    // Register Niagara VFX commands
    FNiagaraCommandRegistration::RegisterAllCommands();

    UE_LOG(LogTemp, Log, TEXT("FUnrealMCPMainDispatcher::RegisterAllCommands: All command types registered"));
}

void FUnrealMCPMainDispatcher::Initialize()
{
    if (bIsInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("FUnrealMCPMainDispatcher::Initialize: Already initialized"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FUnrealMCPMainDispatcher::Initialize: Initializing command dispatcher"));
    
    // Legacy adapter removed
    
    // Register all command types
    RegisterAllCommands();
    
    bIsInitialized = true;
    
    UE_LOG(LogTemp, Log, TEXT("FUnrealMCPMainDispatcher::Initialize: Command dispatcher initialized successfully"));
}

void FUnrealMCPMainDispatcher::Shutdown()
{
    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("FUnrealMCPMainDispatcher::Shutdown: Not initialized"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FUnrealMCPMainDispatcher::Shutdown: Shutting down command dispatcher"));
    
    // Unregister all command types
    FBlueprintCommandRegistration::UnregisterAllBlueprintCommands();
    FBlueprintNodeCommandRegistration::UnregisterAllBlueprintNodeCommands();
    FBlueprintActionCommandRegistration::UnregisterAllBlueprintActionCommands();
    FDataTableCommandRegistration::UnregisterAllCommands();
    FEditorCommandRegistration::UnregisterAllCommands();
    FUMGCommandRegistration::UnregisterAllUMGCommands();
    FMaterialCommandRegistration::UnregisterAllCommands();
    FNiagaraCommandRegistration::UnregisterAllCommands();

    // Clear the entire registry
    FUnrealMCPCommandRegistry::Get().ClearRegistry();
    
    bIsInitialized = false;
    
    UE_LOG(LogTemp, Log, TEXT("FUnrealMCPMainDispatcher::Shutdown: Command dispatcher shut down successfully"));
}

TSharedPtr<FJsonObject> FUnrealMCPMainDispatcher::GetAvailableCommands()
{
    if (!bIsInitialized)
    {
        return CreateErrorResponse(TEXT("Command dispatcher not initialized"));
    }
    
    // Get help information from registry
    FString HelpString = FUnrealMCPCommandRegistry::Get().GetAllCommandsHelp();
    TSharedPtr<FJsonObject> HelpObject = JsonStringToObject(HelpString);
    
    if (!HelpObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to get command help information"));
    }
    
    return HelpObject;
}

FString FUnrealMCPMainDispatcher::JsonObjectToString(const TSharedPtr<FJsonObject>& JsonObject) const
{
    if (!JsonObject.IsValid())
    {
        return TEXT("{}");
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

TSharedPtr<FJsonObject> FUnrealMCPMainDispatcher::JsonStringToObject(const FString& JsonString) const
{
    if (JsonString.IsEmpty())
    {
        return MakeShared<FJsonObject>();
    }
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("FUnrealMCPMainDispatcher::JsonStringToObject: Failed to parse JSON string: %s"), *JsonString);
        return MakeShared<FJsonObject>();
    }
    
    return JsonObject;
}

TSharedPtr<FJsonObject> FUnrealMCPMainDispatcher::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    
    return ErrorObj;
}
