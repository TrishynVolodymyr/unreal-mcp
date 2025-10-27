#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IDataTableService.h"

/**
 * Command for getting row names and field names from DataTable assets
 * Implements the IUnrealMCPCommand interface for standardized command execution
 * Complies with the new architecture using Command Pattern and Error Handling System
 */
class UNREALMCP_API FGetDataTableRowNamesCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InDataTableService - Reference to the DataTable service for operations
     */
    explicit FGetDataTableRowNamesCommand(IDataTableService& InDataTableService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the DataTable service */
    IDataTableService& DataTableService;
    
    /**
     * Parse JSON parameters into structured data
     * @param JsonString - JSON string containing parameters
     * @param OutDataTablePath - Parsed DataTable path
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutDataTablePath, FString& OutError) const;
    
    /**
     * Create success response JSON using structured approach
     * @param RowNames - Array of row names
     * @param FieldNames - Array of field names
     * @return JSON response string
     */
    FString CreateSuccessResponse(const TArray<FString>& RowNames, const TArray<FString>& FieldNames) const;
    
    /**
     * Create error response JSON using the error handling system (deprecated method for compatibility)
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
