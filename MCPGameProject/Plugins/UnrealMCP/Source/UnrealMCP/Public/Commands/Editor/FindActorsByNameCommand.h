#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IEditorService.h"

/**
 * Command for finding actors by name pattern
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FFindActorsByNameCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InEditorService - Reference to the editor service for operations
     */
    explicit FFindActorsByNameCommand(IEditorService& InEditorService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the editor service */
    IEditorService& EditorService;
    
    /**
     * Parse JSON parameters
     * @param JsonString - JSON string containing parameters
     * @param OutPattern - Parsed pattern
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutPattern, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param Actors - Array of matching actors
     * @return JSON response string
     */
    FString CreateSuccessResponse(const TArray<AActor*>& Actors) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
