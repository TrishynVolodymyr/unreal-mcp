#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SetActorPropertyNotificationTestActor.generated.h"

UCLASS(NotBlueprintable, Transient)
class AUnrealMCPActorPropertyNotificationTestActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 TestValue = 0;

	int32 PostEditChangeCount = 0;
	FName LastChangedProperty = NAME_None;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
