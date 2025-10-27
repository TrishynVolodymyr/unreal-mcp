#include "Commands/DataTable/DeleteDataTableRowsCommand.h"
#include "Services/IDataTableService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/DataTable.h"

FDeleteDataTableRowsCommand::FDeleteDataTableRowsCommand(TSharedPtr<IDataTableService> InDataTableService)
    : DataTableService(InDataTableService)
{
}

FString FDeleteDataTableRowsCommand::Execute(const FString& Parameters)
{
    UE_LOG(LogTemp, Warning, TEXT("=== NEW ARCHITECTURE PROOF ==="));
    UE_LOG(LogTemp, Warning, TEXT("FDeleteDataTableRowsCommand::Execute: NEW ARCHITECTURE COMMAND CALLED!"));
    UE_LOG(LogTemp, Warning, TEXT("Using service layer delegation instead of direct legacy calls"));
    
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Invalid JSON parameters"));
        TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(Error);
        
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Use internal execution with JSON objects
    TSharedPtr<FJsonObject> Response = ExecuteInternal(JsonObject);
    
    // Convert response back to string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
    return OutputString;
}

FString FDeleteDataTableRowsCommand::GetCommandName() const
{
    return TEXT("delete_datatable_rows");
}

bool FDeleteDataTableRowsCommand::ValidateParams(const FString& Parameters) const
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString ValidationError;
    return ValidateParamsInternal(JsonObject, ValidationError);
}

TSharedPtr<FJsonObject> FDeleteDataTableRowsCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
    if (!DataTableService.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("NEW ARCHITECTURE: DataTableService is not valid!"));
        FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("DataTable service is not available"));
        return CreateErrorResponse(Error);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("NEW ARCHITECTURE: DataTableService is valid, proceeding with service call"));

    // Validate parameters using the new validation framework
    FString ValidationError;
    if (!ValidateParamsInternal(Params, ValidationError))
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(ValidationError);
        return CreateErrorResponse(Error);
    }

    // Extract parameters
    FString DataTablePath = Params->GetStringField(TEXT("datatable_path"));
    TArray<TSharedPtr<FJsonValue>> RowNamesArray = Params->GetArrayField(TEXT("row_names"));
    
    TArray<FString> RowNames;
    for (const TSharedPtr<FJsonValue>& RowNameValue : RowNamesArray)
    {
        RowNames.Add(RowNameValue->AsString());
    }
    
    // Find the DataTable using the service
    UDataTable* DataTable = DataTableService->FindDataTable(DataTablePath);
    if (!DataTable)
    {
        FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(FString::Printf(TEXT("DataTable not found: %s"), *DataTablePath));
        return CreateErrorResponse(Error);
    }
    
    // Delete rows using the service
    TArray<FString> DeletedRows;
    TArray<FString> FailedRows;
    bool bSuccess = DataTableService->DeleteRowsFromDataTable(DataTable, RowNames, DeletedRows, FailedRows);
    
    // Always return success response with details about deleted/failed rows
    // This allows the client to see which rows failed and why
    return CreateSuccessResponse(DeletedRows, FailedRows);
}

bool FDeleteDataTableRowsCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
    // Validate required datatable_path parameter
    if (!Params->HasField(TEXT("datatable_path")) || Params->GetStringField(TEXT("datatable_path")).IsEmpty())
    {
        OutError = TEXT("Missing or empty required 'datatable_path' parameter");
        return false;
    }
    
    // Validate required row_names parameter
    if (!Params->HasField(TEXT("row_names")))
    {
        OutError = TEXT("Missing required 'row_names' parameter");
        return false;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* RowNamesArray;
    if (!Params->TryGetArrayField(TEXT("row_names"), RowNamesArray))
    {
        OutError = TEXT("'row_names' parameter must be an array");
        return false;
    }
    
    if (RowNamesArray->Num() == 0)
    {
        OutError = TEXT("'row_names' array cannot be empty");
        return false;
    }
    
    // Validate that all row names are strings
    for (const TSharedPtr<FJsonValue>& RowNameValue : *RowNamesArray)
    {
        if (RowNameValue->Type != EJson::String || RowNameValue->AsString().IsEmpty())
        {
            OutError = TEXT("All row names must be non-empty strings");
            return false;
        }
    }
    
    return true;
}

TSharedPtr<FJsonObject> FDeleteDataTableRowsCommand::CreateSuccessResponse(const TArray<FString>& DeletedRows, const TArray<FString>& FailedRows) const
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
    
    return ResponseObj;
}

TSharedPtr<FJsonObject> FDeleteDataTableRowsCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetNumberField(TEXT("error_code"), Error.ErrorCode);
    
    if (!Error.ErrorDetails.IsEmpty())
    {
        ResponseObj->SetStringField(TEXT("details"), Error.ErrorDetails);
    }
    
    return ResponseObj;
}

