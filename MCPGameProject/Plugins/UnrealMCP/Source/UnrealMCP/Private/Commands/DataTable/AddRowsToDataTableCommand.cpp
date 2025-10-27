#include "Commands/DataTable/AddRowsToDataTableCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/DataTable.h"
#include "MCPErrorHandler.h"

FAddRowsToDataTableCommand::FAddRowsToDataTableCommand(IDataTableService& InDataTableService)
    : DataTableService(InDataTableService)
{
}

FString FAddRowsToDataTableCommand::Execute(const FString& Parameters)
{
    // First validate parameters using the validation framework
    if (!ValidateParams(Parameters))
    {
        FMCPError ValidationError = FMCPErrorHandler::CreateValidationFailedError(
            TEXT("Parameter validation failed for add_rows_to_datatable command")
        );
        FMCPErrorHandler::LogError(ValidationError);
        return CreateErrorResponse(TEXT("Invalid parameters for command 'add_rows_to_datatable'"));
    }
    
    // Parse parameters
    FString DataTablePath;
    TArray<FDataTableRowParams> Rows;
    FString ParseError;
    
    if (!ParseParameters(Parameters, DataTablePath, Rows, ParseError))
    {
        FMCPError ParseErrorObj = FMCPErrorHandler::CreateInvalidParametersError(
            FString::Printf(TEXT("Failed to parse parameters: %s"), *ParseError)
        );
        FMCPErrorHandler::LogError(ParseErrorObj);
        return CreateErrorResponse(ParseError);
    }
    
    // Find the DataTable
    UDataTable* DataTable = DataTableService.FindDataTable(DataTablePath);
    if (!DataTable)
    {
        FMCPError NotFoundError = FMCPErrorHandler::CreateExecutionFailedError(
            FString::Printf(TEXT("DataTable not found: %s"), *DataTablePath)
        );
        FMCPErrorHandler::LogError(NotFoundError);
        return CreateErrorResponse(FString::Printf(TEXT("DataTable not found: %s"), *DataTablePath));
    }
    
    // Add rows using the service
    TArray<FString> AddedRows;
    TArray<FString> FailedRows;
    bool bSuccess = DataTableService.AddRowsToDataTable(DataTable, Rows, AddedRows, FailedRows);
    
    if (!bSuccess && AddedRows.Num() == 0)
    {
        FMCPError ExecutionError = FMCPErrorHandler::CreateExecutionFailedError(
            TEXT("Failed to add any rows to DataTable")
        );
        FMCPErrorHandler::LogError(ExecutionError);
        return CreateErrorResponse(TEXT("Failed to add any rows"));
    }
    
    // Log successful operation
    UE_LOG(LogTemp, Log, TEXT("MCP DataTable: Successfully added %d rows to DataTable '%s'"), 
           AddedRows.Num(), *DataTablePath);
    
    return CreateSuccessResponse(AddedRows, FailedRows);
}

FString FAddRowsToDataTableCommand::GetCommandName() const
{
    return TEXT("add_rows_to_datatable");
}

bool FAddRowsToDataTableCommand::ValidateParams(const FString& Parameters) const
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }
    
    // Basic parameter validation - datatable_path is required
    FString DataTablePath;
    if (!JsonObject->TryGetStringField(TEXT("datatable_path"), DataTablePath) || DataTablePath.IsEmpty())
    {
        return false;
    }
    
    // Rows parameter is required and must be an array
    const TArray<TSharedPtr<FJsonValue>>* RowsArray;
    if (!JsonObject->TryGetArrayField(TEXT("rows"), RowsArray))
    {
        return false;
    }
    
    // Validate each row object structure
    for (const TSharedPtr<FJsonValue>& RowValue : *RowsArray)
    {
        TSharedPtr<FJsonObject> RowObj = RowValue->AsObject();
        if (!RowObj.IsValid())
        {
            return false;
        }
        
        // Validate row_name field
        FString RowName;
        if (!RowObj->TryGetStringField(TEXT("row_name"), RowName) || RowName.IsEmpty())
        {
            return false;
        }
        
        // Validate row_data field
        if (!RowObj->HasField(TEXT("row_data")))
        {
            return false;
        }
        
        TSharedPtr<FJsonObject> RowData = RowObj->GetObjectField(TEXT("row_data"));
        if (!RowData.IsValid())
        {
            return false;
        }
    }
    
    return true;
}

bool FAddRowsToDataTableCommand::ParseParameters(const FString& JsonString, FString& OutDataTablePath, TArray<FDataTableRowParams>& OutRows, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required datatable_path parameter
    if (!JsonObject->TryGetStringField(TEXT("datatable_path"), OutDataTablePath))
    {
        OutError = TEXT("Missing required 'datatable_path' parameter");
        return false;
    }
    
    // Parse required rows parameter
    if (!JsonObject->HasField(TEXT("rows")))
    {
        OutError = TEXT("Missing required 'rows' parameter");
        return false;
    }
    
    TArray<TSharedPtr<FJsonValue>> RowsArray = JsonObject->GetArrayField(TEXT("rows"));
    OutRows.Empty();
    
    for (const TSharedPtr<FJsonValue>& RowValue : RowsArray)
    {
        TSharedPtr<FJsonObject> RowObj = RowValue->AsObject();
        if (!RowObj.IsValid())
        {
            OutError = TEXT("Invalid row object in rows array");
            return false;
        }
        
        FDataTableRowParams RowParams;
        
        // Parse row_name
        if (!RowObj->TryGetStringField(TEXT("row_name"), RowParams.RowName))
        {
            OutError = TEXT("Missing 'row_name' in row object");
            return false;
        }
        
        // Parse row_data
        if (!RowObj->HasField(TEXT("row_data")))
        {
            OutError = TEXT("Missing 'row_data' in row object");
            return false;
        }
        
        RowParams.RowData = RowObj->GetObjectField(TEXT("row_data"));
        if (!RowParams.RowData.IsValid())
        {
            OutError = TEXT("Invalid 'row_data' in row object");
            return false;
        }
        
        OutRows.Add(RowParams);
    }
    
    return true;
}

FString FAddRowsToDataTableCommand::CreateSuccessResponse(const TArray<FString>& AddedRows, const TArray<FString>& FailedRows) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("command"), GetCommandName());
    
    // Add successfully added rows
    TArray<TSharedPtr<FJsonValue>> AddedRowsJson;
    for (const FString& RowName : AddedRows)
    {
        AddedRowsJson.Add(MakeShared<FJsonValueString>(RowName));
    }
    ResponseObj->SetArrayField(TEXT("added_rows"), AddedRowsJson);
    
    // Add failed rows if any
    if (FailedRows.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> FailedRowsJson;
        for (const FString& FailedRow : FailedRows)
        {
            FailedRowsJson.Add(MakeShared<FJsonValueString>(FailedRow));
        }
        ResponseObj->SetArrayField(TEXT("failed_rows"), FailedRowsJson);
    }
    
    // Add metadata
    TSharedPtr<FJsonObject> Metadata = MakeShared<FJsonObject>();
    Metadata->SetStringField(TEXT("timestamp"), FDateTime::UtcNow().ToIso8601());
    Metadata->SetStringField(TEXT("operation"), TEXT("add_rows"));
    Metadata->SetNumberField(TEXT("added_count"), AddedRows.Num());
    Metadata->SetNumberField(TEXT("failed_count"), FailedRows.Num());
    ResponseObj->SetObjectField(TEXT("metadata"), Metadata);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FAddRowsToDataTableCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

