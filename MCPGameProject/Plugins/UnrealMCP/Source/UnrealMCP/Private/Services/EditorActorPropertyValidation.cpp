#include "Services/EditorActorPropertyValidation.h"

#include "Dom/JsonValue.h"
#include "UObject/UnrealType.h"

namespace UnrealMCP::EditorActorPropertyValidation
{
bool ValidateJsonType(
	FProperty* Property,
	const FString& PropertyName,
	const TSharedPtr<FJsonValue>& PropertyValue,
	FString& OutError)
{
	if (!PropertyValue.IsValid())
	{
		OutError = FString::Printf(TEXT("Property '%s' requires a JSON value"), *PropertyName);
		return false;
	}

	const FByteProperty* ByteProperty = CastField<FByteProperty>(Property);
	const bool bEnum = (ByteProperty && ByteProperty->GetIntPropertyEnum()) || Property->IsA<FEnumProperty>();
	if (Property->IsA<FNumericProperty>() && !bEnum && PropertyValue->Type != EJson::Number)
	{
		OutError = FString::Printf(TEXT("Property '%s' requires a JSON number"), *PropertyName);
		return false;
	}
	if (Property->IsA<FBoolProperty>() && PropertyValue->Type != EJson::Boolean)
	{
		OutError = FString::Printf(TEXT("Property '%s' requires a JSON boolean"), *PropertyName);
		return false;
	}
	if (Property->IsA<FStrProperty>() && PropertyValue->Type != EJson::String)
	{
		OutError = FString::Printf(TEXT("Property '%s' requires a JSON string"), *PropertyName);
		return false;
	}
	if (bEnum && PropertyValue->Type != EJson::String && PropertyValue->Type != EJson::Number)
	{
		OutError = FString::Printf(TEXT("Enum property '%s' requires a JSON string or number"), *PropertyName);
		return false;
	}
	if (Property->IsA<FStructProperty>())
	{
		const TArray<TSharedPtr<FJsonValue>>* Components = nullptr;
		if (!PropertyValue->TryGetArray(Components))
		{
			OutError = FString::Printf(TEXT("Struct property '%s' requires a JSON array"), *PropertyName);
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Component : *Components)
		{
			if (!Component.IsValid() || Component->Type != EJson::Number)
			{
				OutError = FString::Printf(TEXT("Struct property '%s' requires numeric components"), *PropertyName);
				return false;
			}
		}
	}
	if (Property->IsA<FObjectProperty>() && PropertyValue->Type != EJson::String)
	{
		OutError = FString::Printf(TEXT("Object/class property '%s' requires a JSON string path"), *PropertyName);
		return false;
	}
	return true;
}
}
