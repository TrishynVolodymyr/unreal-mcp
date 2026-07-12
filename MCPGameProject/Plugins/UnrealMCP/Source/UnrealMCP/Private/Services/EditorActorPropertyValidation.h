#pragma once

#include "CoreMinimal.h"

class FJsonValue;
class FProperty;

namespace UnrealMCP::EditorActorPropertyValidation
{
bool ValidateJsonType(
	FProperty* Property,
	const FString& PropertyName,
	const TSharedPtr<FJsonValue>& PropertyValue,
	FString& OutError);
}
