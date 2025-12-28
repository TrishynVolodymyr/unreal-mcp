#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to create a FontFace asset for use with TextBlock widgets.
 * Supports SDF fonts with configurable distance field settings.
 */
class UNREALMCP_API FCreateFontFaceCommand : public IUnrealMCPCommand
{
public:
    FCreateFontFaceCommand(TSharedPtr<IProjectService> InProjectService);

    virtual FString GetCommandName() const override { return TEXT("create_font_face"); }
    virtual bool ValidateParams(const FString& Parameters) const override;
    virtual FString Execute(const FString& Parameters) override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
