#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IEditorService.h"

/**
 * Command for spawning new actors
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FSpawnActorCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InEditorService - Reference to the editor service for operations
     */
    explicit FSpawnActorCommand(IEditorService& InEditorService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the editor service */
    IEditorService& EditorService;
    
    /**
     * Parse JSON parameters into actor spawn parameters
     * @param JsonString - JSON string containing parameters
     * @param OutParams - Parsed parameters
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FActorSpawnParams& OutParams, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param Actor - Spawned actor
     * @return JSON response string
     */
    FString CreateSuccessResponse(AActor* Actor) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
