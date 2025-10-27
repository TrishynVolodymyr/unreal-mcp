#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"
#include "Dom/JsonObject.h"

/**
 * Command for modifying properties on Blueprint components
 * This is a universal tool for setting any component properties including:
 * - WidgetClass on WidgetComponent
 * - StaticMesh on StaticMeshComponent
 * - Material on mesh components
 * - LightColor, Intensity on LightComponent
 * - Any other component-specific properties
 * 
 * Uses the new service layer for property operations
 */
class UNREALMCP_API FSetComponentPropertyCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintService - Reference to the blueprint service for operations
     */
    explicit FSetComponentPropertyCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the blueprint service */
    IBlueprintService& BlueprintService;
    
    /**
     * Parse JSON parameters for property setting
     * @param JsonString - JSON string containing parameters
     * @param OutBlueprintName - Name of the target blueprint
     * @param OutComponentName - Name of the target component
     * @param OutProperties - Properties to set
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, 
                        FString& OutComponentName, TSharedPtr<FJsonObject>& OutProperties, 
                        FString& OutError) const;
    
    /**
     * Set properties on a component using the service layer
     * @param Blueprint - Target blueprint
     * @param ComponentName - Name of the component
     * @param Properties - Properties to set
     * @param OutSuccessProperties - List of successfully set properties
     * @param OutFailedProperties - List of failed properties with error messages
     * @param OutAvailableProperties - List of all available properties on the component
     * @return true if at least one property was set successfully
     */
    bool SetComponentProperties(UBlueprint* Blueprint, const FString& ComponentName,
                               const TSharedPtr<FJsonObject>& Properties,
                               TArray<FString>& OutSuccessProperties,
                               TMap<FString, FString>& OutFailedProperties,
                               TArray<FString>& OutAvailableProperties) const;
    
    /**
     * Create success response JSON
     * @param SuccessProperties - List of successfully set properties
     * @param FailedProperties - Map of failed properties to error messages
     * @param AvailableProperties - List of all available properties on the component
     * @return JSON response string
     */
    FString CreateSuccessResponse(const TArray<FString>& SuccessProperties,
                                 const TMap<FString, FString>& FailedProperties,
                                 const TArray<FString>& AvailableProperties) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
