#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IMaterialService.h"

/**
 * Command for applying a material to an actor's mesh component
 */
class UNREALMCP_API FApplyMaterialToActorCommand : public IUnrealMCPCommand
{
public:
    explicit FApplyMaterialToActorCommand(IMaterialService& InMaterialService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IMaterialService& MaterialService;

    struct FApplyMaterialRequest
    {
        FString ActorName;
        FString MaterialPath;
        int32 SlotIndex = 0;
        FString ComponentName;
    };

    bool ParseParameters(const FString& JsonString, FApplyMaterialRequest& OutRequest, FString& OutError) const;
    FString CreateSuccessResponse(const FString& ActorName, const FString& MaterialPath, int32 SlotIndex) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
