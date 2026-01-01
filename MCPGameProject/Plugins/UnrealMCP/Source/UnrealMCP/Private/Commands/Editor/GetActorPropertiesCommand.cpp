#include "Commands/Editor/GetActorPropertiesCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/DataTable.h"

FGetActorPropertiesCommand::FGetActorPropertiesCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FGetActorPropertiesCommand::Execute(const FString& Parameters)
{
    FString ActorName;
    FString Error;
    
    if (!ParseParameters(Parameters, ActorName, Error))
    {
        return CreateErrorResponse(Error);
    }
    
    // Find the actor
    AActor* Actor = EditorService.FindActorByName(ActorName);
    if (!Actor)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }
    
    return CreateSuccessResponse(Actor);
}

FString FGetActorPropertiesCommand::GetCommandName() const
{
    return TEXT("get_actor_properties");
}

bool FGetActorPropertiesCommand::ValidateParams(const FString& Parameters) const
{
    FString ActorName;
    FString Error;
    return ParseParameters(Parameters, ActorName, Error);
}

bool FGetActorPropertiesCommand::ParseParameters(const FString& JsonString, FString& OutActorName, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Get required actor name
    if (!JsonObject->TryGetStringField(TEXT("name"), OutActorName))
    {
        OutError = TEXT("Missing 'name' parameter");
        return false;
    }
    
    return true;
}

FString FGetActorPropertiesCommand::CreateSuccessResponse(AActor* Actor) const
{
    if (!Actor)
    {
        return CreateErrorResponse(TEXT("Invalid actor"));
    }
    
    // Create comprehensive actor properties response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    
    // Basic actor info
    ResponseObj->SetStringField(TEXT("name"), Actor->GetName());
    ResponseObj->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    // Transform information
    FTransform Transform = Actor->GetTransform();
    TSharedPtr<FJsonObject> TransformObj = MakeShared<FJsonObject>();
    
    // Location
    FVector Location = Transform.GetLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    TransformObj->SetArrayField(TEXT("location"), LocationArray);
    
    // Rotation
    FRotator Rotation = Transform.GetRotation().Rotator();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    TransformObj->SetArrayField(TEXT("rotation"), RotationArray);
    
    // Scale
    FVector Scale = Transform.GetScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    TransformObj->SetArrayField(TEXT("scale"), ScaleArray);
    
    ResponseObj->SetObjectField(TEXT("transform"), TransformObj);
    
    // Additional properties
    ResponseObj->SetBoolField(TEXT("hidden"), Actor->IsHidden());
    ResponseObj->SetStringField(TEXT("mobility"), Actor->GetRootComponent() ?
        UEnum::GetValueAsString(Actor->GetRootComponent()->Mobility) : TEXT("Unknown"));

    // Blueprint variables - check if this is a Blueprint-generated class
    UClass* ActorClass = Actor->GetClass();
    if (ActorClass && ActorClass->IsChildOf(AActor::StaticClass()))
    {
        TSharedPtr<FJsonObject> BlueprintVarsObj = MakeShared<FJsonObject>();
        bool bHasBlueprintVars = false;

        // Iterate through all properties defined in this class (not inherited from native)
        for (TFieldIterator<FProperty> PropIt(ActorClass, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
        {
            FProperty* Property = *PropIt;
            if (!Property) continue;

            // Skip non-blueprint editable properties
            if (!Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
                continue;

            FString PropName = Property->GetName();
            void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Actor);

            // Handle different property types
            if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
            {
                FString Value = StrProp->GetPropertyValue(ValuePtr);
                BlueprintVarsObj->SetStringField(PropName, Value);
                bHasBlueprintVars = true;
            }
            else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
            {
                FName Value = NameProp->GetPropertyValue(ValuePtr);
                BlueprintVarsObj->SetStringField(PropName, Value.ToString());
                bHasBlueprintVars = true;
            }
            else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
            {
                FText Value = TextProp->GetPropertyValue(ValuePtr);
                BlueprintVarsObj->SetStringField(PropName, Value.ToString());
                bHasBlueprintVars = true;
            }
            else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
            {
                bool Value = BoolProp->GetPropertyValue(ValuePtr);
                BlueprintVarsObj->SetBoolField(PropName, Value);
                bHasBlueprintVars = true;
            }
            else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
            {
                int32 Value = IntProp->GetPropertyValue(ValuePtr);
                BlueprintVarsObj->SetNumberField(PropName, Value);
                bHasBlueprintVars = true;
            }
            else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
            {
                float Value = FloatProp->GetPropertyValue(ValuePtr);
                BlueprintVarsObj->SetNumberField(PropName, Value);
                bHasBlueprintVars = true;
            }
            else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
            {
                double Value = DoubleProp->GetPropertyValue(ValuePtr);
                BlueprintVarsObj->SetNumberField(PropName, Value);
                bHasBlueprintVars = true;
            }
            else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
            {
                UObject* ObjValue = ObjProp->GetPropertyValue(ValuePtr);
                if (ObjValue)
                {
                    // For DataTables and other assets, get the path
                    BlueprintVarsObj->SetStringField(PropName, ObjValue->GetPathName());
                }
                else
                {
                    BlueprintVarsObj->SetStringField(PropName, TEXT("None"));
                }
                bHasBlueprintVars = true;
            }
            else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
            {
                FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
                int64 Value = UnderlyingProp->GetSignedIntPropertyValue(ValuePtr);
                UEnum* Enum = EnumProp->GetEnum();
                if (Enum)
                {
                    BlueprintVarsObj->SetStringField(PropName, Enum->GetNameStringByValue(Value));
                }
                else
                {
                    BlueprintVarsObj->SetNumberField(PropName, Value);
                }
                bHasBlueprintVars = true;
            }
            else if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
            {
                uint8 Value = ByteProp->GetPropertyValue(ValuePtr);
                if (ByteProp->Enum)
                {
                    BlueprintVarsObj->SetStringField(PropName, ByteProp->Enum->GetNameStringByValue(Value));
                }
                else
                {
                    BlueprintVarsObj->SetNumberField(PropName, Value);
                }
                bHasBlueprintVars = true;
            }
        }

        if (bHasBlueprintVars)
        {
            ResponseObj->SetObjectField(TEXT("blueprint_variables"), BlueprintVarsObj);
        }
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FGetActorPropertiesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    
    return OutputString;
}

