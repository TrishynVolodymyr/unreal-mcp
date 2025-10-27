#include "Commands/DataTable/CreateDataTableCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/DataTable.h"
#include "MCPErrorHandler.h"

FCreateDataTableCommand::FCreateDataTableCommand(IDataTableService& InDataTableService)
    : DataTableService(InDataTableService)
{
}

FString FCreateDataTableCommand::Execute(const FString& Parameters)
{
    // First validate parameters using the validation framework
    if (!ValidateParams(Parameters))
    {
        FMCPError ValidationError = FMCPErrorHandler::CreateValidationFailedError(
            TEXT("Parameter validation failed for create_datatable command")
        );
        FMCPErrorHandler::LogError(ValidationError);
        return FMCPErrorHandler::CreateStructuredErrorResponse(ValidationError);
    }
    
    // Parse parameters
    FDataTableCreationParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        FMCPError ParseErrorObj = FMCPErrorHandler::CreateInvalidParametersError(
            FString::Printf(TEXT("Failed to parse parameters: %s"), *ParseError)
        );
        FMCPErrorHandler::LogError(ParseErrorObj);
        return FMCPErrorHandler::CreateStructuredErrorResponse(ParseErrorObj);
    }
    
    // Additional business logic validation
    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        FMCPError BusinessValidationError = FMCPErrorHandler::CreateValidationFailedError(
            FString::Printf(TEXT("Business validation failed: %s"), *ValidationError)
        );
        FMCPErrorHandler::LogError(BusinessValidationError);
        return FMCPErrorHandler::CreateStructuredErrorResponse(BusinessValidationError);
    }
    
    // Create the DataTable using the service
    UDataTable* CreatedDataTable = DataTableService.CreateDataTable(Params);
    if (!CreatedDataTable)
    {
        // Get detailed error message from the service
        FString DetailedError = DataTableService.GetLastErrorMessage();
        if (DetailedError.IsEmpty())
        {
            DetailedError = FString::Printf(TEXT("Failed to create DataTable '%s' with struct '%s'"), *Params.Name, *Params.RowStructName);
        }
        
        FMCPError ExecutionError = FMCPErrorHandler::CreateExecutionFailedError(DetailedError);
        FMCPErrorHandler::LogError(ExecutionError);
        return CreateErrorResponse(DetailedError);
    }
    
    // Log successful creation
    UE_LOG(LogTemp, Log, TEXT("MCP DataTable: Successfully created DataTable '%s' at path '%s'"), 
           *CreatedDataTable->GetName(), *CreatedDataTable->GetPathName());
    
    return CreateSuccessResponse(CreatedDataTable);
}

FString FCreateDataTableCommand::GetCommandName() const
{
    return TEXT("create_datatable");
}

bool FCreateDataTableCommand::ValidateParams(const FString& Parameters) const
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }
    
    // Basic parameter validation
    FString DataTableName;
    if (!JsonObject->TryGetStringField(TEXT("datatable_name"), DataTableName) || DataTableName.IsEmpty())
    {
        return false;
    }
    
    FString RowStructName;
    if (!JsonObject->TryGetStringField(TEXT("row_struct_name"), RowStructName) || RowStructName.IsEmpty())
    {
        return false;
    }
    
    // Additional business logic validation
    FDataTableCreationParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    
    FString BusinessValidationError;
    return Params.IsValid(BusinessValidationError);
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
    ResponseObj->SetStringField(TEXT("command"), GetCommandName());
    
    // DataTable information
    TSharedPtr<FJsonObject> DataTableInfo = MakeShared<FJsonObject>();
    DataTableInfo->SetStringField(TEXT("name"), DataTable->GetName());
    DataTableInfo->SetStringField(TEXT("path"), DataTable->GetPathName());
    DataTableInfo->SetStringField(TEXT("package"), DataTable->GetOutermost()->GetName());
    
    // Struct information
    if (const UScriptStruct* RowStruct = DataTable->GetRowStruct())
    {
        DataTableInfo->SetStringField(TEXT("row_struct"), RowStruct->GetName());
        DataTableInfo->SetStringField(TEXT("row_struct_path"), RowStruct->GetPathName());
    }
    
    // Row count (should be 0 for newly created table)
    DataTableInfo->SetNumberField(TEXT("row_count"), DataTable->GetRowNames().Num());
    
    ResponseObj->SetObjectField(TEXT("datatable"), DataTableInfo);
    
    // Add metadata
    TSharedPtr<FJsonObject> Metadata = MakeShared<FJsonObject>();
    Metadata->SetStringField(TEXT("timestamp"), FDateTime::UtcNow().ToIso8601());
    Metadata->SetStringField(TEXT("operation"), TEXT("create"));
    ResponseObj->SetObjectField(TEXT("metadata"), Metadata);
    
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



