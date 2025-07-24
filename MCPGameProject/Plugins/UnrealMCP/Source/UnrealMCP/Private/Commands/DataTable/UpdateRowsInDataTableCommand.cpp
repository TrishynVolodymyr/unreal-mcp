#include "Commands/DataTable/UpdateRowsInDataTableCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/DataTable.h"

FUpdateRowsInDataTableCommand::FUpdateRowsInDataTableCommand(IDataTableService& InDataTableService)
    : DataTableService(InDataTableService)
{
}

FString FUpdateRowsInDataTableCommand::Execute(const FString& Parameters)
{
    FString DataTableName;
    TArray<FDataTableRowParams> Rows;
    FString ParseError;
    
    if (!ParseParameters(Parameters, DataTableName, Rows, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the DataTable
    UDataTable* DataTable = DataTableService.FindDataTable(DataTableName);
    if (!DataTable)
    {
        return CreateErrorResponse(FString::Printf(TEXT("DataTable not found: %s"), *DataTableName));
    }
    
    // Update rows using the service
    TArray<FString> UpdatedRows;
    TArray<FString> FailedRows;
    bool bSuccess = DataTableService.UpdateRowsInDataTable(DataTable, Rows, UpdatedRows, FailedRows);
    
    if (!bSuccess && UpdatedRows.Num() == 0)
    {
        return CreateErrorResponse(TEXT("Failed to update any rows"));
    }
    
    return CreateSuccessResponse(UpdatedRows, FailedRows);
}

FString FUpdateRowsInDataTableCommand::GetCommandName() const
{
    return TEXT("update_rows_in_datatable");
}

bool FUpdateRowsInDataTableCommand::ValidateParams(const FString& Parameters) const
{
    FString DataTableName;
    TArray<FDataTableRowParams> Rows;
    FString ParseError;
    
    return ParseParameters(Parameters, DataTableName, Rows, ParseError);
}

bool FUpdateRowsInDataTableCommand::ParseParameters(const FString& JsonString, FString& OutDataTableName, TArray<FDataTableRowParams>& OutRows, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required datatable_name parameter
    if (!JsonObject->TryGetStringField(TEXT("datatable_name"), OutDataTableName))
    {
        OutError = TEXT("Missing required 'datatable_name' parameter");
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

FString FUpdateRowsInDataTableCommand::CreateSuccessResponse(const TArray<FString>& UpdatedRows, const TArray<FString>& FailedRows) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    
    TArray<TSharedPtr<FJsonValue>> UpdatedRowsJson;
    for (const FString& RowName : UpdatedRows)
    {
        UpdatedRowsJson.Add(MakeShared<FJsonValueString>(RowName));
    }
    ResponseObj->SetArrayField(TEXT("updated_rows"), UpdatedRowsJson);
    
    TArray<TSharedPtr<FJsonValue>> FailedRowsJson;
    for (const FString& RowName : FailedRows)
    {
        FailedRowsJson.Add(MakeShared<FJsonValueString>(RowName));
    }
    ResponseObj->SetArrayField(TEXT("failed_rows"), FailedRowsJson);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FUpdateRowsInDataTableCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}