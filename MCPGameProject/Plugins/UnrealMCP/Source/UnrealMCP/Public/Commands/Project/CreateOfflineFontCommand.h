#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to create an offline (bitmap/SDF atlas) font from a texture and metrics JSON.
 */
class UNREALMCP_API FCreateOfflineFontCommand : public IUnrealMCPCommand
{
public:
    FCreateOfflineFontCommand(TSharedPtr<IProjectService> InProjectService);

    virtual FString GetCommandName() const override { return TEXT("create_offline_font"); }
    virtual bool ValidateParams(const FString& Parameters) const override;
    virtual FString Execute(const FString& Parameters) override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
