#include "Commands/Project/CreateAssetCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FCreateAssetCommand::FCreateAssetCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FCreateAssetCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }
    FString Name;
    if (!JsonObject->TryGetStringField(TEXT("name"), Name) || Name.IsEmpty())
    {
        return false;
    }
    FString AssetClass;
    if (!JsonObject->TryGetStringField(TEXT("asset_class"), AssetClass) || AssetClass.IsEmpty())
    {
        return false;
    }
    return true;
}

FString FCreateAssetCommand::Execute(const FString& Parameters)
{
    auto MakeError = [](const FString& Msg) -> FString
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(Msg);
        FString Out;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return Out;
    };

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return MakeError(TEXT("Invalid JSON parameters"));
    }
    if (!ValidateParams(Parameters))
    {
        return MakeError(TEXT("Parameter validation failed. Required: name (string), asset_class (string). Optional: folder_path (string)"));
    }

    const FString Name = JsonObject->GetStringField(TEXT("name"));
    const FString AssetClass = JsonObject->GetStringField(TEXT("asset_class"));
    FString FolderPath;
    if (!JsonObject->TryGetStringField(TEXT("folder_path"), FolderPath) || FolderPath.IsEmpty())
    {
        FolderPath = TEXT("/Game");
    }

    FString AssetPath, Error;
    if (!ProjectService->CreateAsset(Name, AssetClass, FolderPath, AssetPath, Error))
    {
        return MakeError(Error);
    }

    TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
    ResponseData->SetBoolField(TEXT("success"), true);
    ResponseData->SetStringField(TEXT("name"), Name);
    ResponseData->SetStringField(TEXT("asset_class"), AssetClass);
    ResponseData->SetStringField(TEXT("folder_path"), FolderPath);
    ResponseData->SetStringField(TEXT("asset_path"), AssetPath);
    ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created asset at %s"), *AssetPath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}
