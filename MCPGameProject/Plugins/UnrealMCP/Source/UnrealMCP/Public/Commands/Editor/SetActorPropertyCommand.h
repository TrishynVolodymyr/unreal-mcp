#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IEditorService.h"

/**
 * Command for setting actor properties
 */
class UNREALMCP_API FSetActorPropertyCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InEditorService - Reference to the editor service
     */
    explicit FSetActorPropertyCommand(IEditorService& InEditorService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the editor service */
    IEditorService& EditorService;

    /**
     * Parse command parameters for single property
     * @param JsonString - JSON parameters string
     * @param OutActorName - Parsed actor name
     * @param OutPropertyName - Parsed property name
     * @param OutPropertyValue - Parsed property value
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutActorName,
                        FString& OutPropertyName, TSharedPtr<FJsonValue>& OutPropertyValue,
                        FString& OutError) const;

    /**
     * Parse command parameters for batch properties
     * @param JsonString - JSON parameters string
     * @param OutActorName - Parsed actor name
     * @param OutProperties - Array of property name/value pairs
     * @param OutError - Error message if parsing fails
     * @return true if batch format detected and parsed
     */
    bool ParseBatchParameters(const FString& JsonString, FString& OutActorName,
                             TArray<TPair<FString, TSharedPtr<FJsonValue>>>& OutProperties,
                             FString& OutError) const;

    /**
     * Create success response for single property
     * @param Actor - Actor that was modified
     * @return JSON success response
     */
    FString CreateSuccessResponse(AActor* Actor) const;

    /**
     * Create success response for batch properties
     * @param Actor - Actor that was modified
     * @param SuccessCount - Number of properties set successfully
     * @param FailedProperties - List of properties that failed
     * @return JSON success response
     */
    FString CreateBatchSuccessResponse(AActor* Actor, int32 SuccessCount,
                                       const TArray<FString>& FailedProperties) const;

    /**
     * Create error response
     * @param ErrorMessage - Error message
     * @return JSON error response
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};