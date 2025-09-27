#include "Services/DataTableService.h"
#include "Services/DataTableTransformationService.h"
#include "Engine/DataTable.h"
#include "UObject/ConstructorHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/DataTableFactory.h"
#include "EditorScriptingUtilities/Public/EditorAssetLibrary.h"
#include "AssetToolsModule.h"
#include "JsonObjectConverter.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/MetaData.h"
#include "ScopedTransaction.h"

FDataTableService::FDataTableService()
{
}

bool FDataTableCreationParams::IsValid(FString& OutError) const
{
    if (Name.IsEmpty())
    {
        OutError = TEXT("DataTable name cannot be empty");
        return false;
    }
    
    if (RowStructName.IsEmpty())
    {
        OutError = TEXT("Row struct name cannot be empty");
        return false;
    }
    
    return true;
}

bool FDataTableRowParams::IsValid(const UDataTable* DataTable, FString& OutError) const
{
    if (RowName.IsEmpty())
    {
        OutError = TEXT("Row name cannot be empty");
        return false;
    }
    
    if (!RowData.IsValid())
    {
        OutError = TEXT("Row data is invalid");
        return false;
    }
    
    if (!DataTable)
    {
        OutError = TEXT("DataTable is null");
        return false;
    }
    
    return true;
}

UDataTable* FDataTableService::CreateDataTable(const FDataTableCreationParams& Params)
{
    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Invalid parameters: %s"), *ValidationError);
        LastErrorMessage = FString::Printf(TEXT("Invalid parameters: %s"), *ValidationError);
        return nullptr;
    }
    
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Creating DataTable named '%s'"), *Params.Name);
    
    // Find the struct
    UScriptStruct* FoundStruct = FindStruct(Params.RowStructName);
    if (!FoundStruct)
    {
        LastErrorMessage = FString::Printf(TEXT("Struct '%s' not found. Make sure the project is compiled and the struct exists. Tried paths: %s"), 
            *Params.RowStructName, *GetTriedStructPaths(Params.RowStructName));
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: %s"), *LastErrorMessage);
        return nullptr;
    }
    
    // Asset verification and retry logic
    FString FinalName = Params.Name;
    FString FullPath = FString::Printf(TEXT("%s/%s"), *Params.Path, *FinalName);
    
    // Check if asset already exists
    if (DoesAssetExist(FullPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Asset already exists at path: '%s'"), *FullPath);
        
        // Generate unique name with retry logic
        FinalName = GenerateUniqueAssetName(Params.Name, Params.Path);
        FullPath = FString::Printf(TEXT("%s/%s"), *Params.Path, *FinalName);
        
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Using unique name '%s' to avoid conflicts"), *FinalName);
    }
    
    // Create the DataTable using factory
    UDataTableFactory* Factory = NewObject<UDataTableFactory>();
    Factory->Struct = FoundStruct;
    
    // Create the asset using IAssetTools
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Attempting to create asset at path: '%s'"), *FullPath);
    
    UDataTable* NewDataTable = Cast<UDataTable>(AssetToolsModule.Get().CreateAsset(FinalName, Params.Path, UDataTable::StaticClass(), Factory));
    
    if (!NewDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Failed to create DataTable asset"));
        return nullptr;
    }
    
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Successfully created DataTable asset at: '%s'"), *NewDataTable->GetPathName());
    
    // Note: Metadata setting removed for UE 5.6 compatibility
    if (!Params.Description.IsEmpty())
    {
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Description provided but metadata setting skipped for UE 5.6 compatibility: '%s'"), *Params.Description);
    }
    
    // Save the asset
    SaveAndSyncDataTable(NewDataTable);
    
    return NewDataTable;
}

