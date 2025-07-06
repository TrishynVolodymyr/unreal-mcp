#include "Commands/UnrealMCPDataTableCommands.h"
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

FUnrealMCPDataTableCommands::FUnrealMCPDataTableCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_datatable"))
    {
        return HandleCreateDataTable(Params);
    }
    else if (CommandType == TEXT("add_rows_to_datatable"))
    {
        return HandleAddRowsToDataTable(Params);
    }
    else if (CommandType == TEXT("get_datatable_rows"))
    {
        return HandleGetDataTableRows(Params);
    }
    
    else if (CommandType == TEXT("get_datatable_property_map"))
    {
        return HandleGetDataTablePropertyMap(Params);
    }
    else if (CommandType == TEXT("get_datatable_row_names"))
    {
        return HandleGetDataTableRowNames(Params);
    }
    else if (CommandType == TEXT("update_rows_in_datatable"))
    {
        return HandleUpdateRowsInDataTable(Params);
    }
    else if (CommandType == TEXT("delete_datatable_rows"))
    {
        return HandleDeleteDataTableRows(Params);
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown DataTable command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleCreateDataTable(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString DataTableName;
    if (!Params->TryGetStringField(TEXT("datatable_name"), DataTableName))
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Missing 'datatable_name' parameter"));
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'datatable_name' parameter"));
    }
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Creating DataTable named '%s'"), *DataTableName);

    FString RowStructName;
    if (!Params->TryGetStringField(TEXT("row_struct_name"), RowStructName))
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Missing 'row_struct_name' parameter"));
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'row_struct_name' parameter"));
    }
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Using struct '%s'"), *RowStructName);

    // Get optional parameters
    FString Path = TEXT("/Game/Data");
    Params->TryGetStringField(TEXT("path"), Path);
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Path set to '%s'"), *Path);

    FString Description;
    Params->TryGetStringField(TEXT("description"), Description);

    // Try alternative struct names if needed
    TArray<FString> StructNameVariations;
    
    // First try the direct name if it's already a full path
    if (RowStructName.StartsWith(TEXT("/Game/")))
    {
        StructNameVariations.Add(RowStructName);
    }
    else if (RowStructName.StartsWith(TEXT("/Script/")))
    {
        StructNameVariations.Add(RowStructName);
    }
    else
    {
        // Try engine and core paths first
        StructNameVariations.Add(FUnrealMCPCommonUtils::BuildEnginePath(RowStructName));
        StructNameVariations.Add(FUnrealMCPCommonUtils::BuildCorePath(RowStructName));
        
        // Then try game paths - ensure no double slashes
        FString GamePath = FUnrealMCPCommonUtils::GetGameContentPath();
        if (!GamePath.EndsWith(TEXT("/")))
        {
            GamePath += TEXT("/");
        }
        
        // Try in Blueprints folder
        StructNameVariations.Add(FString::Printf(TEXT("%sBlueprints/%s.%s"), *GamePath, *RowStructName, *RowStructName));
        
        // Try in Data folder
        StructNameVariations.Add(FString::Printf(TEXT("%sData/%s.%s"), *GamePath, *RowStructName, *RowStructName));
        
        // Try direct in Game folder
        StructNameVariations.Add(FString::Printf(TEXT("%s%s.%s"), *GamePath, *RowStructName, *RowStructName));
    }
    
    // Log all variations we're going to try
    for (const FString& Variation : StructNameVariations)
    {
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Will try to find struct with name: '%s'"), *Variation);
    }
    
    // Create the DataTable
    UDataTableFactory* Factory = NewObject<UDataTableFactory>();
    Factory->Struct = nullptr;
    
    // Try each variation of the struct name
    for (const FString& StructVariation : StructNameVariations)
    {
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Trying to find struct with name: '%s'"), *StructVariation);
        UScriptStruct* FoundStruct = LoadObject<UScriptStruct>(nullptr, *StructVariation);
        if (FoundStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Successfully found struct: '%s'"), *StructVariation);
            Factory->Struct = FoundStruct;
            break;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Could not find struct: '%s'"), *StructVariation);
        }
    }
    
    if (!Factory->Struct)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Failed to find any struct matching: '%s'"), *RowStructName);
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Could not find struct: %s"), *RowStructName));
    }

    // Create the asset using IAssetTools
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    FString FullPath = FString::Printf(TEXT("%s/%s"), *Path, *DataTableName);
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Attempting to create asset at path: '%s'"), *FullPath);
    
    UDataTable* NewDataTable = Cast<UDataTable>(AssetToolsModule.Get().CreateAsset(DataTableName, Path, UDataTable::StaticClass(), Factory));
    
    if (!NewDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: Failed to create DataTable asset"));
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create DataTable"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Successfully created DataTable asset at: '%s'"), *NewDataTable->GetPathName());

    // Note: Metadata setting removed for UE 5.6 compatibility
    if (!Description.IsEmpty())
    {
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Description provided but metadata setting skipped for UE 5.6 compatibility: '%s'"), *Description);
    }

    // Save the asset to disk
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Attempting to save asset: '%s'"), *NewDataTable->GetPathName());
    bool bSaved = UEditorAssetLibrary::SaveAsset(NewDataTable->GetPathName(), false);
    if (bSaved)
    {
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Asset saved successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Failed to save asset"));
    }

    // Return success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("path"), FullPath);
    return ResultObj;
}

