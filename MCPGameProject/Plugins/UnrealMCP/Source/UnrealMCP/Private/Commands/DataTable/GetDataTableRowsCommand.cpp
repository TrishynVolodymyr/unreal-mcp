#include "Commands/DataTable/GetDataTableRowsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/DataTable.h"

FGetDataTableRowsCommand::FGetDataTableRowsCommand(IDataTableService& InDataTableService)
    : DataTableService(InDataTableService)
{
}

FString FGetDataTableRowsCommand::Execute(const FString& Parameters)
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
    
    // Get rows using the service
    TSharedPtr<FJsonObject> RowsData = DataTableService.GetDataTableRows(DataTable, RowNames);
    if (!RowsData.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to get DataTable rows"));
    }
    
    return CreateSuccessResponse(RowsData);
}

FString FGetDataTableRowsCommand::GetCommandName() const
{
    return TEXT("get_datatable_rows");
}

bool FGetDataTableRowsCommand::ValidateParams(const FString& Parameters) const
{
    FString DataTableName;
    TArray<FString> RowNames;
    FString ParseError;
    
    return ParseParameters(Parameters, DataTableName, RowNames, ParseError);
}

bool FGetDataTableRowsCommand::ParseParameters(const FString& JsonString, FString& OutDataTableName, TArray<FString>& OutRowNames, FString& OutError) const
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
    
    // Parse optional row_names parameter
    OutRowNames.Empty();
    if (JsonObject->HasField(TEXT("row_names")))
    {
        TArray<TSharedPtr<FJsonValue>> RowNamesArray = JsonObject->GetArrayField(TEXT("row_names"));
        for (const TSharedPtr<FJsonValue>& RowNameValue : RowNamesArray)
        {
            OutRowNames.Add(RowNameValue->AsString());
        }
    }
    
    return true;
}

FString FGetDataTableRowsCommand::CreateSuccessResponse(const TSharedPtr<FJsonObject>& RowsData) const
{
    // The service already returns a properly formatted JSON object with rows array
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RowsData.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FGetDataTableRowsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}