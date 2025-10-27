#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Json.h"

// Forward declarations
class IUMGService;

/**
 * Command for creating parent and child widget components in UMG Widget Blueprints
 * Implements the new command pattern architecture with service layer delegation
 * Follows the standardized architecture pattern for consistent command handling
 */
class UNREALMCP_API FCreateParentChildWidgetCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InUMGService - Reference to the UMG service for operations
     */
    explicit FCreateParentChildWidgetCommand(IUMGService& InUMGService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the UMG service */
    IUMGService& UMGService;
    
    /**
     * Parse JSON parameters into component creation parameters
     * @param JsonString - JSON string containing parameters
     * @param OutBlueprintName - Parsed blueprint name
     * @param OutParentComponentName - Parsed parent component name
     * @param OutChildComponentName - Parsed child component name
     * @param OutParentComponentType - Parsed parent component type
     * @param OutChildComponentType - Parsed child component type
     * @param OutParentPosition - Parsed parent position
     * @param OutParentSize - Parsed parent size
     * @param OutChildAttributes - Parsed child attributes
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutParentComponentName,
                        FString& OutChildComponentName, FString& OutParentComponentType, FString& OutChildComponentType,
                        FVector2D& OutParentPosition, FVector2D& OutParentSize, TSharedPtr<FJsonObject>& OutChildAttributes,
                        FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param BlueprintName - Name of the blueprint
     * @param ParentComponentName - Name of the parent component
     * @param ChildComponentName - Name of the child component
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& BlueprintName, const FString& ParentComponentName, const FString& ChildComponentName) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};