#include "Commands/Project/ListInputMappingContextsCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"

FListInputMappingContextsCommand::FListInputMappingContextsCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

FString FListInputMappingContextsCommand::GetCommandName() const
{
    return TEXT("list_input_mapping_contexts");
}

bool FListInputMappingContextsCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return true; // No required parameters
}

FString FListInputMappingContextsCommand::Execute(const FString& Parameters)
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

    // Get all Input Mapping Context assets
    TArray<FString> Assets = UEditorAssetLibrary::ListAssets(Path, true, false);
    
    TArray<TSharedPtr<FJsonValue>> ContextArray;
    
    for (const FString& AssetPath : Assets)
    {
        // Check if this is an Input Mapping Context asset
        UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
        UInputMappingContext* MappingContext = Cast<UInputMappingContext>(Asset);
        
        if (MappingContext)
        {
            TSharedPtr<FJsonObject> ContextObj = MakeShared<FJsonObject>();
            ContextObj->SetStringField(TEXT("name"), FPaths::GetBaseFilename(AssetPath));
            ContextObj->SetStringField(TEXT("path"), AssetPath);
            
            // Get mapping count
            const TArray<FEnhancedActionKeyMapping>& Mappings = MappingContext->GetMappings();
            ContextObj->SetNumberField(TEXT("mapping_count"), Mappings.Num());
            
            // Get details about mappings
            TArray<TSharedPtr<FJsonValue>> MappingArray;
            for (const FEnhancedActionKeyMapping& Mapping : Mappings)
            {
                if (Mapping.Action)
                {
                    TSharedPtr<FJsonObject> MappingObj = MakeShared<FJsonObject>();
                    MappingObj->SetStringField(TEXT("action_name"), Mapping.Action->GetName());
                    MappingObj->SetStringField(TEXT("key"), Mapping.Key.ToString());
                    MappingArray.Add(MakeShared<FJsonValueObject>(MappingObj));
                }
            }
            ContextObj->SetArrayField(TEXT("mappings"), MappingArray);
            
            ContextArray.Add(MakeShared<FJsonValueObject>(ContextObj));
        }
    }

    // Create success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("path"), Path);
    ResponseObj->SetNumberField(TEXT("count"), ContextArray.Num());
    ResponseObj->SetArrayField(TEXT("contexts"), ContextArray);

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