// Helper: Map GUID property names to struct property names
static TMap<FString, FString> BuildGuidToStructNameMap(const UScriptStruct* RowStruct) {
    TMap<FString, FString> map;
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt) {
        FProperty* Property = *PropIt;
        FString GuidName = Property->GetName(); // This is the GUID name
        FString StructName = Property->GetAuthoredName(); // This is the original struct name
        map.Add(GuidName, StructName);
        if (GuidName != StructName) {
            UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Mapping GUID property '%s' to struct property '%s'"), *GuidName, *StructName);
        }
    }
    return map;
}

static TSharedPtr<FJsonObject> TransformJsonToStructNames(const TSharedPtr<FJsonObject>& InJson, const TMap<FString, FString>& GuidToStructMap) {
    TSharedPtr<FJsonObject> OutJson = MakeShared<FJsonObject>();
    for (const auto& Pair : InJson->Values) {
        FString Key = Pair.Key;
        const FString* StructName = GuidToStructMap.Find(Key);
        if (StructName) {
            OutJson->SetField(*StructName, Pair.Value);
        } else {
            OutJson->SetField(Key, Pair.Value); // fallback
        }
    }
    return OutJson;
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleAddRowsToDataTable(const TSharedPtr<FJsonObject>& Params)
{
    FString DataTableName;
    if (!Params->TryGetStringField(TEXT("datatable_name"), DataTableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'datatable_name' parameter"));
    }
    if (!Params->HasField(TEXT("rows")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'rows' parameter"));
    }
    TArray<TSharedPtr<FJsonValue>> RowsArray = Params->GetArrayField(TEXT("rows"));
    UDataTable* DataTable = FindDataTable(DataTableName);
    if (!DataTable)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("DataTable not found: %s"), *DataTableName));
    }
    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    if (!RowStruct)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get row struct from DataTable"));
    }
    TArray<FString> AddedRows;
    for (const TSharedPtr<FJsonValue>& RowValue : RowsArray)
    {
        TSharedPtr<FJsonObject> RowObj = RowValue->AsObject();
        if (!RowObj.IsValid()) continue;
        FString RowName;
        if (!RowObj->TryGetStringField(TEXT("row_name"), RowName)) continue;
        if (!RowObj->HasField(TEXT("row_data"))) continue;
        TSharedPtr<FJsonObject> RowData = RowObj->GetObjectField(TEXT("row_data"));
        // Validate row data
        FString ErrorMessage;
        if (!ValidateRowData(DataTable, RowData, ErrorMessage)) continue;
        // Map GUID property names to struct property names and transform JSON
        TMap<FString, FString> GuidToStructMap = BuildGuidToStructNameMap(RowStruct);
        TSharedPtr<FJsonObject> StructJson = TransformJsonToStructNames(RowData, GuidToStructMap);
        {
            FString DebugJsonString;
            TSharedRef<TJsonWriter<>> DebugWriter = TJsonWriterFactory<>::Create(&DebugJsonString);
            FJsonSerializer::Serialize(StructJson.ToSharedRef(), DebugWriter);
            UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Transformed row_data for UStruct: %s (row: %s)"), *DebugJsonString, *RowName);
        }
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
            continue;
        }
        // Dump struct values after conversion
        {
            TSharedPtr<FJsonObject> DebugJson = MakeShared<FJsonObject>();
            FJsonObjectConverter::UStructToJsonObject(RowStruct, NewRow, DebugJson.ToSharedRef());
            FString DebugJsonString;
            TSharedRef<TJsonWriter<>> DebugWriter = TJsonWriterFactory<>::Create(&DebugJsonString);
            FJsonSerializer::Serialize(DebugJson.ToSharedRef(), DebugWriter);
            UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: Struct values after JSON->UStruct: %s (row: %s)"), *DebugJsonString, *RowName);
        }
        DataTable->AddRow(FName(*RowName), *NewRow);
        RowStruct->DestroyStruct(RowMemory);
        FMemory::Free(RowMemory);
        AddedRows.Add(RowName);
        // Trigger DataTable UI refresh in editor
        DataTable->Modify(true);
        DataTable->PostEditChange();
        DataTable->MarkPackageDirty();
    }
    // Save and sync once
    UEditorAssetLibrary::SaveAsset(DataTable->GetPathName(), false);
    UEditorAssetLibrary::SyncBrowserToObjects({ DataTable->GetPathName() });

    // Force DataTable editor UI to refresh by closing and reopening the asset
