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
     * @deprecated Use FDataTableTransformationService::AutoTransformToGuidNames instead
     */
    TSharedPtr<FJsonObject> AutoTransformToGuidNames(const TSharedPtr<FJsonObject>& InJson, const UScriptStruct* RowStruct);

    /**
     * @deprecated Use FDataTableTransformationService::AutoTransformFromGuidNames instead
     */
    TSharedPtr<FJsonObject> AutoTransformFromGuidNames(const TSharedPtr<FJsonObject>& InJson, const UScriptStruct* RowStruct);
    
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
    
    /**
     * Check if an asset already exists at the given path
     * @param AssetPath - Full path to check for asset existence
     * @return true if asset exists, false otherwise
     */
    bool DoesAssetExist(const FString& AssetPath);
    
    /**
     * Generate a unique asset name by appending incremental numbers
     * @param BaseName - Base name for the asset
     * @param AssetPath - Path where the asset will be created
     * @param MaxRetries - Maximum number of retry attempts (default: 100)
     * @return Unique asset name that doesn't conflict with existing assets
     */
    FString GenerateUniqueAssetName(const FString& BaseName, const FString& AssetPath, int32 MaxRetries = 100);
    
    /**
     * Get detailed error message from the last failed operation
     * @return Last error message with details
     */
    virtual FString GetLastErrorMessage() const override { return LastErrorMessage; }
    
    /**
     * Get list of paths that were tried when searching for a struct
     * @param StructName - Original struct name that was searched
     * @return Formatted string of all tried paths
     */
    FString GetTriedStructPaths(const FString& StructName) const;
    
    /**
     * Fill missing fields in JSON object with default values based on struct definition
     * @param RowStruct - The struct definition to use for filling missing fields
     * @param RowData - JSON object to fill with missing fields
     */
    void FillMissingFields(const UScriptStruct* RowStruct, const TSharedPtr<FJsonObject>& RowData);

private:
    /** Last error message from failed operations */
    FString LastErrorMessage;
    
    /** Paths that were tried during the last struct search */
    TArray<FString> TriedStructPaths;
};
