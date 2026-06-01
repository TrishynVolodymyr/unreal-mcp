#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Set the editor (perspective) viewport camera location and orientation.
 *
 * Lets MCP clients frame the level for screenshots without a human dragging the
 * viewport — orbit a point (location + look_at), or set an explicit rotation.
 * This is what makes automated visual verification (capture_viewport_screenshot
 * from chosen angles, incl. view-dependent rendering) possible.
 */
class UNREALMCP_API FSetViewportCameraCommand : public IUnrealMCPCommand
{
public:
    FSetViewportCameraCommand() = default;
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;
};
