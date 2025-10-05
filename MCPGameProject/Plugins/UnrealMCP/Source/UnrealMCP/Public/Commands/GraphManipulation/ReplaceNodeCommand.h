#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/BlueprintNodeService.h"

/**
 * Command to replace a node in a Blueprint graph.
 * This performs: disconnect old node -> delete old node -> create new node -> reconnect (where possible)
 */
class FReplaceNodeCommand : public IUnrealMCPCommand
{
public:
    explicit FReplaceNodeCommand(IBlueprintNodeService& InService)
        : Service(InService)
    {
    }

    virtual ~FReplaceNodeCommand() = default;

    virtual FString GetCommandName() const override
    {
        return TEXT("replace_node");
    }

    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override { return true; }

private:
    IBlueprintNodeService& Service;
    
    struct FPinConnection
    {
        FString PinName;
        TArray<FString> ConnectedNodeIds;
        TArray<FString> ConnectedPinNames;
        bool bIsInput;
    };
    
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