UDataTable* FDataTableService::FindDataTable(const FString& DataTableName)
{
    // Try multiple path variations to find the datatable
    TArray<FString> PathVariations;
    PathVariations.Add(FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("Data/%s"), *DataTableName)));
    PathVariations.Add(FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("Data/%s.%s"), *DataTableName, *DataTableName)));
    PathVariations.Add(DataTableName); // Try direct name
    PathVariations.Add(FUnrealMCPCommonUtils::BuildGamePath(DataTableName)); // Try under Game root
    
    for (const FString& Path : PathVariations)
    {
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Attempting to load DataTable at path: '%s'"), *Path);
        UDataTable* FoundTable = Cast<UDataTable>(UEditorAssetLibrary::LoadAsset(Path));
        if (FoundTable)
        {
            UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Successfully found DataTable at: '%s'"), *Path);
            return FoundTable;
        }
    }
    
    UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Failed to find DataTable: '%s' in any location"), *DataTableName);
    return nullptr;
}

bool FDataTableService::AddRowsToDataTable(UDataTable* DataTable, const TArray<FDataTableRowParams>& Rows, TArray<FString>& OutAddedRows, TArray<FString>& OutFailedRows)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: DataTable is null"));
        return false;
    }
    
    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    if (!RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Failed to get row struct from DataTable"));
        return false;
    }
    
        OutAddedRows.Empty();
    OutFailedRows.Empty();
    
    for (const FDataTableRowParams& RowParams : Rows)
    {
        FString ValidationError;
        if (!RowParams.IsValid(DataTable, ValidationError))
        {
            OutFailedRows.Add(FString::Printf(TEXT("%s: %s"), *RowParams.RowName, *ValidationError));
            continue;
        }
        
        // Check if we have GUID fields that need transformation BEFORE validation (to avoid auto-fill contamination)
        bool bHasGuidFields = false;
        for (const auto& Field : RowParams.RowData->Values)
        {
            bool bIsGuid = FDataTableTransformationService::IsGuidField(Field.Key);
            UE_LOG(LogTemp, Warning, TEXT("Field '%s' is GUID: %s"), *Field.Key, bIsGuid ? TEXT("YES") : TEXT("NO"));
            if (bIsGuid)
            {
                bHasGuidFields = true;
                break;
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("Has GUID fields: %s"), bHasGuidFields ? TEXT("YES") : TEXT("NO"));
        
        // Transform GUID field names to friendly names before validation and JsonObjectToUStruct
        TSharedPtr<FJsonObject> StructJson = RowParams.RowData;
        if (bHasGuidFields)
        {
            StructJson = FDataTableTransformationService::AutoTransformFromGuidNames(RowParams.RowData, RowStruct);
        }
        
        // Validate row data (after potential transformation)
        if (!ValidateRowData(DataTable, StructJson, ValidationError))
        {
            OutFailedRows.Add(FString::Printf(TEXT("%s: %s"), *RowParams.RowName, *ValidationError));
            continue;
        }
        
        // Allocate memory for the new row
        uint8* RowMemory = (uint8*)FMemory::Malloc(RowStruct->GetStructureSize());
        RowStruct->InitializeStruct(RowMemory);
        FTableRowBase* NewRow = reinterpret_cast<FTableRowBase*>(RowMemory);
        
        TSharedRef<FJsonObject> JsonRef = StructJson.ToSharedRef();
        
        // DEBUG: Log the JSON structure before conversion
        FString JsonString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(JsonRef, Writer);
        UE_LOG(LogTemp, Warning, TEXT("MCP DEBUG: JSON before conversion: %s"), *JsonString);
        
        // Log each field in JsonRef->Values
        for (const auto& Pair : JsonRef->Values)
        {
            FString ValueType = TEXT("Unknown");
            if (Pair.Value->Type == EJson::String) ValueType = TEXT("String");
            else if (Pair.Value->Type == EJson::Number) ValueType = TEXT("Number");
            else if (Pair.Value->Type == EJson::Boolean) ValueType = TEXT("Boolean");
            else if (Pair.Value->Type == EJson::Array) ValueType = TEXT("Array");
            else if (Pair.Value->Type == EJson::Object) ValueType = TEXT("Object");
            
            UE_LOG(LogTemp, Warning, TEXT("MCP DEBUG: JSON field '%s' = Type: %s"), *Pair.Key, *ValueType);
            
            if (Pair.Value->Type == EJson::Array)
            {
                const TArray<TSharedPtr<FJsonValue>>* ArrayPtr = nullptr;
                if (Pair.Value->TryGetArray(ArrayPtr) && ArrayPtr)
                {
                    UE_LOG(LogTemp, Warning, TEXT("MCP DEBUG: Array field '%s' has %d elements"), *Pair.Key, ArrayPtr->Num());
                }
            }
        }
        
        // UE 5.6 proper fix: Use JsonAttributesToUStruct for struct array support
        bool bJsonConverted = FJsonObjectConverter::JsonAttributesToUStruct(JsonRef->Values, RowStruct, RowMemory);
        
        UE_LOG(LogTemp, Warning, TEXT("MCP DEBUG: JsonAttributesToUStruct result: %s"), bJsonConverted ? TEXT("SUCCESS") : TEXT("FAILED"));
        
        if (!bJsonConverted)
        {
            RowStruct->DestroyStruct(RowMemory);
            FMemory::Free(RowMemory);
            OutFailedRows.Add(FString::Printf(TEXT("%s: failed to convert JSON to UStruct"), *RowParams.RowName));
            continue;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: JsonObjectToUStruct SUCCESS - Adding row '%s' to DataTable"), *RowParams.RowName);
        DataTable->AddRow(FName(*RowParams.RowName), *NewRow);
        UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Row '%s' successfully added to DataTable"), *RowParams.RowName);
        
        RowStruct->DestroyStruct(RowMemory);
        FMemory::Free(RowMemory);
        
        OutAddedRows.Add(RowParams.RowName);
    }
    
    if (OutAddedRows.Num() > 0)
    {
        // Mark dirty and refresh
        DataTable->Modify(true);
        DataTable->PostEditChange();
        DataTable->MarkPackageDirty();
        
        SaveAndSyncDataTable(DataTable);
        RefreshDataTableEditor(DataTable);
        
        return true;
    }
    
    return false;
}

bool FDataTableService::UpdateRowsInDataTable(UDataTable* DataTable, const TArray<FDataTableRowParams>& Rows, TArray<FString>& OutUpdatedRows, TArray<FString>& OutFailedRows)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: DataTable is null"));
        return false;
    }
    
    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    if (!RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Failed to get row struct from DataTable"));
        return false;
    }
    
    OutUpdatedRows.Empty();
    OutFailedRows.Empty();
    
    for (const FDataTableRowParams& RowParams : Rows)
    {
        FString ValidationError;
        if (!RowParams.IsValid(DataTable, ValidationError))
        {
            OutFailedRows.Add(FString::Printf(TEXT("%s: %s"), *RowParams.RowName, *ValidationError));
            continue;
        }
        
        // Check if row exists
        if (!DataTable->GetRowMap().Contains(FName(*RowParams.RowName)))
        {
            OutFailedRows.Add(FString::Printf(TEXT("%s: row not found"), *RowParams.RowName));
            continue;
        }
        
        // Check if we have GUID fields that need transformation BEFORE validation (to avoid auto-fill contamination)
        bool bHasGuidFields = false;
        for (const auto& Field : RowParams.RowData->Values)
        {
            bool bIsGuid = FDataTableTransformationService::IsGuidField(Field.Key);
            UE_LOG(LogTemp, Warning, TEXT("Field '%s' is GUID: %s"), *Field.Key, bIsGuid ? TEXT("YES") : TEXT("NO"));
            if (bIsGuid)
            {
                bHasGuidFields = true;
                break;
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("Has GUID fields: %s"), bHasGuidFields ? TEXT("YES") : TEXT("NO"));
        
        // Transform GUID field names to friendly names before validation and JsonObjectToUStruct
        TSharedPtr<FJsonObject> StructJson = RowParams.RowData;
        if (bHasGuidFields)
        {
            StructJson = FDataTableTransformationService::AutoTransformFromGuidNames(RowParams.RowData, RowStruct);
        }
        
        // Validate row data (after potential transformation)
        if (!ValidateRowData(DataTable, StructJson, ValidationError))
        {
            OutFailedRows.Add(FString::Printf(TEXT("%s: %s"), *RowParams.RowName, *ValidationError));
            continue;
        }
        
        // Allocate memory for the new row
        uint8* RowMemory = (uint8*)FMemory::Malloc(RowStruct->GetStructureSize());
        RowStruct->InitializeStruct(RowMemory);
        FTableRowBase* NewRow = reinterpret_cast<FTableRowBase*>(RowMemory);
        
        TSharedRef<FJsonObject> JsonRef = StructJson.ToSharedRef();
        
        // UE 5.6 proper fix: Use JsonAttributesToUStruct for struct array support
        bool bJsonConverted = FJsonObjectConverter::JsonAttributesToUStruct(JsonRef->Values, RowStruct, RowMemory);
        
        if (!bJsonConverted)
        {
            RowStruct->DestroyStruct(RowMemory);
            FMemory::Free(RowMemory);
            OutFailedRows.Add(FString::Printf(TEXT("%s: failed to convert JSON to UStruct"), *RowParams.RowName));
            continue;
        }
        
        // Use AddRow to update the row
        DataTable->AddRow(FName(*RowParams.RowName), *NewRow);
        
        // Notify DataTable of the change
        DataTable->HandleDataTableChanged(FName(*RowParams.RowName));
        
        RowStruct->DestroyStruct(RowMemory);
        FMemory::Free(RowMemory);
        
        OutUpdatedRows.Add(RowParams.RowName);
    }
    
    if (OutUpdatedRows.Num() > 0)
    {
        // Mark dirty and refresh
        DataTable->Modify(true);
        DataTable->PostEditChange();
        DataTable->MarkPackageDirty();
        
        SaveAndSyncDataTable(DataTable);
        RefreshDataTableEditor(DataTable);
        
        return true;
    }
    
    return false;
}

