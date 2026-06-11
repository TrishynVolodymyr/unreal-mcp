#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Set the active editor or PIE camera location and orientation.
 *
 * Lets MCP clients frame the level for screenshots without a human dragging the
 * viewport — orbit a point (location + look_at), or set an explicit rotation.
 * This is what makes automated visual verification (capture_viewport_screenshot
 * from chosen angles, incl. view-dependent rendering) possible. During PIE a
 * transient camera actor becomes the local player's view target so custom pawn
 * camera implementations cannot ignore the requested transform.
 */
class UNREALMCP_API FSetViewportCameraCommand : public IUnrealMCPCommand
{
public:
    FSetViewportCameraCommand() = default;
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;
};
