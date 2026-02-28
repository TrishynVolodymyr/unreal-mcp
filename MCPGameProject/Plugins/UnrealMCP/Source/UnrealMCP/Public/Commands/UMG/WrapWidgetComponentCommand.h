#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"

// Forward declarations
class IUMGService;
struct FMCPError;

/**
 * Command for wrapping an existing widget component with a container widget.
 * Creates a new container (SizeBox, Border, VBox, HBox, Overlay, ScaleBox, etc.),
 * puts it where the original widget was, and moves the original widget inside.
 * 
 * This is the MCP equivalent of UE Editor's "Wrap With..." context menu action.
 */
class UNREALMCP_API FWrapWidgetComponentCommand : public IUnrealMCPCommand
{
public:
    explicit FWrapWidgetComponentCommand(TSharedPtr<IUMGService> InUMGService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IUMGService> UMGService;
    
    TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params);
    bool ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const;
    TSharedPtr<FJsonObject> CreateSuccessResponse(const FString& BlueprintName, const FString& ComponentName,
                                                   const FString& WrapperName, const FString& WrapperType) const;
    TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error) const;
};
