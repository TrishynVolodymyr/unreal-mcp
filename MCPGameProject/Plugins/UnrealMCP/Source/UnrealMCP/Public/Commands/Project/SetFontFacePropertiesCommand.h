#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to modify properties on an existing FontFace asset.
 */
class UNREALMCP_API FSetFontFacePropertiesCommand : public IUnrealMCPCommand
{
public:
    FSetFontFacePropertiesCommand(TSharedPtr<IProjectService> InProjectService);

    virtual FString GetCommandName() const override { return TEXT("set_font_face_properties"); }
    virtual bool ValidateParams(const FString& Parameters) const override;
    virtual FString Execute(const FString& Parameters) override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
