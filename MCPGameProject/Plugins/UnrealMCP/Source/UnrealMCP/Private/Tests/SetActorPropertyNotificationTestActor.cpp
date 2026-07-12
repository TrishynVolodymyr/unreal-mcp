#include "Tests/SetActorPropertyNotificationTestActor.h"

#if WITH_EDITOR
void AUnrealMCPActorPropertyNotificationTestActor::PreEditChange(FProperty* PropertyAboutToChange)
{
	++PreEditChangeCount;
	ValueObservedBeforeChange = TestValue;
	NotificationOrder.Add(TEXT("Pre"));
	Super::PreEditChange(PropertyAboutToChange);
}

void AUnrealMCPActorPropertyNotificationTestActor::PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	++PostEditChangeCount;
	NotificationOrder.Add(TEXT("Post"));
	LastChangedProperty = PropertyChangedEvent.GetPropertyName();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
