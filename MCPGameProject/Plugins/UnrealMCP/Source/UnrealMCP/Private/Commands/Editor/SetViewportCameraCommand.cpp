#include "Commands/Editor/SetViewportCameraCommand.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Editor.h"
#include "LevelEditorViewport.h"

namespace
{
    bool ParseVec3(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, FVector& Out)
    {
        const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
        if (Obj->TryGetArrayField(Field, Arr) && Arr && Arr->Num() == 3)
        {
            Out.X = (*Arr)[0]->AsNumber();
            Out.Y = (*Arr)[1]->AsNumber();
            Out.Z = (*Arr)[2]->AsNumber();
            return true;
        }
        return false;
    }

    FString SerializeJson(const TSharedPtr<FJsonObject>& Obj)
    {
        FString Out;
        TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
        FJsonSerializer::Serialize(Obj.ToSharedRef(), W);
        return Out;
    }

    FString ErrJson(const FString& Msg)
    {
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetBoolField(TEXT("success"), false);
        R->SetStringField(TEXT("error"), Msg);
        return SerializeJson(R);
    }

    // Prefer a real perspective level viewport; fall back to the last-active client.
    FLevelEditorViewportClient* FindPerspectiveViewport()
    {
        if (GEditor)
        {
            for (FLevelEditorViewportClient* Client : GEditor->GetLevelViewportClients())
            {
                if (Client && Client->IsPerspective())
                {
                    return Client;
                }
            }
        }
        return GCurrentLevelEditingViewportClient;
    }
}

FString FSetViewportCameraCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return ErrJson(TEXT("Invalid JSON parameters"));
    }

    FLevelEditorViewportClient* VC = FindPerspectiveViewport();
    if (!VC)
    {
        return ErrJson(TEXT("No perspective level editor viewport found"));
    }

    // Location: explicit, else keep current.
    FVector Location = VC->GetViewLocation();
    ParseVec3(JsonObject, TEXT("location"), Location);

    // Orientation: look_at takes priority (aim at a point — ideal for orbiting),
    // else explicit [pitch, yaw, roll], else keep current.
    FRotator Rotation = VC->GetViewRotation();
    FVector LookAt;
    FVector RotArr;
    if (ParseVec3(JsonObject, TEXT("look_at"), LookAt))
    {
        Rotation = (LookAt - Location).Rotation();
    }
    else if (ParseVec3(JsonObject, TEXT("rotation"), RotArr))
    {
        Rotation = FRotator(RotArr.X, RotArr.Y, RotArr.Z);
    }

    VC->SetViewLocation(Location);
    VC->SetViewRotation(Rotation);

    double Fov = 0.0;
    if (JsonObject->TryGetNumberField(TEXT("fov"), Fov) && Fov > 1.0 && Fov < 170.0)
    {
        VC->ViewFOV = (float)Fov;
    }

    VC->Invalidate();
    // Force an immediate redraw so a following capture_viewport_screenshot sees the new view.
    if (GEditor)
    {
        GEditor->RedrawLevelEditingViewports(true);
    }

    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    R->SetBoolField(TEXT("success"), true);

    TArray<TSharedPtr<FJsonValue>> LocOut;
    LocOut.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocOut.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocOut.Add(MakeShared<FJsonValueNumber>(Location.Z));
    R->SetArrayField(TEXT("location"), LocOut);

    TArray<TSharedPtr<FJsonValue>> RotOut;
    RotOut.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotOut.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotOut.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    R->SetArrayField(TEXT("rotation"), RotOut);

    R->SetStringField(TEXT("message"), TEXT("Viewport camera updated"));
    return SerializeJson(R);
}

FString FSetViewportCameraCommand::GetCommandName() const { return TEXT("set_viewport_camera"); }
bool FSetViewportCameraCommand::ValidateParams(const FString& Parameters) const { return true; }
