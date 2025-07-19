#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IEditorService.h"

/**
 * Command for getting all actors in the current level
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FGetActorsInLevelCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InEditorService - Reference to the editor service for operations
     */
    explicit FGetActorsInLevelCommand(IEditorService& InEditorService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the editor service */
    IEditorService& EditorService;
    
    /**
     * Create success response JSON
     * @param Actors - Array of actors
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
