#include "Commands/DataTable/GetDataTableRowNamesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/DataTable.h"

FGetDataTableRowNamesCommand::FGetDataTableRowNamesCommand(IDataTableService& InDataTableService)
    : DataTableService(InDataTableService)
{
}

FString FGetDataTableRowNamesCommand::Execute(const FString& Parameters)
{
    FString DataTableName;
    FString ParseError;
    
    if (!ParseParameters(Parameters, DataTableName, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the DataTable
    UDataTable* DataTable = DataTableService.FindDataTable(DataTableName);
    if (!DataTable)
    {
        return CreateErrorResponse(FString::Printf(TEXT("DataTable not found: %s"), *DataTableName));
    }
    
    // Get row names and field names using the service
    TArray<FString> RowNames;
    TArray<FString> FieldNames;
    bool bSuccess = DataTableService.GetDataTableRowNames(DataTable, RowNames, FieldNames);
    
    if (!bSuccess)
    {
        return CreateErrorResponse(TEXT("Failed to get DataTable row names"));
    }
    
    return CreateSuccessResponse(RowNames, FieldNames);
}

FString FGetDataTableRowNamesCommand::GetCommandName() const
{
    return TEXT("get_datatable_row_names");
}

bool FGetDataTableRowNamesCommand::ValidateParams(const FString& Parameters) const
{
    FString DataTableName;
    FString ParseError;
    
    return ParseParameters(Parameters, DataTableName, ParseError);
}

bool FGetDataTableRowNamesCommand::ParseParameters(const FString& JsonString, FString& OutDataTableName, FString& OutError) const
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
    
    return true;
}

FString FGetDataTableRowNamesCommand::CreateSuccessResponse(const TArray<FString>& RowNames, const TArray<FString>& FieldNames) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    
    TArray<TSharedPtr<FJsonValue>> RowNamesJson;
    for (const FString& RowName : RowNames)
    {
        RowNamesJson.Add(MakeShared<FJsonValueString>(RowName));
    }
    ResponseObj->SetArrayField(TEXT("row_names"), RowNamesJson);
    
    TArray<TSharedPtr<FJsonValue>> FieldNamesJson;
    for (const FString& FieldName : FieldNames)
    {
        FieldNamesJson.Add(MakeShared<FJsonValueString>(FieldName));
    }
    ResponseObj->SetArrayField(TEXT("field_names"), FieldNamesJson);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FGetDataTableRowNamesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}