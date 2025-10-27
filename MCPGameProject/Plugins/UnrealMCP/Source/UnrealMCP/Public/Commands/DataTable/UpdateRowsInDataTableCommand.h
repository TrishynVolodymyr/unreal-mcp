#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IDataTableService.h"

/**
 * Command for updating rows in DataTable assets
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FUpdateRowsInDataTableCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InDataTableService - Reference to the DataTable service for operations
     */
    explicit FUpdateRowsInDataTableCommand(IDataTableService& InDataTableService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the DataTable service */
    IDataTableService& DataTableService;
    
    /**
     * Parse JSON parameters into DataTable path and row parameters
     * @param JsonString - JSON string containing parameters
     * @param OutDataTablePath - Parsed DataTable path
     * @param OutRows - Parsed row parameters
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutDataTablePath, TArray<FDataTableRowParams>& OutRows, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param UpdatedRows - Names of successfully updated rows
     * @param FailedRows - Names of rows that failed to update
     * @return JSON response string
     */
    FString CreateSuccessResponse(const TArray<FString>& UpdatedRows, const TArray<FString>& FailedRows) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