#if WITH_EDITOR
    if (GEditor)
    {
        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        if (AssetEditorSubsystem)
        {
            AssetEditorSubsystem->CloseAllEditorsForAsset(DataTable);
            AssetEditorSubsystem->OpenEditorForAsset(DataTable);
        }
    }
#endif

    // Return success and list of added rows
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    TArray<TSharedPtr<FJsonValue>> AddedRowsJson;
    for (const FString& Name : AddedRows) { AddedRowsJson.Add(MakeShared<FJsonValueString>(Name)); }
    ResultObj->SetArrayField(TEXT("added_rows"), AddedRowsJson);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleGetDataTableRows(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString DataTableName;
    if (!Params->TryGetStringField(TEXT("datatable_name"), DataTableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'datatable_name' parameter"));
    }

    // Find the DataTable
    UDataTable* DataTable = FindDataTable(DataTableName);
    if (!DataTable)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("DataTable not found: %s"), *DataTableName));
    }

    // Get specific row names if provided
    TArray<FString> RowNames;
    if (Params->HasField(TEXT("row_names")))
    {
        TArray<TSharedPtr<FJsonValue>> RowNamesArray = Params->GetArrayField(TEXT("row_names"));
        for (const TSharedPtr<FJsonValue>& RowNameValue : RowNamesArray)
        {
            RowNames.Add(RowNameValue->AsString());
        }
    }

    // Get rows
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

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleGetDataTablePropertyMap(const TSharedPtr<FJsonObject>& Params)
{
    FString DataTableName;
    if (!Params->TryGetStringField(TEXT("datatable_name"), DataTableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'datatable_name' parameter"));
    }

    UDataTable* DataTable = FindDataTable(DataTableName);
    if (!DataTable)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("DataTable not found: %s"), *DataTableName));
    }

    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    if (!RowStruct)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get row struct from DataTable"));
    }

    TSharedPtr<FJsonObject> MappingObj = MakeShared<FJsonObject>();
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        FString InternalName = Property->GetName();
        FString DisplayName = Property->GetAuthoredName(); // This is usually the user-facing name
        MappingObj->SetStringField(DisplayName, InternalName);
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetObjectField(TEXT("property_map"), MappingObj);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleGetDataTableRowNames(const TSharedPtr<FJsonObject>& Params)
{
    FString DataTableName;
    if (!Params->TryGetStringField(TEXT("datatable_name"), DataTableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'datatable_name' parameter"));
    }

    UDataTable* DataTable = FindDataTable(DataTableName);
    if (!DataTable)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("DataTable not found: %s"), *DataTableName));
    }

    // Get row names
    TArray<FName> RowNames = DataTable->GetRowNames();
    TArray<TSharedPtr<FJsonValue>> RowNamesArray;
    for (const FName& RowName : RowNames)
    {
        RowNamesArray.Add(MakeShared<FJsonValueString>(RowName.ToString()));
    }

    // Get field (struct property) names
    TArray<TSharedPtr<FJsonValue>> FieldNamesArray;
    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    if (RowStruct)
    {
        for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
        {
            FProperty* Property = *PropIt;
            FieldNamesArray.Add(MakeShared<FJsonValueString>(Property->GetName()));
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetArrayField(TEXT("row_names"), RowNamesArray);
    ResultObj->SetArrayField(TEXT("field_names"), FieldNamesArray);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleUpdateRowsInDataTable(const TSharedPtr<FJsonObject>& Params)
{
    FString DataTableName;
    if (!Params->TryGetStringField(TEXT("datatable_name"), DataTableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'datatable_name' parameter"));
    }
    if (!Params->HasField(TEXT("rows")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'rows' parameter"));
    }
    TArray<TSharedPtr<FJsonValue>> RowsArray = Params->GetArrayField(TEXT("rows"));
    UDataTable* DataTable = FindDataTable(DataTableName);
    if (!DataTable)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("DataTable not found: %s"), *DataTableName));
    }
    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    if (!RowStruct)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get row struct from DataTable"));
    }
    TArray<FString> UpdatedRows;
    TArray<FString> FailedRows;
    for (const TSharedPtr<FJsonValue>& RowValue : RowsArray)
    {
        TSharedPtr<FJsonObject> RowObj = RowValue->AsObject();
        if (!RowObj.IsValid()) { FailedRows.Add(TEXT("Invalid row object")); continue; }
        FString RowName;
        if (!RowObj->TryGetStringField(TEXT("row_name"), RowName)) { FailedRows.Add(TEXT("Missing row_name")); continue; }
        if (!RowObj->HasField(TEXT("row_data"))) { FailedRows.Add(RowName + TEXT(": missing row_data")); continue; }
        TSharedPtr<FJsonObject> RowData = RowObj->GetObjectField(TEXT("row_data"));
        // Check if row exists
        if (!DataTable->GetRowMap().Contains(FName(*RowName))) { FailedRows.Add(RowName + TEXT(": not found")); continue; }
        // Validate row data
        FString ErrorMessage;
        if (!ValidateRowData(DataTable, RowData, ErrorMessage)) { FailedRows.Add(RowName + TEXT(": ") + ErrorMessage); continue; }
        // Map GUID property names to struct property names and transform JSON
        TMap<FString, FString> GuidToStructMap = BuildGuidToStructNameMap(RowStruct);
        TSharedPtr<FJsonObject> StructJson = TransformJsonToStructNames(RowData, GuidToStructMap);
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
            FailedRows.Add(RowName + TEXT(": failed to convert JSON to UStruct"));
            continue;
        }
        // Use AddRow to update the row
        DataTable->AddRow(FName(*RowName), *NewRow);
        // Notify DataTable of the change
        DataTable->HandleDataTableChanged(FName(*RowName));
        RowStruct->DestroyStruct(RowMemory);
        FMemory::Free(RowMemory);
        UpdatedRows.Add(RowName);
    }
    // Debug: Log in-memory values after update
    for (const FString& RowName : UpdatedRows) {
        void* RowPtr = DataTable->FindRowUnchecked(FName(*RowName));
        if (RowPtr) {
            TSharedPtr<FJsonObject> DebugJson = MakeShared<FJsonObject>();
            FJsonObjectConverter::UStructToJsonObject(RowStruct, RowPtr, DebugJson.ToSharedRef());
            FString DebugJsonString;
            TSharedRef<TJsonWriter<>> DebugWriter = TJsonWriterFactory<>::Create(&DebugJsonString);
            FJsonSerializer::Serialize(DebugJson.ToSharedRef(), DebugWriter);
            UE_LOG(LogTemp, Warning, TEXT("MCP DataTable: In-memory row after update: %s = %s"), *RowName, *DebugJsonString);
        }
    }
    // Mark dirty and refresh editor UI
    DataTable->Modify(true);
    DataTable->PostEditChange();
    DataTable->MarkPackageDirty();
