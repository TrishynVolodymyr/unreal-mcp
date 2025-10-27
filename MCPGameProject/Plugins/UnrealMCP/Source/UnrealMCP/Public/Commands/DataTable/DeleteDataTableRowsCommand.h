#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"

// Forward declarations
class IDataTableService;
struct FMCPError;

/**
 * Command for deleting rows from DataTable assets
 * Implements the new command pattern architecture with service layer delegation
 * while maintaining compatibility with the existing string-based interface
 */
class UNREALMCP_API FDeleteDataTableRowsCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InDataTableService - Shared pointer to the DataTable service for operations
     */
    explicit FDeleteDataTableRowsCommand(TSharedPtr<IDataTableService> InDataTableService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Shared pointer to the DataTable service */
    TSharedPtr<IDataTableService> DataTableService;
    
    /**
     * Internal execution with JSON objects (new architecture)
     * @param Params - JSON parameters
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params);
    
    /**
     * Internal validation with JSON objects (new architecture)
     * @param Params - JSON parameters
     * @param OutError - Error message if validation fails
     * @return true if validation passes
     */
    bool ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const;
    
    /**
     * Create success response JSON object
     * @param DeletedRows - Names of successfully deleted rows
     * @param FailedRows - Names of rows that failed to delete
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateSuccessResponse(const TArray<FString>& DeletedRows, const TArray<FString>& FailedRows) const;
    
    /**
     * Create error response JSON object from MCP error
     * @param Error - MCP error to convert to response
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error) const;
};
