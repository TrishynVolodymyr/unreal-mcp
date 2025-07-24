#include "Commands/DataTable/CreateDataTableCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/DataTable.h"

FCreateDataTableCommand::FCreateDataTableCommand(IDataTableService& InDataTableService)
    : DataTableService(InDataTableService)
{
}

FString FCreateDataTableCommand::Execute(const FString& Parameters)
{
    FDataTableCreationParams Params;
    FString ParseError;
    
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Validate parameters
    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }
    
    // Create the DataTable using the service
    UDataTable* CreatedDataTable = DataTableService.CreateDataTable(Params);
    if (!CreatedDataTable)
    {
        return CreateErrorResponse(TEXT("Failed to create DataTable"));
    }
    
    return CreateSuccessResponse(CreatedDataTable);
}

FString FCreateDataTableCommand::GetCommandName() const
{
    return TEXT("create_datatable");
}

bool FCreateDataTableCommand::ValidateParams(const FString& Parameters) const
{
    FDataTableCreationParams Params;
    FString ParseError;
    
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FCreateDataTableCommand::ParseParameters(const FString& JsonString, FDataTableCreationParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required datatable_name parameter
    if (!JsonObject->TryGetStringField(TEXT("datatable_name"), OutParams.Name))
    {
        OutError = TEXT("Missing required 'datatable_name' parameter");
        return false;
    }
    
    // Parse required row_struct_name parameter
    if (!JsonObject->TryGetStringField(TEXT("row_struct_name"), OutParams.RowStructName))
    {
        OutError = TEXT("Missing required 'row_struct_name' parameter");
        return false;
    }
    
    // Parse optional path parameter
    JsonObject->TryGetStringField(TEXT("path"), OutParams.Path);
    
    // Parse optional description parameter
    JsonObject->TryGetStringField(TEXT("description"), OutParams.Description);
    
    return true;
}

FString FCreateDataTableCommand::CreateSuccessResponse(UDataTable* DataTable) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("path"), FString::Printf(TEXT("%s/%s"), *DataTable->GetOutermost()->GetName(), *DataTable->GetName()));
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FCreateDataTableCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}