bool FDataTableService::DeleteRowsFromDataTable(UDataTable* DataTable, const TArray<FString>& RowNames, TArray<FString>& OutDeletedRows, TArray<FString>& OutFailedRows)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: DataTable is null"));
        return false;
    }
    
    // Check if DataTable is valid and has a row struct
    if (!DataTable->GetRowStruct())
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: DataTable has no row struct"));
        return false;
    }
    

    
    OutDeletedRows.Empty();
    OutFailedRows.Empty();
    
    // First, validate all row names exist before attempting deletion
    TArray<FName> ValidRowNames;
    for (const FString& RowName : RowNames)
    {
        FName RowFName(*RowName);
        if (DataTable->GetRowMap().Contains(RowFName))
        {
            ValidRowNames.Add(RowFName);
        }
        else
        {
            OutFailedRows.Add(RowName);
            UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Row '%s' not found in DataTable"), *RowName);
        }
    }
    
    // Use direct row map manipulation to avoid UE 5.6 RemoveRow() crashes
    for (const FName& RowFName : ValidRowNames)
    {
        try
        {
            // Mark the DataTable as modified before deletion
            DataTable->Modify();
            
            // Get direct access to the row map
            TMap<FName, uint8*>& RowMap = const_cast<TMap<FName, uint8*>&>(DataTable->GetRowMap());
            
            // Double-check the row exists before deletion
            if (RowMap.Contains(RowFName))
            {
                // Create a scoped transaction for undo/redo support
                FScopedTransaction Transaction(FText::FromString(FString::Printf(TEXT("Delete DataTable Row '%s'"), *RowFName.ToString())));
                
                // Mark as modified again within transaction
                DataTable->Modify();
                
                // Get the row data pointer before removing
                uint8* RowData = RowMap[RowFName];
                
                // Remove from the map first
                RowMap.Remove(RowFName);
                
                // Free the memory if it exists and we have a valid struct
                if (RowData && DataTable->GetRowStruct())
                {
                    // Properly destroy the struct data
                    DataTable->GetRowStruct()->DestroyStruct(RowData);
                    // Free the allocated memory
                    FMemory::Free(RowData);
                }
                
                OutDeletedRows.Add(RowFName.ToString());
                UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Successfully deleted row '%s'"), *RowFName.ToString());
            }
            else
            {
                OutFailedRows.Add(RowFName.ToString());
                UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Row '%s' not found during deletion"), *RowFName.ToString());
            }
        }
        catch (const std::exception& e)
        {
            OutFailedRows.Add(RowFName.ToString());
            UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Exception while deleting row '%s': %s"), *RowFName.ToString(), *FString(e.what()));
        }
        catch (...)
        {
            OutFailedRows.Add(RowFName.ToString());
            UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Unknown exception while deleting row '%s'"), *RowFName.ToString());
        }
    }
    
    // Only save if we successfully deleted at least one row
    if (OutDeletedRows.Num() > 0)
    {
        try
        {
            SaveAndSyncDataTable(DataTable);
            RefreshDataTableEditor(DataTable);
            UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Successfully deleted %d rows, failed %d rows"), OutDeletedRows.Num(), OutFailedRows.Num());
            return true;
        }
        catch (...)
        {
            UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Exception occurred while saving DataTable after deletion"));
            return false;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: No rows were deleted"));
    return false;
}

TSharedPtr<FJsonObject> FDataTableService::GetDataTableRows(const UDataTable* DataTable, const TArray<FString>& RowNames)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: DataTable is null"));
        return nullptr;
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> RowsArray;
    
    if (RowNames.Num() > 0)
    {
        // Get specific rows
        for (const FString& RowName : RowNames)
        {
            if (DataTable->GetRowMap().Contains(FName(*RowName)))
            {
                RowsArray.Add(MakeShared<FJsonValueObject>(RowToJson(DataTable, FName(*RowName))));
            }
        }
    }
    else
    {
        // Get all rows
        for (const auto& RowPair : DataTable->GetRowMap())
        {
            RowsArray.Add(MakeShared<FJsonValueObject>(RowToJson(DataTable, RowPair.Key)));
        }
    }
    
    ResultObj->SetArrayField(TEXT("rows"), RowsArray);
    return ResultObj;
}

