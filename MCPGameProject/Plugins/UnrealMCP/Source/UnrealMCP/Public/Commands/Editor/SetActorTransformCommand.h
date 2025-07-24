#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IEditorService.h"

/**
 * Command for setting actor transform properties
 */
class UNREALMCP_API FSetActorTransformCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InEditorService - Reference to the editor service
     */
    explicit FSetActorTransformCommand(IEditorService& InEditorService);

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
     * @param OutLocation - Parsed location (optional)
     * @param OutRotation - Parsed rotation (optional)
     * @param OutScale - Parsed scale (optional)
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutActorName, 
                        TOptional<FVector>& OutLocation, TOptional<FRotator>& OutRotation, 
                        TOptional<FVector>& OutScale, FString& OutError) const;

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