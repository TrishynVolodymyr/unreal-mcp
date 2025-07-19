#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IEditorService.h"

/**
 * Command for setting light properties
 */
class UNREALMCP_API FSetLightPropertyCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InEditorService - Reference to the editor service
     */
    explicit FSetLightPropertyCommand(IEditorService& InEditorService);

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
     * @param OutPropertyName - Parsed property name
     * @param OutPropertyValue - Parsed property value
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutActorName, 
                        FString& OutPropertyName, FString& OutPropertyValue, 
                        FString& OutError) const;

    /**
     * Create success response
     * @param Actor - Actor that was modified
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