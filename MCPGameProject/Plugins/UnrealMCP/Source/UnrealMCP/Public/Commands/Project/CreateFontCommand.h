#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Unified command to create font assets in Unreal Engine.
 *
 * Supports three source types:
 * - "ttf": Import an external TTF file as a FontFace asset
 * - "sdf_texture": Create a FontFace from an SDF texture (for distance field rendering)
 * - "offline": Create a UFont from a texture atlas and metrics JSON (bitmap fonts)
 *
 * This command consolidates the previous create_font_face and create_offline_font commands.
 */
class UNREALMCP_API FCreateFontCommand : public IUnrealMCPCommand
{
public:
    FCreateFontCommand(TSharedPtr<IProjectService> InProjectService);

    virtual FString GetCommandName() const override { return TEXT("create_font"); }
    virtual bool ValidateParams(const FString& Parameters) const override;
    virtual FString Execute(const FString& Parameters) override;

private:
    TSharedPtr<IProjectService> ProjectService;

    /** Execute TTF import workflow */
    FString ExecuteTTFImport(const TSharedPtr<FJsonObject>& JsonObject);

    /** Execute SDF texture workflow (like old create_font_face) */
    FString ExecuteSDFTexture(const TSharedPtr<FJsonObject>& JsonObject);

    /** Execute offline/bitmap workflow (like old create_offline_font) */
    FString ExecuteOffline(const TSharedPtr<FJsonObject>& JsonObject);

    /** Helper to create error response JSON */
    FString CreateErrorResponse(const FString& ErrorMessage) const;

    /** Helper to create success response JSON */
    FString CreateSuccessResponse(const TSharedPtr<FJsonObject>& ResponseData) const;
};
