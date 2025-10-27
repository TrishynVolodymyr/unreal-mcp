#include "Commands/DataTable/GetDataTableRowsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/DataTable.h"
#include "MCPErrorHandler.h"

FGetDataTableRowsCommand::FGetDataTableRowsCommand(IDataTableService& InDataTableService)
    : DataTableService(InDataTableService)
{
}

FString FGetDataTableRowsCommand::Execute(const FString& Parameters)
{
    // First validate parameters using the validation framework
    if (!ValidateParams(Parameters))
    {
        FMCPError ValidationError = FMCPErrorHandler::CreateValidationFailedError(
            TEXT("Parameter validation failed for get_datatable_rows command")
        );
        FMCPErrorHandler::LogError(ValidationError);
        return FMCPErrorHandler::CreateStructuredErrorResponse(ValidationError);
    }
    
    // Parse parameters
    FString DataTablePath;
    TArray<FString> RowNames;
    FString ParseError;
    
    if (!ParseParameters(Parameters, DataTablePath, RowNames, ParseError))
    {
        FMCPError ParseErrorObj = FMCPErrorHandler::CreateInvalidParametersError(
            FString::Printf(TEXT("Failed to parse parameters: %s"), *ParseError)
        );
        FMCPErrorHandler::LogError(ParseErrorObj);
        return FMCPErrorHandler::CreateStructuredErrorResponse(ParseErrorObj);
    }
    
    // Find the DataTable
    UDataTable* DataTable = DataTableService.FindDataTable(DataTablePath);
    if (!DataTable)
    {
        FMCPError NotFoundError = FMCPErrorHandler::CreateExecutionFailedError(
            FString::Printf(TEXT("DataTable not found: %s"), *DataTablePath)
        );
        FMCPErrorHandler::LogError(NotFoundError);
        return FMCPErrorHandler::CreateStructuredErrorResponse(NotFoundError);
    }
    
    // Get rows using the service
    TSharedPtr<FJsonObject> RowsData = DataTableService.GetDataTableRows(DataTable, RowNames);
    if (!RowsData.IsValid())
    {
        FMCPError ExecutionError = FMCPErrorHandler::CreateExecutionFailedError(
            TEXT("Failed to get DataTable rows from service")
        );
        FMCPErrorHandler::LogError(ExecutionError);
        return FMCPErrorHandler::CreateStructuredErrorResponse(ExecutionError);
    }
    
    // Log successful operation
    UE_LOG(LogTemp, Log, TEXT("MCP DataTable: Successfully retrieved %d rows from DataTable '%s'"), 
           RowsData->GetArrayField(TEXT("rows")).Num(), *DataTablePath);
    
    return CreateSuccessResponse(RowsData);
}

FString FGetDataTableRowsCommand::GetCommandName() const
{
    return TEXT("get_datatable_rows");
}

bool FGetDataTableRowsCommand::ValidateParams(const FString& Parameters) const
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }
    
    // Basic parameter validation
    FString DataTablePath;
    if (!JsonObject->TryGetStringField(TEXT("datatable_path"), DataTablePath) || DataTablePath.IsEmpty())
    {
        return false;
    }
    
    // Additional validation - row_names is optional, but if provided must be valid array
    if (JsonObject->HasField(TEXT("row_names")))
    {
        // Check if the field is null (which is valid)
        TSharedPtr<FJsonValue> RowNamesValue = JsonObject->TryGetField(TEXT("row_names"));
        if (RowNamesValue.IsValid() && RowNamesValue->Type != EJson::Null)
        {
            const TArray<TSharedPtr<FJsonValue>>* RowNamesArray;
            if (!JsonObject->TryGetArrayField(TEXT("row_names"), RowNamesArray))
            {
                return false;
            }
            
            // Validate that all row names are strings
            for (const TSharedPtr<FJsonValue>& RowNameValue : *RowNamesArray)
            {
                if (!RowNameValue.IsValid() || RowNameValue->Type != EJson::String)
                {
                    return false;
                }
            }
        }
    }
    
    return true;
}

bool FGetDataTableRowsCommand::ParseParameters(const FString& JsonString, FString& OutDataTablePath, TArray<FString>& OutRowNames, FString& OutError) const
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
    
    // Parse optional row_names parameter
    OutRowNames.Empty();
    if (JsonObject->HasField(TEXT("row_names")))
    {
        // Check if the field is null (which means get all rows)
        TSharedPtr<FJsonValue> RowNamesValue = JsonObject->TryGetField(TEXT("row_names"));
        if (RowNamesValue.IsValid() && RowNamesValue->Type != EJson::Null)
        {
            TArray<TSharedPtr<FJsonValue>> RowNamesArray = JsonObject->GetArrayField(TEXT("row_names"));
            for (const TSharedPtr<FJsonValue>& RowNameValue : RowNamesArray)
            {
                OutRowNames.Add(RowNameValue->AsString());
            }
        }
        // If row_names is null, OutRowNames remains empty, which means get all rows
    }
    
    return true;
}

FString FGetDataTableRowsCommand::CreateSuccessResponse(const TSharedPtr<FJsonObject>& RowsData) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("command"), GetCommandName());
    
    // Add the rows data from the service
    ResponseObj->SetArrayField(TEXT("rows"), RowsData->GetArrayField(TEXT("rows")));
    
    // Add metadata
    TSharedPtr<FJsonObject> Metadata = MakeShared<FJsonObject>();
    Metadata->SetStringField(TEXT("timestamp"), FDateTime::UtcNow().ToIso8601());
    Metadata->SetStringField(TEXT("operation"), TEXT("get_rows"));
    Metadata->SetNumberField(TEXT("row_count"), RowsData->GetArrayField(TEXT("rows")).Num());
    ResponseObj->SetObjectField(TEXT("metadata"), Metadata);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FGetDataTableRowsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    // This method is deprecated in favor of using FMCPErrorHandler::CreateStructuredErrorResponse
    // Keeping for backward compatibility but should not be used in new code
    FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(ErrorMessage);
    return FMCPErrorHandler::CreateStructuredErrorResponse(Error);
}

