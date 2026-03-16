#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class UNREALMCP_API FAutoGenerateLodsCommand : public IUnrealMCPCommand
{
public:
	FAutoGenerateLodsCommand() = default;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
