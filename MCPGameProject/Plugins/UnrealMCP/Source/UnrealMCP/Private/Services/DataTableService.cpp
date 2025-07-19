#include "Services/DataTableService.h"
#include "Engine/DataTable.h"
#include "UObject/ConstructorHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/DataTableFactory.h"
#include "EditorScriptingUtilities/Public/EditorAssetLibrary.h"
#include "AssetToolsModule.h"
#include "JsonObjectConverter.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/MetaData.h"

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
        return nullptr;
    }
    
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Creating DataTable named '%s'"), *Params.Name);
    
    // Find the struct
    UScriptStruct* FoundStruct = FindStruct(Params.RowStructName);
    if (!FoundStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Failed to find struct: %s"), *Params.RowStructName);
        return nullptr;
    }
    
    // Create the DataTable using factory
    UDataTableFactory* Factory = NewObject<UDataTableFactory>();
    Factory->Struct = FoundStruct;
    
    // Create the asset using IAssetTools
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    FString FullPath = FString::Printf(TEXT("%s/%s"), *Params.Path, *Params.Name);
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Attempting to create asset at path: '%s'"), *FullPath);
    
    UDataTable* NewDataTable = Cast<UDataTable>(AssetToolsModule.Get().CreateAsset(Params.Name, Params.Path, UDataTable::StaticClass(), Factory));
    
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
        
        // Validate row data
        if (!ValidateRowData(DataTable, RowParams.RowData, ValidationError))
        {
            OutFailedRows.Add(FString::Printf(TEXT("%s: %s"), *RowParams.RowName, *ValidationError));
            continue;
        }
        
        // Map GUID property names to struct property names and transform JSON
        TMap<FString, FString> GuidToStructMap = BuildGuidToStructNameMap(RowStruct);
        TSharedPtr<FJsonObject> StructJson = TransformJsonToStructNames(RowParams.RowData, GuidToStructMap);
        
        // Allocate memory for the new row
        uint8* RowMemory = (uint8*)FMemory::Malloc(RowStruct->GetStructureSize());
        RowStruct->InitializeStruct(RowMemory);
        FTableRowBase* NewRow = reinterpret_cast<FTableRowBase*>(RowMemory);
        
        TSharedRef<FJsonObject> JsonRef = StructJson.ToSharedRef();
        bool bJsonConverted = FJsonObjectConverter::JsonObjectToUStruct(JsonRef, RowStruct, RowMemory);
        
        if (!bJsonConverted)
        {
            RowStruct->DestroyStruct(RowMemory);
            FMemory::Free(RowMemory);
            OutFailedRows.Add(FString::Printf(TEXT("%s: failed to convert JSON to UStruct"), *RowParams.RowName));
            continue;
        }
        
        DataTable->AddRow(FName(*RowParams.RowName), *NewRow);
        
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
        
        // Validate row data
        if (!ValidateRowData(DataTable, RowParams.RowData, ValidationError))
        {
            OutFailedRows.Add(FString::Printf(TEXT("%s: %s"), *RowParams.RowName, *ValidationError));
            continue;
        }
        
        // Map GUID property names to struct property names and transform JSON
        TMap<FString, FString> GuidToStructMap = BuildGuidToStructNameMap(RowStruct);
        TSharedPtr<FJsonObject> StructJson = TransformJsonToStructNames(RowParams.RowData, GuidToStructMap);
        
        // Allocate memory for the new row
        uint8* RowMemory = (uint8*)FMemory::Malloc(RowStruct->GetStructureSize());
        RowStruct->InitializeStruct(RowMemory);
        FTableRowBase* NewRow = reinterpret_cast<FTableRowBase*>(RowMemory);
        
        TSharedRef<FJsonObject> JsonRef = StructJson.ToSharedRef();
        bool bJsonConverted = FJsonObjectConverter::JsonObjectToUStruct(JsonRef, RowStruct, RowMemory);
        
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
    
    OutDeletedRows.Empty();
    OutFailedRows.Empty();
    
    for (const FString& RowName : RowNames)
    {
        if (DataTable->GetRowMap().Contains(FName(*RowName)))
        {
            DataTable->RemoveRow(FName(*RowName));
            OutDeletedRows.Add(RowName);
        }
        else
        {
            OutFailedRows.Add(RowName);
        }
    }
    
    if (OutDeletedRows.Num() > 0)
    {
        SaveAndSyncDataTable(DataTable);
        RefreshDataTableEditor(DataTable);
        return true;
    }
    
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
    
    // Check if all required properties are present
    for (TFieldIterator<FProperty> PropIt(DataTable->GetRowStruct()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (!RowData->HasField(Property->GetName()))
        {
            OutError = FString::Printf(TEXT("Missing required property: %s"), *Property->GetName());
            UE_LOG(LogTemp, Error, TEXT("MCP DataTable: %s"), *OutError);
            return false;
        }
    }
    
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
    TSharedPtr<FJsonObject> RowObj = MakeShared<FJsonObject>();
    RowObj->SetStringField(TEXT("row_name"), RowName.ToString());
    
    // Get the row data
    void* RowPtr = DataTable->FindRowUnchecked(RowName);
    if (RowPtr && DataTable->GetRowStruct())
    {
        TSharedPtr<FJsonObject> RowDataObj = MakeShared<FJsonObject>();
        FJsonObjectConverter::UStructToJsonObject(DataTable->GetRowStruct(), RowPtr, RowDataObj.ToSharedRef());
        RowObj->SetObjectField(TEXT("row_data"), RowDataObj);
    }
    
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