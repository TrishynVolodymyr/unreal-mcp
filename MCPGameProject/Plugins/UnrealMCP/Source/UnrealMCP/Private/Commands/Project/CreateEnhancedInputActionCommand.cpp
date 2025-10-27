#include "Commands/Project/CreateEnhancedInputActionCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EnhancedInput/Public/InputAction.h"
#include "AssetToolsModule.h"
#include "Factories/Factory.h"
#include "EditorAssetLibrary.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetRegistryModule.h"

FCreateEnhancedInputActionCommand::FCreateEnhancedInputActionCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

FString FCreateEnhancedInputActionCommand::GetCommandName() const
{
    return TEXT("create_enhanced_input_action");
}

bool FCreateEnhancedInputActionCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString ActionName;
    if (!JsonObject->TryGetStringField(TEXT("action_name"), ActionName) || ActionName.IsEmpty())
    {
        return false;
    }

    return true;
}

FString FCreateEnhancedInputActionCommand::Execute(const FString& Parameters)
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid JSON parameters"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Validate parameters
    if (!ValidateParams(Parameters))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString ActionName = JsonObject->GetStringField(TEXT("action_name"));
    
    FString Path = TEXT("/Game/Input/Actions");
    JsonObject->TryGetStringField(TEXT("path"), Path);

    FString Description;
    JsonObject->TryGetStringField(TEXT("description"), Description);

    FString ValueType = TEXT("Digital");
    JsonObject->TryGetStringField(TEXT("value_type"), ValueType);

    // Add IA_ prefix if not present
    if (!ActionName.StartsWith(TEXT("IA_")))
    {
        ActionName = TEXT("IA_") + ActionName;
    }

    // Make sure the path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(Path))
    {
        if (!UEditorAssetLibrary::MakeDirectory(Path))
        {
            TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create directory: %s"), *Path));
            FString OutputString;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
            FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
            return OutputString;
        }
    }

    // Create the asset path
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + ActionName;

    // Check if the asset already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Enhanced Input Action already exists: %s"), *PackageName));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Create the Enhanced Input Action asset
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    
    // Create the asset using the asset tools
    UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(ActionName, PackagePath.LeftChop(1), UInputAction::StaticClass(), nullptr);
    UInputAction* NewAction = Cast<UInputAction>(CreatedAsset);
    
    if (!NewAction)
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Enhanced Input Action asset"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Set the value type
    if (ValueType.Equals(TEXT("Digital"), ESearchCase::IgnoreCase))
    {
        NewAction->ValueType = EInputActionValueType::Boolean;
    }
    else if (ValueType.Equals(TEXT("Analog"), ESearchCase::IgnoreCase))
    {
        NewAction->ValueType = EInputActionValueType::Axis1D;
    }
    else if (ValueType.Equals(TEXT("Axis2D"), ESearchCase::IgnoreCase))
    {
        NewAction->ValueType = EInputActionValueType::Axis2D;
    }
    else if (ValueType.Equals(TEXT("Axis3D"), ESearchCase::IgnoreCase))
    {
        NewAction->ValueType = EInputActionValueType::Axis3D;
    }
    else
    {
        // Default to Digital/Boolean
        NewAction->ValueType = EInputActionValueType::Boolean;
    }

    // Mark the asset as dirty and save
    NewAction->MarkPackageDirty();
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().AssetCreated(NewAction);

    // Create success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("action_name"), ActionName);
    ResponseObj->SetStringField(TEXT("asset_path"), PackageName);
    ResponseObj->SetStringField(TEXT("value_type"), ValueType);
    if (!Description.IsEmpty())
    {
        ResponseObj->SetStringField(TEXT("description"), Description);
    }

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

