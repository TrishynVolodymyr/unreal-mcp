#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for spawning an actor with a PCG Component in the editor level.
 * Creates an AActor, attaches a UPCGComponent, assigns a PCG Graph, and configures generation trigger.
 */
class UNREALMCP_API FSpawnPCGActorCommand : public IUnrealMCPCommand
{
public:
	FSpawnPCGActorCommand() = default;

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	struct FSpawnPCGActorParams
	{
		FString GraphPath;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FString ActorLabel;
		FString GenerationTrigger = TEXT("GenerateOnLoad");
		FVector VolumeExtents = FVector(500.0, 500.0, 500.0); // Half-extents of bounds box in UU
	};

	bool ParseParameters(const FString& JsonString, FSpawnPCGActorParams& OutParams, FString& OutError) const;
	FString CreateSuccessResponse(const FString& ActorName, const FString& ActorPath, const FString& ComponentName) const;
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
