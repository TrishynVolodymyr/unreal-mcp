#pragma once

#include "CoreMinimal.h"
#include "Services/IDataTableService.h"

/**
 * Concrete implementation of IDataTableService
 * Handles all DataTable operations using Unreal Engine APIs
 */
class UNREALMCP_API FDataTableService : public IDataTableService
{
public:
    FDataTableService();
    virtual ~FDataTableService() = default;
    
    // IDataTableService interface
    virtual UDataTable* CreateDataTable(const FDataTableCreationParams& Params) override;
    virtual UDataTable* FindDataTable(const FString& DataTableName) override;
    virtual bool AddRowsToDataTable(UDataTable* DataTable, const TArray<FDataTableRowParams>& Rows, TArray<FString>& OutAddedRows, TArray<FString>& OutFailedRows) override;
    virtual bool UpdateRowsInDataTable(UDataTable* DataTable, const TArray<FDataTableRowParams>& Rows, TArray<FString>& OutUpdatedRows, TArray<FString>& OutFailedRows) override;
    virtual bool DeleteRowsFromDataTable(UDataTable* DataTable, const TArray<FString>& RowNames, TArray<FString>& OutDeletedRows, TArray<FString>& OutFailedRows) override;
    virtual TSharedPtr<FJsonObject> GetDataTableRows(const UDataTable* DataTable, const TArray<FString>& RowNames = TArray<FString>()) override;
    virtual bool GetDataTableRowNames(const UDataTable* DataTable, TArray<FString>& OutRowNames, TArray<FString>& OutFieldNames) override;
    virtual TSharedPtr<FJsonObject> GetDataTablePropertyMap(const UDataTable* DataTable) override;
    virtual bool ValidateRowData(const UDataTable* DataTable, const TSharedPtr<FJsonObject>& RowData, FString& OutError) override;

private:
    /**
     * Find struct by trying multiple path variations
     * @param StructName - Name or path of the struct
     * @return Found struct or nullptr
     */
    UScriptStruct* FindStruct(const FString& StructName);
    
    /**
     * Build GUID to struct name mapping for a struct
     * @param RowStruct - Target struct
     * @return Mapping from GUID property names to struct property names
     */
    TMap<FString, FString> BuildGuidToStructNameMap(const UScriptStruct* RowStruct);
    
    /**
     * Transform JSON object from GUID property names to struct property names
     * @param InJson - Input JSON with GUID property names
     * @param GuidToStructMap - Mapping from GUID to struct names
     * @return Transformed JSON with struct property names
     */
    TSharedPtr<FJsonObject> TransformJsonToStructNames(const TSharedPtr<FJsonObject>& InJson, const TMap<FString, FString>& GuidToStructMap);
    
    /**
     * Convert a single row to JSON
     * @param DataTable - Source DataTable
     * @param RowName - Name of the row to convert
     * @return JSON representation of the row
     */
    TSharedPtr<FJsonObject> RowToJson(const UDataTable* DataTable, const FName& RowName);
    
    /**
     * Refresh DataTable editor UI
     * @param DataTable - DataTable to refresh
     */
    void RefreshDataTableEditor(UDataTable* DataTable);
    
    /**
     * Save and sync DataTable asset
     * @param DataTable - DataTable to save
     */
    void SaveAndSyncDataTable(UDataTable* DataTable);
};