bool FDataTableService::GetDataTableRowNames(const UDataTable* DataTable, TArray<FString>& OutRowNames, TArray<FString>& OutFieldNames)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: DataTable is null"));
        return false;
    }
    
    // Get row names
    TArray<FName> RowNames = DataTable->GetRowNames();
    OutRowNames.Empty();
    for (const FName& RowName : RowNames)
    {
        OutRowNames.Add(RowName.ToString());
    }
    
    // Get field (struct property) names
    OutFieldNames.Empty();
    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    if (RowStruct)
    {
        for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
        {
            FProperty* Property = *PropIt;
            OutFieldNames.Add(Property->GetName());
        }
    }
    
    return true;
}

TSharedPtr<FJsonObject> FDataTableService::GetDataTablePropertyMap(const UDataTable* DataTable)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: DataTable is null"));
        return nullptr;
    }
    
    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    if (!RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Failed to get row struct from DataTable"));
        return nullptr;
    }
    
    TSharedPtr<FJsonObject> MappingObj = MakeShared<FJsonObject>();
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        FString InternalName = Property->GetName();
        FString DisplayName = Property->GetAuthoredName(); // This is usually the user-facing name
        MappingObj->SetStringField(DisplayName, InternalName);
    }
    
    return MappingObj;
}

bool FDataTableService::ValidateRowData(const UDataTable* DataTable, const TSharedPtr<FJsonObject>& RowData, FString& OutError)
{
    if (!DataTable || !DataTable->GetRowStruct())
    {
        OutError = TEXT("Invalid DataTable or row struct");
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: %s"), *OutError);
        return false;
    }
    
    if (!RowData.IsValid())
    {
        OutError = TEXT("Invalid row data");
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: %s"), *OutError);
        return false;
    }
    
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Validating row data for struct: '%s'"), *DataTable->GetRowStruct()->GetName());
    
    // Auto-fill missing properties with default values instead of failing
    FillMissingFields(DataTable->GetRowStruct(), RowData);
    
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Row data validation successful"));
    return true;
}

