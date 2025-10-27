#include "Commands/DataTable/GetDataTablePropertyMapCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/DataTable.h"

FGetDataTablePropertyMapCommand::FGetDataTablePropertyMapCommand(IDataTableService& InDataTableService)
    : DataTableService(InDataTableService)
{
}

FString FGetDataTablePropertyMapCommand::Execute(const FString& Parameters)
{
    FString DataTablePath;
    FString ParseError;
    
    if (!ParseParameters(Parameters, DataTablePath, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the DataTable
    UDataTable* DataTable = DataTableService.FindDataTable(DataTablePath);
    if (!DataTable)
    {
        return CreateErrorResponse(FString::Printf(TEXT("DataTable not found: %s"), *DataTablePath));
    }
    
    // Get property map using the service
    TSharedPtr<FJsonObject> PropertyMap = DataTableService.GetDataTablePropertyMap(DataTable);
    if (!PropertyMap.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to get DataTable property map"));
    }
    
    return CreateSuccessResponse(PropertyMap);
}

FString FGetDataTablePropertyMapCommand::GetCommandName() const
{
    return TEXT("get_datatable_property_map");
}

bool FGetDataTablePropertyMapCommand::ValidateParams(const FString& Parameters) const
{
    FString DataTableName;
    FString ParseError;
    
    return ParseParameters(Parameters, DataTableName, ParseError);
}

bool FGetDataTablePropertyMapCommand::ParseParameters(const FString& JsonString, FString& OutDataTablePath, FString& OutError) const
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
    
    return true;
}

FString FGetDataTablePropertyMapCommand::CreateSuccessResponse(const TSharedPtr<FJsonObject>& PropertyMap) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("property_map"), PropertyMap);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FGetDataTablePropertyMapCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

