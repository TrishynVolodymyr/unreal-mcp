#include "Commands/Project/RemoveMappingFromContextCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "EnhancedInput/Public/InputAction.h"
#include "EditorAssetLibrary.h"
#include "Engine/Engine.h"

FRemoveMappingFromContextCommand::FRemoveMappingFromContextCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

FString FRemoveMappingFromContextCommand::GetCommandName() const
{
    return TEXT("remove_mapping_from_context");
}

bool FRemoveMappingFromContextCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString ContextPath, ActionPath;
    if (!JsonObject->TryGetStringField(TEXT("context_path"), ContextPath) || ContextPath.IsEmpty())
    {
        return false;
    }
    if (!JsonObject->TryGetStringField(TEXT("action_path"), ActionPath) || ActionPath.IsEmpty())
    {
        return false;
    }

    // Either key OR remove_all_keys is required.
    bool bRemoveAll = false;
    JsonObject->TryGetBoolField(TEXT("remove_all_keys"), bRemoveAll);
    FString Key;
    const bool bHasKey = JsonObject->TryGetStringField(TEXT("key"), Key) && !Key.IsEmpty();
    if (!bRemoveAll && !bHasKey)
    {
        return false;
    }
    return true;
}

FString FRemoveMappingFromContextCommand::Execute(const FString& Parameters)
{
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

    if (!ValidateParams(Parameters))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed (need context_path + action_path + (key OR remove_all_keys=true))"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    const FString ContextPath = JsonObject->GetStringField(TEXT("context_path"));
    const FString ActionPath = JsonObject->GetStringField(TEXT("action_path"));
    FString Key;
    JsonObject->TryGetStringField(TEXT("key"), Key);
    bool bRemoveAll = false;
    JsonObject->TryGetBoolField(TEXT("remove_all_keys"), bRemoveAll);

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

    // Count matching mappings BEFORE so caller knows how many were removed.
    int32 Removed = 0;
    if (bRemoveAll)
    {
        for (const FEnhancedActionKeyMapping& M : Context->GetMappings())
        {
            if (M.Action == Action) ++Removed;
        }
        Context->UnmapAllKeysFromAction(Action);
    }
    else
    {
        const FKey InputKey(*Key);
        if (!InputKey.IsValid())
        {
            TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid key: %s"), *Key));
            FString OutputString;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
            FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
            return OutputString;
        }
        for (const FEnhancedActionKeyMapping& M : Context->GetMappings())
        {
            if (M.Action == Action && M.Key == InputKey) ++Removed;
        }
        Context->UnmapKey(Action, InputKey);
    }

    Context->MarkPackageDirty();

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("context_path"), ContextPath);
    ResponseObj->SetStringField(TEXT("action_path"), ActionPath);
    ResponseObj->SetNumberField(TEXT("removed_count"), Removed);
    if (!bRemoveAll)
    {
        ResponseObj->SetStringField(TEXT("key"), Key);
    }
    ResponseObj->SetBoolField(TEXT("remove_all_keys"), bRemoveAll);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
