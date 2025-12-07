#include "Utils/ActorUtils.h"
#include "GameFramework/Actor.h"
#include "Editor.h"
#include "EngineUtils.h"

TSharedPtr<FJsonValue> FActorUtils::ActorToJson(AActor* Actor)
{
    if (!Actor)
    {
        return MakeShared<FJsonValueNull>();
    }

    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());

    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);

    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);

    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);

    return MakeShared<FJsonValueObject>(ActorObject);
}

TSharedPtr<FJsonObject> FActorUtils::ActorToJsonObject(AActor* Actor, bool bDetailed)
{
    if (!Actor)
    {
        return nullptr;
    }

    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());

    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);

    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);

    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);

    return ActorObject;
}

AActor* FActorUtils::FindActorByName(const FString& ActorName)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return nullptr;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->GetName() == ActorName)
        {
            return Actor;
        }
    }
    return nullptr;
}

bool FActorUtils::CallFunctionByName(UObject* Target, const FString& FunctionName, const TArray<FString>& StringParams, FString& OutError)
{
    if (!Target) {
        OutError = TEXT("Target is null");
        return false;
    }
    UFunction* Function = Target->FindFunction(FName(*FunctionName));
    if (!Function) {
        OutError = FString::Printf(TEXT("Function not found: %s"), *FunctionName);
        return false;
    }
    uint8* Params = (uint8*)FMemory_Alloca(Function->ParmsSize);
    FMemory::Memzero(Params, Function->ParmsSize);

    int32 ParamIndex = 0;
    for (TFieldIterator<FProperty> It(Function); It && (It->PropertyFlags & CPF_Parm); ++It)
    {
        if (It->IsA<FStrProperty>() && ParamIndex < StringParams.Num())
        {
            FStrProperty* StrProp = CastField<FStrProperty>(*It);
            void* ValuePtr = It->ContainerPtrToValuePtr<void>(Params);
            StrProp->SetPropertyValue(ValuePtr, StringParams[ParamIndex]);
            ParamIndex++;
        }
        // Extend for other types as needed
    }
    Target->ProcessEvent(Function, Params);
    return true;
}