UScriptStruct* FDataTableService::FindStruct(const FString& StructName)
{
    // Try alternative struct names if needed
    TArray<FString> StructNameVariations;
    
    // First try the direct name if it's already a full path
    if (StructName.StartsWith(TEXT("/Game/")))
    {
        StructNameVariations.Add(StructName);
    }
    else if (StructName.StartsWith(TEXT("/Script/")))
    {
        StructNameVariations.Add(StructName);
    }
    else
    {
        // Try engine and core paths first
        StructNameVariations.Add(FUnrealMCPCommonUtils::BuildEnginePath(StructName));
        StructNameVariations.Add(FUnrealMCPCommonUtils::BuildCorePath(StructName));
        
        // Then try game paths - ensure no double slashes
        FString GamePath = FUnrealMCPCommonUtils::GetGameContentPath();
        if (!GamePath.EndsWith(TEXT("/")))
        {
            GamePath += TEXT("/");
        }
        
        // Try in Blueprints folder
        StructNameVariations.Add(FString::Printf(TEXT("%sBlueprints/%s.%s"), *GamePath, *StructName, *StructName));
        
        // Try in Data folder
        StructNameVariations.Add(FString::Printf(TEXT("%sData/%s.%s"), *GamePath, *StructName, *StructName));
        
        // Try direct in Game folder
        StructNameVariations.Add(FString::Printf(TEXT("%s%s.%s"), *GamePath, *StructName, *StructName));
    }
    
    // Store tried paths for error reporting
    TriedStructPaths = StructNameVariations;
    
    // Try each variation of the struct name
    for (const FString& StructVariation : StructNameVariations)
    {
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Trying to find struct with name: '%s'"), *StructVariation);
        UScriptStruct* FoundStruct = LoadObject<UScriptStruct>(nullptr, *StructVariation);
        if (FoundStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Successfully found struct: '%s'"), *StructVariation);
            return FoundStruct;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Could not find struct: '%s'"), *StructVariation);
        }
    }
    
    UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Failed to find any struct matching: '%s'"), *StructName);
    return nullptr;
}

