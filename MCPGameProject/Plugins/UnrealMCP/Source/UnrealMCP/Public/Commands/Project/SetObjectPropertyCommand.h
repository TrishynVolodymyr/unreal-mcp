#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command for setting ANY property on ANY UObject asset via UE property-text
 * import (handles object refs, arrays, structs, enums — anything ExportText
 * round-trips). Generic counterpart to set_data_asset_property.
 */
class UNREALMCP_API FSetObjectPropertyCommand : public IUnrealMCPCommand
{
public:
    FSetObjectPropertyCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FSetObjectPropertyCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("set_object_property"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;
};
