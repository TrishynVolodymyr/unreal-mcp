#include "Commands/Project/SetInputActionMappableSettingsCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EnhancedInput/Public/InputAction.h"
#include "EnhancedInput/Public/PlayerMappableKeySettings.h"
#include "EditorAssetLibrary.h"
#include "UObject/SavePackage.h"

FSetInputActionMappableSettingsCommand::FSetInputActionMappableSettingsCommand(TSharedPtr<IProjectService> InProjectService)
	: ProjectService(InProjectService)
{
}

FString FSetInputActionMappableSettingsCommand::GetCommandName() const
{
	return TEXT("set_input_action_mappable_settings");
}

bool FSetInputActionMappableSettingsCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	FString ActionPath;
	if (!JsonObject->TryGetStringField(TEXT("action_path"), ActionPath) || ActionPath.IsEmpty())
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

FString FSetInputActionMappableSettingsCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		TSharedPtr<FJsonObject> Err = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid JSON parameters"));
		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Err.ToSharedRef(), W);
		return Out;
	}

	FString ActionPath = JsonObject->GetStringField(TEXT("action_path"));
	FString MappingName = JsonObject->GetStringField(TEXT("mapping_name"));

	FString DisplayName;
	JsonObject->TryGetStringField(TEXT("display_name"), DisplayName);
	if (DisplayName.IsEmpty())
	{
		DisplayName = MappingName;
	}

	FString DisplayCategory;
	JsonObject->TryGetStringField(TEXT("display_category"), DisplayCategory);

	// Load the InputAction asset
	UInputAction* InputAction = Cast<UInputAction>(UEditorAssetLibrary::LoadAsset(ActionPath));
	if (!InputAction)
	{
		TSharedPtr<FJsonObject> Err = FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("InputAction not found: %s"), *ActionPath));
		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Err.ToSharedRef(), W);
		return Out;
	}

	// Access PlayerMappableKeySettings via reflection (it's protected)
	FObjectProperty* SettingsProp = CastField<FObjectProperty>(
		UInputAction::StaticClass()->FindPropertyByName(TEXT("PlayerMappableKeySettings")));
	if (!SettingsProp)
	{
		TSharedPtr<FJsonObject> Err = FUnrealMCPCommonUtils::CreateErrorResponse(
			TEXT("Cannot find PlayerMappableKeySettings property on UInputAction"));
		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Err.ToSharedRef(), W);
		return Out;
	}

	UPlayerMappableKeySettings* KeySettings = Cast<UPlayerMappableKeySettings>(
		SettingsProp->GetObjectPropertyValue(SettingsProp->ContainerPtrToValuePtr<void>(InputAction)));

	if (!KeySettings)
	{
		KeySettings = NewObject<UPlayerMappableKeySettings>(InputAction, UPlayerMappableKeySettings::StaticClass(),
			NAME_None, RF_Public | RF_Transactional);
		SettingsProp->SetObjectPropertyValue(
			SettingsProp->ContainerPtrToValuePtr<void>(InputAction), KeySettings);
	}

	// Set the settings
	KeySettings->Name = FName(*MappingName);
	KeySettings->DisplayName = FText::FromString(DisplayName);
	if (!DisplayCategory.IsEmpty())
	{
		KeySettings->DisplayCategory = FText::FromString(DisplayCategory);
	}

	// Mark dirty and save
	InputAction->MarkPackageDirty();
	UPackage* Package = InputAction->GetOutermost();
	if (Package)
	{
		FString PackageFilename;
		if (FPackageName::DoesPackageExist(Package->GetName(), &PackageFilename))
		{
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			UPackage::SavePackage(Package, InputAction, *PackageFilename, SaveArgs);
		}
	}

	// Build response
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("action_path"), ActionPath);
	Response->SetStringField(TEXT("mapping_name"), MappingName);
	Response->SetStringField(TEXT("display_name"), DisplayName);
	if (!DisplayCategory.IsEmpty())
	{
		Response->SetStringField(TEXT("display_category"), DisplayCategory);
	}
	Response->SetStringField(TEXT("message"), FString::Printf(
		TEXT("PlayerMappableKeySettings set on %s: name=%s, display=%s"),
		*ActionPath, *MappingName, *DisplayName));

	FString Out;
	TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Response.ToSharedRef(), W);
	return Out;
}
