#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to get metadata about an existing FontFace asset.
 */
class UNREALMCP_API FGetFontFaceMetadataCommand : public IUnrealMCPCommand
{
public:
    FGetFontFaceMetadataCommand(TSharedPtr<IProjectService> InProjectService);

    virtual FString GetCommandName() const override { return TEXT("get_font_face_metadata"); }
    virtual bool ValidateParams(const FString& Parameters) const override;
    virtual FString Execute(const FString& Parameters) override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
