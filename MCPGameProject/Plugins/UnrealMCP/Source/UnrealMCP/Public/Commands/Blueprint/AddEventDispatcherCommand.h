#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command to add an Event Dispatcher (multicast delegate) to a Blueprint.
 * 
 * Parameters:
 *   blueprint_name (string, required): Name of the target Blueprint
 *   dispatcher_name (string, required): Name of the event dispatcher to create
 * 
 * Returns success with the dispatcher name, or error if it already exists or blueprint not found.
 */
class UNREALMCP_API FAddEventDispatcherCommand : public IUnrealMCPCommand
{
public:
    explicit FAddEventDispatcherCommand(IBlueprintService& InBlueprintService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBlueprintService& BlueprintService;

    FString CreateSuccessResponse(const FString& BlueprintName, const FString& DispatcherName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
