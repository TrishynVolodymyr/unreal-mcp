#include "Commands/Project/SetIMCMappingSettingsCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "InputMappingContext.h"
#include "EnhancedActionKeyMapping.h"
#include "PlayerMappableKeySettings.h"
#include "InputAction.h"
#include "EditorAssetLibrary.h"
#include "UObject/SavePackage.h"

FSetIMCMappingSettingsCommand::FSetIMCMappingSettingsCommand(TSharedPtr<IProjectService> InProjectService)
	: ProjectService(InProjectService)
{
}

FString FSetIMCMappingSettingsCommand::GetCommandName() const
{
	return TEXT("set_imc_mapping_settings");
}

bool FSetIMCMappingSettingsCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	FString IMCPath;
	if (!JsonObject->TryGetStringField(TEXT("imc_path"), IMCPath) || IMCPath.IsEmpty())
	{
		return false;
	}

	FString ActionName;
	if (!JsonObject->TryGetStringField(TEXT("action_name"), ActionName) || ActionName.IsEmpty())
	{
		return false;
	}

	FString Key;
	if (!JsonObject->TryGetStringField(TEXT("key"), Key) || Key.IsEmpty())
	{
		return false;
	}

	FString MappingName;
	if (!JsonObject->TryGetStringField(TEXT("mapping_name"), MappingName) || MappingName.IsEmpty())
	{
		return false;
	}

	return true;
}

static FString SerializeError(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Err = FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
	FString Out;
	TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Err.ToSharedRef(), W);
	return Out;
}

FString FSetIMCMappingSettingsCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return SerializeError(TEXT("Invalid JSON parameters"));
	}

	FString IMCPath = JsonObject->GetStringField(TEXT("imc_path"));
	FString ActionName = JsonObject->GetStringField(TEXT("action_name"));
	FString Key = JsonObject->GetStringField(TEXT("key"));
	FString MappingName = JsonObject->GetStringField(TEXT("mapping_name"));

	FString DisplayName;
	JsonObject->TryGetStringField(TEXT("display_name"), DisplayName);
	if (DisplayName.IsEmpty())
	{
		DisplayName = MappingName;
	}

	FString DisplayCategory;
	JsonObject->TryGetStringField(TEXT("display_category"), DisplayCategory);

	// Load the IMC asset
	UInputMappingContext* IMC = Cast<UInputMappingContext>(UEditorAssetLibrary::LoadAsset(IMCPath));
	if (!IMC)
	{
		return SerializeError(FString::Printf(TEXT("InputMappingContext not found: %s"), *IMCPath));
	}

	// Find the FKey from the string
	FKey TargetKey(*Key);
	if (!TargetKey.IsValid())
	{
		return SerializeError(FString::Printf(TEXT("Invalid key: %s"), *Key));
	}

	// Find the matching mapping by action name + key
	const TArray<FEnhancedActionKeyMapping>& Mappings = IMC->GetMappings();
	int32 FoundIndex = INDEX_NONE;

	for (int32 i = 0; i < Mappings.Num(); ++i)
	{
		const FEnhancedActionKeyMapping& Mapping = Mappings[i];
		if (Mapping.Action && Mapping.Action->GetName() == ActionName && Mapping.Key == TargetKey)
		{
			FoundIndex = i;
			break;
		}
	}

	if (FoundIndex == INDEX_NONE)
	{
		return SerializeError(FString::Printf(
			TEXT("No mapping found for action '%s' with key '%s' in IMC '%s'"),
			*ActionName, *Key, *IMCPath));
	}

	// Get mutable reference to the mapping
	FEnhancedActionKeyMapping& Mapping = IMC->GetMapping(FoundIndex);

	// Set SettingBehavior to OverrideSettings via reflection (protected property)
	FEnumProperty* BehaviorProp = CastField<FEnumProperty>(
		FEnhancedActionKeyMapping::StaticStruct()->FindPropertyByName(TEXT("SettingBehavior")));
	if (BehaviorProp)
	{
		// EPlayerMappableKeySettingBehaviors::OverrideSettings = 1
		uint8 OverrideValue = 1;
		BehaviorProp->GetUnderlyingProperty()->SetIntPropertyValue(
			BehaviorProp->ContainerPtrToValuePtr<void>(&Mapping), static_cast<int64>(OverrideValue));
	}
	else
	{
		// Fallback: try as a byte property
		FByteProperty* ByteProp = CastField<FByteProperty>(
			FEnhancedActionKeyMapping::StaticStruct()->FindPropertyByName(TEXT("SettingBehavior")));
		if (ByteProp)
		{
			ByteProp->SetPropertyValue_InContainer(&Mapping, 1); // OverrideSettings = 1
		}
		else
		{
			return SerializeError(TEXT("Cannot find SettingBehavior property on FEnhancedActionKeyMapping"));
		}
	}

	// Access PlayerMappableKeySettings via reflection (protected property)
	FObjectProperty* SettingsProp = CastField<FObjectProperty>(
		FEnhancedActionKeyMapping::StaticStruct()->FindPropertyByName(TEXT("PlayerMappableKeySettings")));
	if (!SettingsProp)
	{
		return SerializeError(TEXT("Cannot find PlayerMappableKeySettings property on FEnhancedActionKeyMapping"));
	}

	// Create or get the UPlayerMappableKeySettings instance
	UPlayerMappableKeySettings* KeySettings = Cast<UPlayerMappableKeySettings>(
		SettingsProp->GetObjectPropertyValue(SettingsProp->ContainerPtrToValuePtr<void>(&Mapping)));

	if (!KeySettings)
	{
		KeySettings = NewObject<UPlayerMappableKeySettings>(IMC, UPlayerMappableKeySettings::StaticClass(),
			NAME_None, RF_Public | RF_Transactional);
		SettingsProp->SetObjectPropertyValue(
			SettingsProp->ContainerPtrToValuePtr<void>(&Mapping), KeySettings);
	}

	// Set the settings
	KeySettings->Name = FName(*MappingName);
	KeySettings->DisplayName = FText::FromString(DisplayName);
	if (!DisplayCategory.IsEmpty())
	{
		KeySettings->DisplayCategory = FText::FromString(DisplayCategory);
	}

	// Mark dirty and save
	IMC->MarkPackageDirty();
	UPackage* Package = IMC->GetOutermost();
	if (Package)
	{
		FString PackageFilename;
		if (FPackageName::DoesPackageExist(Package->GetName(), &PackageFilename))
		{
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			UPackage::SavePackage(Package, IMC, *PackageFilename, SaveArgs);
		}
	}

	// Build response
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("imc_path"), IMCPath);
	Response->SetStringField(TEXT("action_name"), ActionName);
	Response->SetStringField(TEXT("key"), Key);
	Response->SetStringField(TEXT("mapping_name"), MappingName);
	Response->SetStringField(TEXT("display_name"), DisplayName);
	if (!DisplayCategory.IsEmpty())
	{
		Response->SetStringField(TEXT("display_category"), DisplayCategory);
	}
	Response->SetStringField(TEXT("message"), FString::Printf(
		TEXT("Per-mapping settings set on %s[%s/%s]: name=%s, behavior=OverrideSettings"),
		*IMCPath, *ActionName, *Key, *MappingName));

	FString Out;
	TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Response.ToSharedRef(), W);
	return Out;
}
