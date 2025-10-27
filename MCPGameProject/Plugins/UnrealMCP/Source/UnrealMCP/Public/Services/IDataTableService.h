#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Dom/JsonObject.h"

/**
 * Parameters for DataTable creation operations
 */
struct UNREALMCP_API FDataTableCreationParams
{
    /** Name of the DataTable to create */
    FString Name;
    
    /** Path where the DataTable should be created */
    FString Path = TEXT("/Game/Data");
    
    /** Name or path of the struct to use for rows */
    FString RowStructName;
    
    /** Optional description for the DataTable */
    FString Description;
    
    /** Default constructor */
    FDataTableCreationParams() = default;
    
    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for DataTable row operations
 */
struct UNREALMCP_API FDataTableRowParams
{
    /** Name of the row */
    FString RowName;
    
    /** Row data as JSON object */
    TSharedPtr<FJsonObject> RowData;
    
    /** Default constructor */
    FDataTableRowParams() = default;
    
    /**
     * Validate the parameters
     * @param DataTable - Target DataTable for validation
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(const UDataTable* DataTable, FString& OutError) const;
};

/**
 * Interface for DataTable service operations
 * Provides abstraction for DataTable creation, modification, and management
 */
class UNREALMCP_API IDataTableService
{
public:
    virtual ~IDataTableService() = default;
    
    /**
     * Create a new DataTable
     * @param Params - DataTable creation parameters
     * @return Created DataTable or nullptr if failed
     */
    virtual UDataTable* CreateDataTable(const FDataTableCreationParams& Params) = 0;
    
    /**
     * Find a DataTable by name
     * @param DataTableName - Name of the DataTable to find
     * @return Found DataTable or nullptr
     */
    virtual UDataTable* FindDataTable(const FString& DataTableName) = 0;
    
    /**
     * Add rows to a DataTable
     * @param DataTable - Target DataTable
     * @param Rows - Array of row parameters to add
     * @param OutAddedRows - Names of successfully added rows
     * @param OutFailedRows - Names of rows that failed to add
     * @return true if at least one row was added successfully
     */
    virtual bool AddRowsToDataTable(UDataTable* DataTable, const TArray<FDataTableRowParams>& Rows, TArray<FString>& OutAddedRows, TArray<FString>& OutFailedRows) = 0;
    
    /**
     * Update rows in a DataTable
     * @param DataTable - Target DataTable
     * @param Rows - Array of row parameters to update
     * @param OutUpdatedRows - Names of successfully updated rows
     * @param OutFailedRows - Names of rows that failed to update
     * @return true if at least one row was updated successfully
     */
    virtual bool UpdateRowsInDataTable(UDataTable* DataTable, const TArray<FDataTableRowParams>& Rows, TArray<FString>& OutUpdatedRows, TArray<FString>& OutFailedRows) = 0;
    
    /**
     * Delete rows from a DataTable
     * @param DataTable - Target DataTable
     * @param RowNames - Names of rows to delete
     * @param OutDeletedRows - Names of successfully deleted rows
     * @param OutFailedRows - Names of rows that failed to delete
     * @return true if at least one row was deleted successfully
     */
    virtual bool DeleteRowsFromDataTable(UDataTable* DataTable, const TArray<FString>& RowNames, TArray<FString>& OutDeletedRows, TArray<FString>& OutFailedRows) = 0;
    
    /**
     * Get all rows from a DataTable
     * @param DataTable - Target DataTable
     * @param RowNames - Optional specific row names to retrieve (empty for all rows)
     * @return JSON object containing the rows
     */
    virtual TSharedPtr<FJsonObject> GetDataTableRows(const UDataTable* DataTable, const TArray<FString>& RowNames = TArray<FString>()) = 0;
    
    /**
     * Get row names and field names from a DataTable
     * @param DataTable - Target DataTable
     * @param OutRowNames - Array to fill with row names
     * @param OutFieldNames - Array to fill with field names
     * @return true if successful
     */
    virtual bool GetDataTableRowNames(const UDataTable* DataTable, TArray<FString>& OutRowNames, TArray<FString>& OutFieldNames) = 0;
    
    /**
     * Get property mapping for a DataTable (GUID to struct name mapping)
     * @param DataTable - Target DataTable
     * @return JSON object containing the property mapping
     */
    virtual TSharedPtr<FJsonObject> GetDataTablePropertyMap(const UDataTable* DataTable) = 0;
    
    /**
     * Validate row data against DataTable structure
     * @param DataTable - Target DataTable
     * @param RowData - Row data to validate
     * @param OutError - Error message if validation fails
     * @return true if validation succeeds
     */
    virtual bool ValidateRowData(const UDataTable* DataTable, const TSharedPtr<FJsonObject>& RowData, FString& OutError) = 0;
    
    /**
     * Get detailed error message from the last failed operation
     * @return Last error message with details
     */
    virtual FString GetLastErrorMessage() const = 0;
};