#if WITH_EDITOR
    if (GEditor)
    {
        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        if (AssetEditorSubsystem)
        {
            AssetEditorSubsystem->CloseAllEditorsForAsset(DataTable);
            AssetEditorSubsystem->OpenEditorForAsset(DataTable);
        }
    }
#endif
    // Save and sync once
    UEditorAssetLibrary::SaveAsset(DataTable->GetPathName(), false);
    UEditorAssetLibrary::SyncBrowserToObjects({ DataTable->GetPathName() });
    // Return success and list of updated/failed rows
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    TArray<TSharedPtr<FJsonValue>> UpdatedRowsJson;
    for (const FString& Name : UpdatedRows) { UpdatedRowsJson.Add(MakeShared<FJsonValueString>(Name)); }
    ResultObj->SetArrayField(TEXT("updated_rows"), UpdatedRowsJson);
    TArray<TSharedPtr<FJsonValue>> FailedRowsJson;
    for (const FString& Name : FailedRows) { FailedRowsJson.Add(MakeShared<FJsonValueString>(Name)); }
    ResultObj->SetArrayField(TEXT("failed_rows"), FailedRowsJson);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::HandleDeleteDataTableRows(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString DataTableName;
    if (!Params->TryGetStringField(TEXT("datatable_name"), DataTableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'datatable_name' parameter"));
    }
    if (!Params->HasField(TEXT("row_names")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'row_names' parameter"));
    }
    TArray<TSharedPtr<FJsonValue>> RowNamesArray = Params->GetArrayField(TEXT("row_names"));
    UDataTable* DataTable = FindDataTable(DataTableName);
    if (!DataTable)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("DataTable not found: %s"), *DataTableName));
    }
    TArray<FString> DeletedRows;
    TArray<FString> FailedRows;
    for (const TSharedPtr<FJsonValue>& RowNameValue : RowNamesArray)
    {
        FString RowName = RowNameValue->AsString();
        if (DataTable->GetRowMap().Contains(FName(*RowName)))
        {
            DataTable->RemoveRow(FName(*RowName));
            DeletedRows.Add(RowName);
        }
        else
        {
            FailedRows.Add(RowName);
        }
    }
    // Save and sync once
    UEditorAssetLibrary::SaveAsset(DataTable->GetPathName(), false);
    UEditorAssetLibrary::SyncBrowserToObjects({ DataTable->GetPathName() });

    // Force DataTable editor UI to refresh by closing and reopening the asset
