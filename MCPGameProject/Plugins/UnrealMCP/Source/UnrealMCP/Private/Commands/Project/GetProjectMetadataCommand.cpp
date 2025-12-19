#include "Commands/Project/GetProjectMetadataCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EnhancedInput/Public/InputAction.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"

FGetProjectMetadataCommand::FGetProjectMetadataCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

FString FGetProjectMetadataCommand::GetCommandName() const
{
    return TEXT("get_project_metadata");
}

bool FGetProjectMetadataCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return true; // No strictly required parameters
}

FString FGetProjectMetadataCommand::Execute(const FString& Parameters)
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

    FString FolderPath;
    JsonObject->TryGetStringField(TEXT("folder_path"), FolderPath);

    FString StructName;
    JsonObject->TryGetStringField(TEXT("struct_name"), StructName);

    // Get fields array
    const TArray<TSharedPtr<FJsonValue>>* FieldsArray = nullptr;
    JsonObject->TryGetArrayField(TEXT("fields"), FieldsArray);

    // Determine which fields to include
    bool bIncludeAll = !FieldsArray || FieldsArray->Num() == 0;
    if (!bIncludeAll && FieldsArray)
    {
        for (const auto& Field : *FieldsArray)
        {
            if (Field->AsString() == TEXT("*"))
            {
                bIncludeAll = true;
                break;
            }
        }
    }

    // Build response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);

    // Add requested fields
    if (bIncludeAll || IsFieldRequested(FieldsArray, TEXT("input_actions")))
    {
        TSharedPtr<FJsonObject> InputActionsInfo = BuildInputActionsInfo(Path);
        ResponseObj->SetObjectField(TEXT("input_actions"), InputActionsInfo);
    }

    if (bIncludeAll || IsFieldRequested(FieldsArray, TEXT("input_contexts")))
    {
        TSharedPtr<FJsonObject> InputContextsInfo = BuildInputContextsInfo(Path);
        ResponseObj->SetObjectField(TEXT("input_contexts"), InputContextsInfo);
    }

    if (IsFieldRequested(FieldsArray, TEXT("structs")) && !StructName.IsEmpty())
    {
        TSharedPtr<FJsonObject> StructInfo = BuildStructInfo(StructName, Path);
        ResponseObj->SetObjectField(TEXT("structs"), StructInfo);
    }
    else if (bIncludeAll && !StructName.IsEmpty())
    {
        // Only include struct info if struct_name is provided
        TSharedPtr<FJsonObject> StructInfo = BuildStructInfo(StructName, Path);
        ResponseObj->SetObjectField(TEXT("structs"), StructInfo);
    }

    if (IsFieldRequested(FieldsArray, TEXT("folder_contents")) && !FolderPath.IsEmpty())
    {
        TSharedPtr<FJsonObject> FolderInfo = BuildFolderContentsInfo(FolderPath);
        ResponseObj->SetObjectField(TEXT("folder_contents"), FolderInfo);
    }
    else if (bIncludeAll && !FolderPath.IsEmpty())
    {
        // Only include folder contents if folder_path is provided
        TSharedPtr<FJsonObject> FolderInfo = BuildFolderContentsInfo(FolderPath);
        ResponseObj->SetObjectField(TEXT("folder_contents"), FolderInfo);
    }

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

bool FGetProjectMetadataCommand::IsFieldRequested(const TArray<TSharedPtr<FJsonValue>>* FieldsArray, const FString& FieldName) const
{
    if (!FieldsArray)
    {
        return false;
    }

    for (const auto& Field : *FieldsArray)
    {
        if (Field->AsString() == FieldName || Field->AsString() == TEXT("*"))
        {
            return true;
        }
    }
    return false;
}

TSharedPtr<FJsonObject> FGetProjectMetadataCommand::BuildInputActionsInfo(const FString& Path) const
{
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

    // Get all Input Action assets
    TArray<FString> Assets = UEditorAssetLibrary::ListAssets(Path, true, false);

    TArray<TSharedPtr<FJsonValue>> ActionArray;

    for (const FString& AssetPath : Assets)
    {
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

    Result->SetStringField(TEXT("path"), Path);
    Result->SetNumberField(TEXT("count"), ActionArray.Num());
    Result->SetArrayField(TEXT("actions"), ActionArray);

    return Result;
}

TSharedPtr<FJsonObject> FGetProjectMetadataCommand::BuildInputContextsInfo(const FString& Path) const
{
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

    // Get all Input Mapping Context assets
    TArray<FString> Assets = UEditorAssetLibrary::ListAssets(Path, true, false);

    TArray<TSharedPtr<FJsonValue>> ContextArray;

    for (const FString& AssetPath : Assets)
    {
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

    Result->SetStringField(TEXT("path"), Path);
    Result->SetNumberField(TEXT("count"), ContextArray.Num());
    Result->SetArrayField(TEXT("contexts"), ContextArray);

    return Result;
}

TSharedPtr<FJsonObject> FGetProjectMetadataCommand::BuildStructInfo(const FString& StructName, const FString& Path) const
{
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

    bool bSuccess = false;
    FString Error;
    TArray<TSharedPtr<FJsonObject>> Variables = ProjectService->ShowStructVariables(StructName, Path, bSuccess, Error);

    Result->SetStringField(TEXT("struct_name"), StructName);
    Result->SetStringField(TEXT("path"), Path);

    if (bSuccess)
    {
        TArray<TSharedPtr<FJsonValue>> VariableArray;
        for (const TSharedPtr<FJsonObject>& Variable : Variables)
        {
            VariableArray.Add(MakeShared<FJsonValueObject>(Variable));
        }
        Result->SetArrayField(TEXT("variables"), VariableArray);
        Result->SetNumberField(TEXT("count"), Variables.Num());
    }
    else
    {
        Result->SetStringField(TEXT("error"), Error);
    }

    return Result;
}

TSharedPtr<FJsonObject> FGetProjectMetadataCommand::BuildFolderContentsInfo(const FString& FolderPath) const
{
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

    bool bSuccess = false;
    FString Error;
    TArray<FString> Contents = ProjectService->ListFolderContents(FolderPath, bSuccess, Error);

    Result->SetStringField(TEXT("folder_path"), FolderPath);

    if (bSuccess)
    {
        TArray<TSharedPtr<FJsonValue>> ContentsArray;
        for (const FString& Content : Contents)
        {
            ContentsArray.Add(MakeShared<FJsonValueString>(Content));
        }
        Result->SetArrayField(TEXT("contents"), ContentsArray);
        Result->SetNumberField(TEXT("count"), Contents.Num());
    }
    else
    {
        Result->SetStringField(TEXT("error"), Error);
    }

    return Result;
}
