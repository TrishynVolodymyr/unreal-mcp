#include "Commands/Project/CreateInputMappingContextCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "AssetToolsModule.h"
#include "Factories/Factory.h"
#include "EditorAssetLibrary.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetRegistryModule.h"

FCreateInputMappingContextCommand::FCreateInputMappingContextCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

FString FCreateInputMappingContextCommand::GetCommandName() const
{
    return TEXT("create_input_mapping_context");
}

bool FCreateInputMappingContextCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString ContextName;
    if (!JsonObject->TryGetStringField(TEXT("context_name"), ContextName) || ContextName.IsEmpty())
    {
        return false;
    }

    return true;
}

FString FCreateInputMappingContextCommand::Execute(const FString& Parameters)
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
    FString ContextName = JsonObject->GetStringField(TEXT("context_name"));
    
    FString Path = TEXT("/Game/Input");
    JsonObject->TryGetStringField(TEXT("path"), Path);

    FString Description;
    JsonObject->TryGetStringField(TEXT("description"), Description);

    // Add IMC_ prefix if not present
    if (!ContextName.StartsWith(TEXT("IMC_")))
    {
        ContextName = TEXT("IMC_") + ContextName;
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
    FString PackageName = PackagePath + ContextName;

    // Check if the asset already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Input Mapping Context already exists: %s"), *PackageName));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Create the Input Mapping Context asset
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    
    // Create the asset using the asset tools
    UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(ContextName, PackagePath.LeftChop(1), UInputMappingContext::StaticClass(), nullptr);
    UInputMappingContext* NewContext = Cast<UInputMappingContext>(CreatedAsset);
    
    if (!NewContext)
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Input Mapping Context asset"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Mark the asset as dirty and save
    NewContext->MarkPackageDirty();
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().AssetCreated(NewContext);

    // Create success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("context_name"), ContextName);
    ResponseObj->SetStringField(TEXT("asset_path"), PackageName);
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

