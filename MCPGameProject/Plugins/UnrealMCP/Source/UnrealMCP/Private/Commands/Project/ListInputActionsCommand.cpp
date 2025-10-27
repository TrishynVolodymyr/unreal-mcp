#include "Commands/Project/ListInputActionsCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EnhancedInput/Public/InputAction.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"

FListInputActionsCommand::FListInputActionsCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

FString FListInputActionsCommand::GetCommandName() const
{
    return TEXT("list_input_actions");
}

bool FListInputActionsCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return true; // No required parameters
}

FString FListInputActionsCommand::Execute(const FString& Parameters)
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

    // Extract parameters
    FString Path = TEXT("/Game");
    JsonObject->TryGetStringField(TEXT("path"), Path);

    // Get all Input Action assets
    TArray<FString> Assets = UEditorAssetLibrary::ListAssets(Path, true, false);
    
    TArray<TSharedPtr<FJsonValue>> ActionArray;
    
    for (const FString& AssetPath : Assets)
    {
        // Check if this is an Input Action asset
        UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
        UInputAction* InputAction = Cast<UInputAction>(Asset);
        
        if (InputAction)
        {
            TSharedPtr<FJsonObject> ActionObj = MakeShared<FJsonObject>();
            ActionObj->SetStringField(TEXT("name"), FPaths::GetBaseFilename(AssetPath));
            ActionObj->SetStringField(TEXT("path"), AssetPath);
            
            // Get value type
            FString ValueTypeStr;
            switch (InputAction->ValueType)
            {
                case EInputActionValueType::Boolean:
                    ValueTypeStr = TEXT("Digital");
                    break;
                case EInputActionValueType::Axis1D:
                    ValueTypeStr = TEXT("Analog");
                    break;
                case EInputActionValueType::Axis2D:
                    ValueTypeStr = TEXT("Axis2D");
                    break;
                case EInputActionValueType::Axis3D:
                    ValueTypeStr = TEXT("Axis3D");
                    break;
                default:
                    ValueTypeStr = TEXT("Unknown");
                    break;
            }
            ActionObj->SetStringField(TEXT("value_type"), ValueTypeStr);
            
            ActionArray.Add(MakeShared<FJsonValueObject>(ActionObj));
        }
    }

    // Create success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("path"), Path);
    ResponseObj->SetNumberField(TEXT("count"), ActionArray.Num());
    ResponseObj->SetArrayField(TEXT("actions"), ActionArray);

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

