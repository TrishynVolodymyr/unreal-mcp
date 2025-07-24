#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IEditorService.h"

/**
 * Command for getting actor properties
 */
class UNREALMCP_API FGetActorPropertiesCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InEditorService - Reference to the editor service
     */
    explicit FGetActorPropertiesCommand(IEditorService& InEditorService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the editor service */
    IEditorService& EditorService;

    /**
     * Parse command parameters
     * @param JsonString - JSON parameters string
     * @param OutActorName - Parsed actor name
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutActorName, FString& OutError) const;

    /**
     * Create success response
     * @param Actor - Actor to get properties from
     * @return JSON success response
     */
    FString CreateSuccessResponse(AActor* Actor) const;

    /**
     * Create error response
     * @param ErrorMessage - Error message
     * @return JSON error response
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};