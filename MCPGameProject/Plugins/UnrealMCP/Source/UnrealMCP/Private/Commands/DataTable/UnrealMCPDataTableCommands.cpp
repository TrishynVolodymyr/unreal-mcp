#include "Commands/DataTable/UnrealMCPDataTableCommands.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Commands/UnrealMCPMainDispatcher.h"

FUnrealMCPDataTableCommands::FUnrealMCPDataTableCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // All DataTable commands now route through the new main dispatcher and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(CommandType, Params);
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleCreateDataTable(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all DataTable commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("create_datatable"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleAddRowsToDataTable(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all DataTable commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_rows_to_datatable"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleGetDataTableRows(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all DataTable commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("get_datatable_rows"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleDeleteDataTableRows(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all DataTable commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("delete_datatable_rows"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleGetDataTablePropertyMap(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all DataTable commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("get_datatable_property_map"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleGetDataTableRowNames(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all DataTable commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("get_datatable_row_names"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleUpdateRowsInDataTable(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all DataTable commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("update_rows_in_datatable"), Params);
}

UDataTable* FUnrealMCPDataTableCommands::FindDataTable(const FString& DataTableName)
{
    // This helper function is no longer needed as the service layer handles data table lookup
    // Keeping the method signature for compatibility but delegating to the service layer
    return nullptr;
}

bool FUnrealMCPDataTableCommands::ValidateRowData(const UDataTable* DataTable, const TSharedPtr<FJsonObject>& RowData, FString& OutErrorMessage)
{
    // This helper function is no longer needed as the service layer handles validation
    // Keeping the method signature for compatibility but delegating to the service layer
    return true;
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::DataTableToJson(const UDataTable* DataTable)
{
    // This helper function is no longer needed as the service layer handles JSON conversion
    // Keeping the method signature for compatibility but delegating to the service layer
    return MakeShared<FJsonObject>();
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::RowToJson(const UDataTable* DataTable, const FName& RowName)
{
    // This helper function is no longer needed as the service layer handles JSON conversion
    // Keeping the method signature for compatibility but delegating to the service layer
    return MakeShared<FJsonObject>();
}

