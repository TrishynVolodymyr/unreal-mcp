#include "Tests/SetActorPropertyNotificationTestActor.h"

#if WITH_EDITOR
void AUnrealMCPActorPropertyNotificationTestActor::PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	++PostEditChangeCount;
	LastChangedProperty = PropertyChangedEvent.GetPropertyName();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
