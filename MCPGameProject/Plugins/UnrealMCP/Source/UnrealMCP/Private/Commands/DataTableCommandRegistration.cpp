#include "Commands/DataTableCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Services/DataTableService.h"

// Include all DataTable command headers
#include "Commands/DataTable/CreateDataTableCommand.h"
#include "Commands/DataTable/AddRowsToDataTableCommand.h"
#include "Commands/DataTable/GetDataTableRowsCommand.h"
#include "Commands/DataTable/UpdateRowsInDataTableCommand.h"
#include "Commands/DataTable/DeleteDataTableRowsCommand.h"
#include "Commands/DataTable/GetDataTableRowNamesCommand.h"
#include "Commands/DataTable/GetDataTablePropertyMapCommand.h"

TArray<TSharedPtr<IUnrealMCPCommand>> FDataTableCommandRegistration::RegisteredCommands;

void FDataTableCommandRegistration::RegisterAllCommands()
{
    UE_LOG(LogTemp, Log, TEXT("Registering DataTable commands..."));
    
    // Get the DataTable service instance
    static FDataTableService DataTableService;
    
    // Register DataTable manipulation commands
    RegisterAndTrackCommand(MakeShared<FCreateDataTableCommand>(DataTableService));
    RegisterAndTrackCommand(MakeShared<FAddRowsToDataTableCommand>(DataTableService));
    RegisterAndTrackCommand(MakeShared<FGetDataTableRowsCommand>(DataTableService));
    RegisterAndTrackCommand(MakeShared<FUpdateRowsInDataTableCommand>(DataTableService));
    RegisterAndTrackCommand(MakeShared<FDeleteDataTableRowsCommand>(DataTableService));
    RegisterAndTrackCommand(MakeShared<FGetDataTableRowNamesCommand>(DataTableService));
    RegisterAndTrackCommand(MakeShared<FGetDataTablePropertyMapCommand>(DataTableService));
    
    UE_LOG(LogTemp, Log, TEXT("Registered %d DataTable commands"), RegisteredCommands.Num());
}

void FDataTableCommandRegistration::UnregisterAllCommands()
{
    UE_LOG(LogTemp, Log, TEXT("Unregistering DataTable commands..."));
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    
    for (const TSharedPtr<IUnrealMCPCommand>& Command : RegisteredCommands)
    {
        if (Command.IsValid())
        {
            Registry.UnregisterCommand(Command->GetCommandName());
        }
    }
    
    RegisteredCommands.Empty();
    UE_LOG(LogTemp, Log, TEXT("Unregistered all DataTable commands"));
}

void FDataTableCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Attempted to register invalid DataTable command"));
        return;
    }
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    
    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommands.Add(Command);
        UE_LOG(LogTemp, Log, TEXT("Registered DataTable command: %s"), *Command->GetCommandName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to register DataTable command: %s"), *Command->GetCommandName());
    }
}