#if WITH_EDITOR
    if (GEditor)
    {
        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        if (AssetEditorSubsystem)
        {
            AssetEditorSubsystem->CloseAllEditorsForAsset(DataTable);
            AssetEditorSubsystem->OpenEditorForAsset(DataTable);
        }
    }
#endif
    // Return success and lists
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    TArray<TSharedPtr<FJsonValue>> DeletedRowsJson;
    for (const FString& Name : DeletedRows) { DeletedRowsJson.Add(MakeShared<FJsonValueString>(Name)); }
    ResultObj->SetArrayField(TEXT("deleted_rows"), DeletedRowsJson);
    TArray<TSharedPtr<FJsonValue>> FailedRowsJson;
    for (const FString& Name : FailedRows) { FailedRowsJson.Add(MakeShared<FJsonValueString>(Name)); }
    ResultObj->SetArrayField(TEXT("failed_rows"), FailedRowsJson);
    return ResultObj;
}

UDataTable* FUnrealMCPDataTableCommands::FindDataTable(const FString& DataTableName)
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

bool FUnrealMCPDataTableCommands::ValidateRowData(const UDataTable* DataTable, const TSharedPtr<FJsonObject>& RowData, FString& OutErrorMessage)
{
    if (!DataTable || !DataTable->GetRowStruct())
    {
        OutErrorMessage = TEXT("Invalid DataTable or row struct");
        UE_LOG(LogTemp, Error, TEXT("MCP DataTable: %s"), *OutErrorMessage);
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Validating row data for struct: '%s'"), *DataTable->GetRowStruct()->GetName());
    
    // Log the JSON data we're trying to validate
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(RowData.ToSharedRef(), Writer);
    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Row data to validate: %s"), *JsonString);
    
    // Get all properties in the struct
    TArray<FString> StructPropertyNames;
    for (TFieldIterator<FProperty> PropIt(DataTable->GetRowStruct()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        StructPropertyNames.Add(Property->GetName());
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Struct has property: '%s' of type '%s'"), 
               *Property->GetName(), *Property->GetCPPType());
    }
    
    // Log all fields in the JSON data
    TArray<FString> JsonFieldNames;
    RowData->Values.GetKeys(JsonFieldNames);
    for (const FString& FieldName : JsonFieldNames)
    {
        UE_LOG(LogTemp, Display, TEXT("MCP DataTable: JSON contains field: '%s'"), *FieldName);
    }

    // Check if all required properties are present
    for (TFieldIterator<FProperty> PropIt(DataTable->GetRowStruct()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (!RowData->HasField(Property->GetName()))
        {
            OutErrorMessage = FString::Printf(TEXT("Missing required property: %s"), *Property->GetName());
            UE_LOG(LogTemp, Error, TEXT("MCP DataTable: %s"), *OutErrorMessage);
            return false;
        }
    }

    UE_LOG(LogTemp, Display, TEXT("MCP DataTable: Row data validation successful"));
    return true;
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::DataTableToJson(const UDataTable* DataTable)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), DataTable->GetName());
    ResultObj->SetStringField(TEXT("path"), DataTable->GetPathName());
    ResultObj->SetStringField(TEXT("row_struct"), DataTable->GetRowStruct()->GetName());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPDataTableCommands::RowToJson(const UDataTable* DataTable, const FName& RowName)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), RowName.ToString());

    const FTableRowBase* Row = DataTable->FindRow<FTableRowBase>(RowName, TEXT(""));
    if (Row)
    {
        TSharedPtr<FJsonObject> RowData = MakeShared<FJsonObject>();
        FJsonObjectConverter::UStructToJsonObject(DataTable->GetRowStruct(), Row, RowData.ToSharedRef());
        ResultObj->SetObjectField(TEXT("data"), RowData);
    }

    return ResultObj;
} 