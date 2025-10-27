#include "Commands/Project/AddMappingToContextCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "EnhancedInput/Public/InputAction.h"
#include "EditorAssetLibrary.h"
#include "Engine/Engine.h"

FAddMappingToContextCommand::FAddMappingToContextCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

FString FAddMappingToContextCommand::GetCommandName() const
{
    return TEXT("add_mapping_to_context");
}

bool FAddMappingToContextCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString ContextPath, ActionPath, Key;
    if (!JsonObject->TryGetStringField(TEXT("context_path"), ContextPath) || ContextPath.IsEmpty())
    {
        return false;
    }
    if (!JsonObject->TryGetStringField(TEXT("action_path"), ActionPath) || ActionPath.IsEmpty())
    {
        return false;
    }
    if (!JsonObject->TryGetStringField(TEXT("key"), Key) || Key.IsEmpty())
    {
        return false;
    }

    return true;
}

FString FAddMappingToContextCommand::Execute(const FString& Parameters)
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
    FString ContextPath = JsonObject->GetStringField(TEXT("context_path"));
    FString ActionPath = JsonObject->GetStringField(TEXT("action_path"));
    FString Key = JsonObject->GetStringField(TEXT("key"));

    // Optional modifiers
    bool bShift = false;
    bool bCtrl = false;
    bool bAlt = false;
    bool bCmd = false;
    JsonObject->TryGetBoolField(TEXT("shift"), bShift);
    JsonObject->TryGetBoolField(TEXT("ctrl"), bCtrl);
    JsonObject->TryGetBoolField(TEXT("alt"), bAlt);
    JsonObject->TryGetBoolField(TEXT("cmd"), bCmd);

    // Load the Input Mapping Context
    if (!UEditorAssetLibrary::DoesAssetExist(ContextPath))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Input Mapping Context does not exist: %s"), *ContextPath));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    UObject* ContextAsset = UEditorAssetLibrary::LoadAsset(ContextPath);
    UInputMappingContext* Context = Cast<UInputMappingContext>(ContextAsset);
    if (!Context)
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to load Input Mapping Context"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Load the Input Action
    if (!UEditorAssetLibrary::DoesAssetExist(ActionPath))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Input Action does not exist: %s"), *ActionPath));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    UObject* ActionAsset = UEditorAssetLibrary::LoadAsset(ActionPath);
    UInputAction* Action = Cast<UInputAction>(ActionAsset);
    if (!Action)
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to load Input Action"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Create the key mapping
    FKey InputKey(*Key);
    if (!InputKey.IsValid())
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid key: %s"), *Key));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Create the enhanced input mapping
    FEnhancedActionKeyMapping& NewMapping = Context->MapKey(Action, InputKey);

    // Set modifiers if specified (note: modifier implementation would need additional work for full functionality)
    if (bShift || bCtrl || bAlt || bCmd)
    {
        UE_LOG(LogTemp, Warning, TEXT("Modifier keys requested but not fully implemented in this version"));
    }

    // Mark the context as dirty
    Context->MarkPackageDirty();

    // Create success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("context_path"), ContextPath);
    ResponseObj->SetStringField(TEXT("action_path"), ActionPath);
    ResponseObj->SetStringField(TEXT("key"), Key);
    ResponseObj->SetBoolField(TEXT("shift"), bShift);
    ResponseObj->SetBoolField(TEXT("ctrl"), bCtrl);
    ResponseObj->SetBoolField(TEXT("alt"), bAlt);
    ResponseObj->SetBoolField(TEXT("cmd"), bCmd);

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

