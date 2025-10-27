#include "Commands/UnrealMCPCommandRegistry.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FUnrealMCPCommandRegistry& FUnrealMCPCommandRegistry::Get()
{
    static FUnrealMCPCommandRegistry Instance;
    return Instance;
}

bool FUnrealMCPCommandRegistry::RegisterCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FUnrealMCPCommandRegistry::RegisterCommand: Invalid command"));
        return false;
    }
    
    FString CommandName = Command->GetCommandName();
    if (CommandName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FUnrealMCPCommandRegistry::RegisterCommand: Command has empty name"));
        return false;
    }
    
    FScopeLock Lock(&RegistryLock);
    
    // Check if command is already registered
    if (RegisteredCommands.Contains(CommandName))
    {
        UE_LOG(LogTemp, Warning, TEXT("FUnrealMCPCommandRegistry::RegisterCommand: Command '%s' is already registered, replacing"), *CommandName);
    }
    
    RegisteredCommands.Add(CommandName, Command);
    UE_LOG(LogTemp, Log, TEXT("FUnrealMCPCommandRegistry::RegisterCommand: Successfully registered command '%s'"), *CommandName);
    
    return true;
}

bool FUnrealMCPCommandRegistry::UnregisterCommand(const FString& CommandName)
{
    if (CommandName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FUnrealMCPCommandRegistry::UnregisterCommand: Empty command name"));
        return false;
    }
    
    FScopeLock Lock(&RegistryLock);
    
    int32 RemovedCount = RegisteredCommands.Remove(CommandName);
    if (RemovedCount > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("FUnrealMCPCommandRegistry::UnregisterCommand: Successfully unregistered command '%s'"), *CommandName);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FUnrealMCPCommandRegistry::UnregisterCommand: Command '%s' was not registered"), *CommandName);
        return false;
    }
}

FString FUnrealMCPCommandRegistry::ExecuteCommand(const FString& CommandName, const FString& Parameters)
{
    if (CommandName.IsEmpty())
    {
        return CreateErrorResponse(TEXT("Empty command name"));
    }
    
    TSharedPtr<IUnrealMCPCommand> Command;
    {
        FScopeLock Lock(&RegistryLock);
        TSharedPtr<IUnrealMCPCommand>* CommandPtr = RegisteredCommands.Find(CommandName);
        if (!CommandPtr || !CommandPtr->IsValid())
        {
            return CreateErrorResponse(FString::Printf(TEXT("Command '%s' not found"), *CommandName));
        }
        Command = *CommandPtr;
    }
    
    // Validate parameters before execution
    if (!Command->ValidateParams(Parameters))
    {
        return CreateErrorResponse(FString::Printf(TEXT("Invalid parameters for command '%s'"), *CommandName));
    }
    
    // Execute the command
    try
    {
        FString Result = Command->Execute(Parameters);
        UE_LOG(LogTemp, Verbose, TEXT("FUnrealMCPCommandRegistry::ExecuteCommand: Successfully executed command '%s'"), *CommandName);
        return Result;
    }
    catch (const std::exception& e)
    {
        FString ErrorMessage = FString::Printf(TEXT("Exception during command execution: %s"), ANSI_TO_TCHAR(e.what()));
        UE_LOG(LogTemp, Error, TEXT("FUnrealMCPCommandRegistry::ExecuteCommand: %s"), *ErrorMessage);
        return CreateErrorResponse(ErrorMessage);
    }
    catch (...)
    {
        FString ErrorMessage = TEXT("Unknown exception during command execution");
        UE_LOG(LogTemp, Error, TEXT("FUnrealMCPCommandRegistry::ExecuteCommand: %s"), *ErrorMessage);
        return CreateErrorResponse(ErrorMessage);
    }
}

bool FUnrealMCPCommandRegistry::IsCommandRegistered(const FString& CommandName) const
{
    FScopeLock Lock(&RegistryLock);
    return RegisteredCommands.Contains(CommandName);
}

TArray<FString> FUnrealMCPCommandRegistry::GetRegisteredCommandNames() const
{
    FScopeLock Lock(&RegistryLock);
    
    TArray<FString> CommandNames;
    RegisteredCommands.GetKeys(CommandNames);
    
    // Sort alphabetically for consistent output
    CommandNames.Sort();
    
    return CommandNames;
}

FString FUnrealMCPCommandRegistry::GetCommandHelp(const FString& CommandName) const
{
    if (CommandName.IsEmpty())
    {
        return CreateErrorResponse(TEXT("Empty command name"));
    }
    
    FScopeLock Lock(&RegistryLock);
    
    const TSharedPtr<IUnrealMCPCommand>* CommandPtr = RegisteredCommands.Find(CommandName);
    if (!CommandPtr || !CommandPtr->IsValid())
    {
        return CreateErrorResponse(FString::Printf(TEXT("Command '%s' not found"), *CommandName));
    }
    
    // Create help response
    TSharedPtr<FJsonObject> HelpObj = MakeShared<FJsonObject>();
    HelpObj->SetBoolField(TEXT("success"), true);
    HelpObj->SetStringField(TEXT("command_name"), CommandName);
    HelpObj->SetStringField(TEXT("description"), FString::Printf(TEXT("Help for command: %s"), *CommandName));
    
    // Add basic command information
    TSharedPtr<FJsonObject> CommandInfo = MakeShared<FJsonObject>();
    CommandInfo->SetStringField(TEXT("name"), CommandName);
    CommandInfo->SetStringField(TEXT("type"), TEXT("blueprint_command"));
    CommandInfo->SetBoolField(TEXT("requires_parameters"), true);
    
    HelpObj->SetObjectField(TEXT("command_info"), CommandInfo);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(HelpObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FUnrealMCPCommandRegistry::GetAllCommandsHelp() const
{
    FScopeLock Lock(&RegistryLock);
    
    TSharedPtr<FJsonObject> HelpObj = MakeShared<FJsonObject>();
    HelpObj->SetBoolField(TEXT("success"), true);
    HelpObj->SetNumberField(TEXT("command_count"), RegisteredCommands.Num());
    
    // Create array of command information
    TArray<TSharedPtr<FJsonValue>> CommandsArray;
    
    TArray<FString> CommandNames;
    RegisteredCommands.GetKeys(CommandNames);
    CommandNames.Sort();
    
    for (const FString& CommandName : CommandNames)
    {
        TSharedPtr<FJsonObject> CommandInfo = MakeShared<FJsonObject>();
        CommandInfo->SetStringField(TEXT("name"), CommandName);
        CommandInfo->SetStringField(TEXT("type"), TEXT("blueprint_command"));
        CommandInfo->SetBoolField(TEXT("requires_parameters"), true);
        CommandInfo->SetStringField(TEXT("description"), FString::Printf(TEXT("Blueprint command: %s"), *CommandName));
        
        CommandsArray.Add(MakeShared<FJsonValueObject>(CommandInfo));
    }
    
    HelpObj->SetArrayField(TEXT("commands"), CommandsArray);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(HelpObj.ToSharedRef(), Writer);
    
    return OutputString;
}

void FUnrealMCPCommandRegistry::ClearRegistry()
{
    FScopeLock Lock(&RegistryLock);
    
    int32 ClearedCount = RegisteredCommands.Num();
    RegisteredCommands.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("FUnrealMCPCommandRegistry::ClearRegistry: Cleared %d registered commands"), ClearedCount);
}

FString FUnrealMCPCommandRegistry::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FUnrealMCPCommandRegistry::CreateSuccessResponse(const FString& Message) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), Message);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}
