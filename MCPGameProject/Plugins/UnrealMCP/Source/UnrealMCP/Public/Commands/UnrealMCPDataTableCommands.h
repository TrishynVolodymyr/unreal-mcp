#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for DataTable-related MCP commands
 */
class UNREALMCP_API FUnrealMCPDataTableCommands
{
public:
    FUnrealMCPDataTableCommands();

    // Handle DataTable commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Specific DataTable command handlers
    TSharedPtr<FJsonObject> HandleCreateDataTable(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddRowsToDataTable(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetDataTableRows(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDeleteDataTableRows(const TSharedPtr<FJsonObject>& Params);

    // New: Handler to get property mapping for a DataTable
    TSharedPtr<FJsonObject> HandleGetDataTablePropertyMap(const TSharedPtr<FJsonObject>& Params);

    // New: Handler to get DataTable row names
    TSharedPtr<FJsonObject> HandleGetDataTableRowNames(const TSharedPtr<FJsonObject>& Params);

    // New: Handler to update rows in a DataTable
    TSharedPtr<FJsonObject> HandleUpdateRowsInDataTable(const TSharedPtr<FJsonObject>& Params);

    // Helper functions
    UDataTable* FindDataTable(const FString& DataTableName);
    bool ValidateRowData(const UDataTable* DataTable, const TSharedPtr<FJsonObject>& RowData, FString& OutErrorMessage);
    TSharedPtr<FJsonObject> DataTableToJson(const UDataTable* DataTable);
    TSharedPtr<FJsonObject> RowToJson(const UDataTable* DataTable, const FName& RowName);
}; 