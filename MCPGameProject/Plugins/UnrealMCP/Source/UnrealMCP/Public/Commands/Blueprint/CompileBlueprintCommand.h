#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command for compiling Blueprint assets
 * Provides enhanced error reporting for compilation issues
 */
class UNREALMCP_API FCompileBlueprintCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintService - Reference to the blueprint service for operations
     */
    explicit FCompileBlueprintCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the blueprint service */
    IBlueprintService& BlueprintService;
    
    /**
     * Parse JSON parameters for blueprint compilation
     * @param JsonString - JSON string containing parameters
     * @param OutBlueprintName - Name of the blueprint to compile
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutError) const;
    
    /**
     * Create success response JSON with compilation details
     * @param BlueprintName - Name of the compiled blueprint
     * @param CompilationTime - Time taken for compilation
     * @param Status - Compilation status message
     * @param Warnings - Array of warning messages (optional)
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& BlueprintName, float CompilationTime, const FString& Status = TEXT("compiled successfully"), const TArray<FString>& Warnings = TArray<FString>()) const;
    
    /**
     * Create error response JSON with detailed compilation errors
     * @param ErrorMessage - Primary error message
     * @param CompilationErrors - Detailed compilation error messages
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage, const TArray<FString>& CompilationErrors = TArray<FString>()) const;
    
    /**
     * Extract detailed compilation errors from blueprint
     * @param Blueprint - Blueprint that failed to compile
     * @return Array of detailed error messages
     */
    TArray<FString> ExtractCompilationErrors(UBlueprint* Blueprint) const;
};
