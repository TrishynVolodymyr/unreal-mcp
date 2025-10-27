#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IDataTableService.h"

/**
 * Command for getting rows from DataTable assets
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FGetDataTableRowsCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InDataTableService - Reference to the DataTable service for operations
     */
    explicit FGetDataTableRowsCommand(IDataTableService& InDataTableService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the DataTable service */
    IDataTableService& DataTableService;
    
    /**
     * Parse JSON parameters into DataTable path and optional row names
     * @param JsonString - JSON string containing parameters
     * @param OutDataTablePath - Parsed DataTable path
     * @param OutRowNames - Parsed row names (empty for all rows)
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutDataTablePath, TArray<FString>& OutRowNames, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param RowsData - JSON object containing the rows data
     * @return JSON response string
     */
    FString CreateSuccessResponse(const TSharedPtr<FJsonObject>& RowsData) const;
    
    /**
     * Create error response JSON (deprecated - use FMCPErrorHandler instead)
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
