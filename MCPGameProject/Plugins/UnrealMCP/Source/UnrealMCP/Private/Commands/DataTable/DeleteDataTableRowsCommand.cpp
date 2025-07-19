#include "Commands/DataTable/DeleteDataTableRowsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/DataTable.h"

FDeleteDataTableRowsCommand::FDeleteDataTableRowsCommand(IDataTableService& InDataTableService)
    : DataTableService(InDataTableService)
{
}

FString FDeleteDataTableRowsCommand::Execute(const FString& Parameters)
{
    FString DataTableName;
    TArray<FString> RowNames;
    FString ParseError;
    
    if (!ParseParameters(Parameters, DataTableName, RowNames, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the DataTable
    UDataTable* DataTable = DataTableService.FindDataTable(DataTableName);
    if (!DataTable)
    {
        return CreateErrorResponse(FString::Printf(TEXT("DataTable not found: %s"), *DataTableName));
    }
    
    // Delete rows using the service
    TArray<FString> DeletedRows;
    TArray<FString> FailedRows;
    bool bSuccess = DataTableService.DeleteRowsFromDataTable(DataTable, RowNames, DeletedRows, FailedRows);
    
    if (!bSuccess && DeletedRows.Num() == 0)
    {
        return CreateErrorResponse(TEXT("Failed to delete any rows"));
    }
    
    return CreateSuccessResponse(DeletedRows, FailedRows);
}

FString FDeleteDataTableRowsCommand::GetCommandName() const
{
    return TEXT("delete_datatable_rows");
}

bool FDeleteDataTableRowsCommand::ValidateParams(const FString& Parameters) const
{
    FString DataTableName;
    TArray<FString> RowNames;
    FString ParseError;
    
    return ParseParameters(Parameters, DataTableName, RowNames, ParseError);
}

bool FDeleteDataTableRowsCommand::ParseParameters(const FString& JsonString, FString& OutDataTableName, TArray<FString>& OutRowNames, FString& OutError) const
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
    
    // Parse required row_names parameter
    if (!JsonObject->HasField(TEXT("row_names")))
    {
        OutError = TEXT("Missing required 'row_names' parameter");
        return false;
    }
    
    TArray<TSharedPtr<FJsonValue>> RowNamesArray = JsonObject->GetArrayField(TEXT("row_names"));
    OutRowNames.Empty();
    
    for (const TSharedPtr<FJsonValue>& RowNameValue : RowNamesArray)
    {
        OutRowNames.Add(RowNameValue->AsString());
    }
    
    return true;
}

FString FDeleteDataTableRowsCommand::CreateSuccessResponse(const TArray<FString>& DeletedRows, const TArray<FString>& FailedRows) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    
    TArray<TSharedPtr<FJsonValue>> DeletedRowsJson;
    for (const FString& RowName : DeletedRows)
    {
        DeletedRowsJson.Add(MakeShared<FJsonValueString>(RowName));
    }
    ResponseObj->SetArrayField(TEXT("deleted_rows"), DeletedRowsJson);
    
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

FString FDeleteDataTableRowsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}