TMap<FString, FString> FDataTableService::BuildGuidToStructNameMap(const UScriptStruct* RowStruct)
{
    TMap<FString, FString> Map;
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        FString GuidName = Property->GetName(); // This is the GUID name
        FString StructName = Property->GetAuthoredName(); // This is the original struct name
        Map.Add(GuidName, StructName);
        if (GuidName != StructName)
        {
            UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Mapping GUID property '%s' to struct property '%s'"), *GuidName, *StructName);
        }
    }
    return Map;
}

TSharedPtr<FJsonObject> FDataTableService::TransformJsonToStructNames(const TSharedPtr<FJsonObject>& InJson, const TMap<FString, FString>& GuidToStructMap)
{
    TSharedPtr<FJsonObject> OutJson = MakeShared<FJsonObject>();
    for (const auto& Pair : InJson->Values)
    {
        FString Key = Pair.Key;
        const FString* StructName = GuidToStructMap.Find(Key);
        if (StructName)
        {
            OutJson->SetField(*StructName, Pair.Value);
        }
        else
        {
            OutJson->SetField(Key, Pair.Value); // fallback
        }
    }
    return OutJson;
}

TSharedPtr<FJsonObject> FDataTableService::RowToJson(const UDataTable* DataTable, const FName& RowName)
{
    UE_LOG(LogTemp, Warning, TEXT("=== MCP DataTable: RowToJson START for row '%s' ==="), *RowName.ToString());
    
    TSharedPtr<FJsonObject> RowObj = MakeShared<FJsonObject>();
    RowObj->SetStringField(TEXT("row_name"), RowName.ToString());
    
    // Get the row data
    void* RowPtr = DataTable->FindRowUnchecked(RowName);
    if (RowPtr && DataTable->GetRowStruct())
    {
        UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Found row data, converting UStruct to JSON"));
        TSharedPtr<FJsonObject> RowDataObj = MakeShared<FJsonObject>();
        FJsonObjectConverter::UStructToJsonObject(DataTable->GetRowStruct(), RowPtr, RowDataObj.ToSharedRef());
        
        UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Raw UStruct to JSON resulted in %d fields"), RowDataObj->Values.Num());
        for (const auto& Pair : RowDataObj->Values)
        {
            UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Raw field: '%s' (type: %d)"), *Pair.Key, (int32)Pair.Value->Type);
        }
        
        // Auto-transform GUID field names back to friendly names
        TSharedPtr<FJsonObject> FriendlyRowDataObj = FDataTableTransformationService::AutoTransformFromGuidNames(RowDataObj, DataTable->GetRowStruct());
        
        RowObj->SetObjectField(TEXT("row_data"), FriendlyRowDataObj);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: RowPtr is null or no struct for row '%s'"), *RowName.ToString());
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== MCP DataTable: RowToJson END ==="));
    return RowObj;
}

void FDataTableService::RefreshDataTableEditor(UDataTable* DataTable)
{
#if WITH_EDITOR
    if (GEditor && DataTable)
    {
        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        if (AssetEditorSubsystem)
        {
            AssetEditorSubsystem->CloseAllEditorsForAsset(DataTable);
            AssetEditorSubsystem->OpenEditorForAsset(DataTable);
        }
    }
#endif
}

void FDataTableService::SaveAndSyncDataTable(UDataTable* DataTable)
{
    if (DataTable)
    {
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Attempting to save asset: '%s'"), *DataTable->GetPathName());
        bool bSaved = UEditorAssetLibrary::SaveAsset(DataTable->GetPathName(), false);
        if (bSaved)
        {
            UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Asset saved successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Failed to save asset"));
        }
        
        UEditorAssetLibrary::SyncBrowserToObjects({ DataTable->GetPathName() });
    }
}

bool FDataTableService::DoesAssetExist(const FString& AssetPath)
{
    // Check if asset exists using EditorAssetLibrary
    bool bExists = UEditorAssetLibrary::DoesAssetExist(AssetPath);
    
    if (bExists)
    {
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Asset verification - Asset exists at path: '%s'"), *AssetPath);
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Asset verification - No asset found at path: '%s'"), *AssetPath);
    }
    
    return bExists;
}

FString FDataTableService::GenerateUniqueAssetName(const FString& BaseName, const FString& AssetPath, int32 MaxRetries)
{
    FString UniqueName = BaseName;
    FString TestPath;
    
    // First try the base name
    TestPath = FString::Printf(TEXT("%s/%s"), *AssetPath, *UniqueName);
    if (!DoesAssetExist(TestPath))
    {
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Base name '%s' is available"), *UniqueName);
        return UniqueName;
    }
    
    // If base name exists, try with incremental suffixes
    for (int32 i = 1; i <= MaxRetries; ++i)
    {
        UniqueName = FString::Printf(TEXT("%s_%03d"), *BaseName, i);
        TestPath = FString::Printf(TEXT("%s/%s"), *AssetPath, *UniqueName);
        
        if (!DoesAssetExist(TestPath))
        {
            UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Generated unique name '%s' after %d attempts"), *UniqueName, i);
            return UniqueName;
        }
    }
    
    // If we couldn't find a unique name after MaxRetries, use timestamp-based approach
    FDateTime Now = FDateTime::Now();
    UniqueName = FString::Printf(TEXT("%s_%s"), *BaseName, *Now.ToString(TEXT("%Y%m%d_%H%M%S")));
    
    UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Could not find unique name after %d retries, using timestamp-based name: '%s'"), MaxRetries, *UniqueName);
    return UniqueName;
}

FString FDataTableService::GetTriedStructPaths(const FString& StructName) const
{
    if (TriedStructPaths.Num() == 0)
    {
        return TEXT("No paths were tried");
    }
    
    FString Result = TEXT("[");
    for (int32 i = 0; i < TriedStructPaths.Num(); ++i)
    {
        Result += FString::Printf(TEXT("'%s'"), *TriedStructPaths[i]);
        if (i < TriedStructPaths.Num() - 1)
        {
            Result += TEXT(", ");
        }
    }
    Result += TEXT("]");
    return Result;
}

void FDataTableService::FillMissingFields(const UScriptStruct* RowStruct, const TSharedPtr<FJsonObject>& RowData)
{
    if (!RowStruct || !RowData.IsValid())
    {
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Auto-filling missing fields for struct: '%s'"), *RowStruct->GetName());
    
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        FString PropertyName = Property->GetName();
        
        if (!RowData->HasField(PropertyName))
        {
            // Auto-fill with appropriate default value based on property type
            if (Property->IsA<FBoolProperty>())
            {
                RowData->SetBoolField(PropertyName, false);
                UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Auto-filled bool property '%s' with false"), *PropertyName);
            }
            else if (Property->IsA<FIntProperty>())
            {
                RowData->SetNumberField(PropertyName, 0);
                UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Auto-filled int property '%s' with 0"), *PropertyName);
            }
            else if (Property->IsA<FFloatProperty>())
            {
                RowData->SetNumberField(PropertyName, 0.0f);
                UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Auto-filled float property '%s' with 0.0"), *PropertyName);
            }
            else if (Property->IsA<FStrProperty>())
            {
                RowData->SetStringField(PropertyName, TEXT(""));
                UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Auto-filled string property '%s' with empty string"), *PropertyName);
            }
            else if (Property->IsA<FTextProperty>())
            {
                RowData->SetStringField(PropertyName, TEXT(""));
                UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Auto-filled text property '%s' with empty string"), *PropertyName);
            }
            else if (Property->IsA<FArrayProperty>())
            {
                // Create empty array for array properties
                TArray<TSharedPtr<FJsonValue>> EmptyArray;
                RowData->SetArrayField(PropertyName, EmptyArray);
                UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Auto-filled array property '%s' with empty array"), *PropertyName);
            }
            else if (Property->IsA<FStructProperty>())
            {
                // For struct properties, create empty object
                TSharedPtr<FJsonObject> EmptyStruct = MakeShared<FJsonObject>();
                RowData->SetObjectField(PropertyName, EmptyStruct);
                UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Auto-filled struct property '%s' with empty object"), *PropertyName);
            }
            else
            {
                // For other types, try to set a null value or empty string as fallback
                RowData->SetStringField(PropertyName, TEXT(""));
                UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Auto-filled unknown property type '%s' with empty string"), *PropertyName);
            }
        }
    }
}

TSharedPtr<FJsonObject> FDataTableService::AutoTransformToGuidNames(const TSharedPtr<FJsonObject>& InJson, const UScriptStruct* RowStruct)
{
    // Deprecated: Use FDataTableTransformationService::AutoTransformToGuidNames instead
    return FDataTableTransformationService::AutoTransformToGuidNames(InJson, RowStruct);
}

TSharedPtr<FJsonObject> FDataTableService::AutoTransformFromGuidNames(const TSharedPtr<FJsonObject>& InJson, const UScriptStruct* RowStruct)
{
    // Deprecated: Use FDataTableTransformationService::AutoTransformFromGuidNames instead
    return FDataTableTransformationService::AutoTransformFromGuidNames(InJson, RowStruct);
}
            // Fallback to property name without GUID
