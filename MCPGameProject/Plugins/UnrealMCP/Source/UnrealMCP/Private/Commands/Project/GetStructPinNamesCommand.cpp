#include "Commands/Project/GetStructPinNamesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"
#include "StructUtils/UserDefinedStruct.h"
#include "AssetRegistry/AssetRegistryModule.h"

FGetStructPinNamesCommand::FGetStructPinNamesCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

FString FGetStructPinNamesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString StructName;
    if (!JsonObject->TryGetStringField(TEXT("struct_name"), StructName))
    {
        return CreateErrorResponse(TEXT("Missing 'struct_name' parameter"));
    }

    UScriptStruct* FoundStruct = FindStruct(StructName);
    if (!FoundStruct)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Struct '%s' not found"), *StructName));
    }

    // Get the struct path for the response
    FString StructPath = FoundStruct->GetPathName();

    // Build field information array
    TArray<TSharedPtr<FJsonValue>> FieldsArray;

    for (TFieldIterator<FProperty> PropIt(FoundStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;

        TSharedPtr<FJsonObject> FieldObj = MakeShared<FJsonObject>();

        // Get the GUID-based name (internal name used for pin connections)
        FString GuidName = Property->GetName();
        FieldObj->SetStringField(TEXT("pin_name"), GuidName);

        // Get the friendly/display name
        FString FriendlyName = Property->GetDisplayNameText().ToString();
        if (FriendlyName.IsEmpty())
        {
            FriendlyName = ExtractFriendlyName(GuidName);
        }
        FieldObj->SetStringField(TEXT("display_name"), FriendlyName);

        // Get the property type
        FString TypeName;
        if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            TypeName = StructProp->Struct ? StructProp->Struct->GetName() : TEXT("Struct");
        }
        else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
        {
            FProperty* InnerProp = ArrayProp->Inner;
            if (const FStructProperty* InnerStructProp = CastField<FStructProperty>(InnerProp))
            {
                TypeName = FString::Printf(TEXT("%s[]"), InnerStructProp->Struct ? *InnerStructProp->Struct->GetName() : TEXT("Struct"));
            }
            else
            {
                TypeName = FString::Printf(TEXT("%s[]"), *InnerProp->GetCPPType());
            }
        }
        else if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
        {
            TypeName = ObjProp->PropertyClass ? ObjProp->PropertyClass->GetName() : TEXT("Object");
        }
        else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
        {
            TypeName = EnumProp->GetEnum() ? EnumProp->GetEnum()->GetName() : TEXT("Enum");
        }
        else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
        {
            if (ByteProp->Enum)
            {
                TypeName = ByteProp->Enum->GetName();
            }
            else
            {
                TypeName = TEXT("Byte");
            }
        }
        else
        {
            TypeName = Property->GetCPPType();
        }
        FieldObj->SetStringField(TEXT("type"), TypeName);

        // Check if this is a GUID-based name
        FieldObj->SetBoolField(TEXT("is_guid_name"), IsGuidField(GuidName));

        FieldsArray.Add(MakeShared<FJsonValueObject>(FieldObj));
    }

    return CreateSuccessResponse(StructName, StructPath, FieldsArray);
}

FString FGetStructPinNamesCommand::GetCommandName() const
{
    return TEXT("get_struct_pin_names");
}

bool FGetStructPinNamesCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString StructName;
    return JsonObject->TryGetStringField(TEXT("struct_name"), StructName);
}

UScriptStruct* FGetStructPinNamesCommand::FindStruct(const FString& StructName) const
{
    // If it looks like a full path, try loading directly
    if (StructName.StartsWith(TEXT("/Game/")) || StructName.StartsWith(TEXT("/Script/")))
    {
        UScriptStruct* FoundStruct = LoadObject<UScriptStruct>(nullptr, *StructName);
        if (FoundStruct)
        {
            return FoundStruct;
        }

        // Also try as UUserDefinedStruct
        UUserDefinedStruct* UserStruct = LoadObject<UUserDefinedStruct>(nullptr, *StructName);
        if (UserStruct)
        {
            return UserStruct;
        }
    }

    // Common paths to search for user-defined structs
    TArray<FString> SearchPaths = {
        FString::Printf(TEXT("/Game/%s"), *StructName),
        FString::Printf(TEXT("/Game/Blueprints/%s"), *StructName),
        FString::Printf(TEXT("/Game/Data/%s"), *StructName),
        FString::Printf(TEXT("/Game/Structs/%s"), *StructName),
        FString::Printf(TEXT("/Game/Inventory/Data/%s"), *StructName),
        FString::Printf(TEXT("/Script/Engine.%s"), *StructName),
        FString::Printf(TEXT("/Script/CoreUObject.%s"), *StructName)
    };

    for (const FString& Path : SearchPaths)
    {
        UScriptStruct* FoundStruct = LoadObject<UScriptStruct>(nullptr, *Path);
        if (FoundStruct)
        {
            return FoundStruct;
        }

        UUserDefinedStruct* UserStruct = LoadObject<UUserDefinedStruct>(nullptr, *Path);
        if (UserStruct)
        {
            return UserStruct;
        }
    }

    // Try asset registry search
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssetsByClass(UUserDefinedStruct::StaticClass()->GetClassPathName(), AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        FString AssetName = AssetData.AssetName.ToString();
        if (AssetName.Equals(StructName, ESearchCase::IgnoreCase) ||
            AssetName.EndsWith(StructName, ESearchCase::IgnoreCase))
        {
            UUserDefinedStruct* UserStruct = Cast<UUserDefinedStruct>(AssetData.GetAsset());
            if (UserStruct)
            {
                return UserStruct;
            }
        }
    }

    return nullptr;
}

bool FGetStructPinNamesCommand::IsGuidField(const FString& FieldName) const
{
    int32 LastUnderscoreIndex;
    if (FieldName.FindLastChar('_', LastUnderscoreIndex) && LastUnderscoreIndex > 0)
    {
        FString Suffix = FieldName.Mid(LastUnderscoreIndex + 1);
        // GUID suffixes are either very long or contain hex-like patterns
        return Suffix.Len() > 30 || (Suffix.Len() >= 8 && !Suffix.IsNumeric());
    }
    return false;
}

FString FGetStructPinNamesCommand::ExtractFriendlyName(const FString& GuidFieldName) const
{
    FString FriendlyName = GuidFieldName;

    // Remove GUID part (everything after last underscore if it looks like a GUID)
    int32 LastUnderscoreIndex;
    if (FriendlyName.FindLastChar('_', LastUnderscoreIndex) && LastUnderscoreIndex > 0)
    {
        FString BaseName = FriendlyName.Left(LastUnderscoreIndex);
        FString Suffix = FriendlyName.Mid(LastUnderscoreIndex + 1);

        // Check if suffix looks like a GUID (long or contains hex chars)
        if (Suffix.Len() > 30 || (Suffix.Len() >= 8 && !Suffix.IsNumeric()))
        {
            FriendlyName = BaseName;
        }
    }

    return FriendlyName;
}

FString FGetStructPinNamesCommand::CreateSuccessResponse(const FString& StructName, const FString& StructPath, const TArray<TSharedPtr<FJsonValue>>& Fields) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("struct_name"), StructName);
    ResponseObj->SetStringField(TEXT("struct_path"), StructPath);
    ResponseObj->SetNumberField(TEXT("field_count"), Fields.Num());
    ResponseObj->SetArrayField(TEXT("fields"), Fields);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Found %d fields in struct '%s'"), Fields.Num(), *StructName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetStructPinNamesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}
