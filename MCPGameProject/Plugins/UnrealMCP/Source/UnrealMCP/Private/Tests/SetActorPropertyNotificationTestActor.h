#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SetActorPropertyNotificationTestActor.generated.h"

UENUM()
enum class EUnrealMCPActorPropertyTestMode : uint8
{
	First,
	Second
};

UCLASS(NotBlueprintable, Transient)
class AUnrealMCPActorPropertyNotificationTestActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 TestValue = 0;

	UPROPERTY()
	EUnrealMCPActorPropertyTestMode TestMode = EUnrealMCPActorPropertyTestMode::First;

	UPROPERTY()
	FString TestString = TEXT("original");

	UPROPERTY()
	FVector2D TestVector = FVector2D(1.0, 2.0);

	int32 PreEditChangeCount = 0;
	int32 PostEditChangeCount = 0;
	int32 ValueObservedBeforeChange = INDEX_NONE;
	TArray<FString> NotificationOrder;
	FName LastChangedProperty = NAME_None;

#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
