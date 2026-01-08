#include "Commands/Editor/SpawnActorCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSpawnActorCommand::FSpawnActorCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FSpawnActorCommand::Execute(const FString& Parameters)
{
    FActorSpawnParams SpawnParams;
    FString Error;
    
    if (!ParseParameters(Parameters, SpawnParams, Error))
    {
        return CreateErrorResponse(Error);
    }
    
    AActor* SpawnedActor = EditorService.SpawnActor(SpawnParams, Error);
    if (!SpawnedActor)
    {
        return CreateErrorResponse(Error);
    }
    
    return CreateSuccessResponse(SpawnedActor);
}

FString FSpawnActorCommand::GetCommandName() const
{
    return TEXT("spawn_actor");
}

bool FSpawnActorCommand::ValidateParams(const FString& Parameters) const
{
    FActorSpawnParams SpawnParams;
    FString Error;
    return ParseParameters(Parameters, SpawnParams, Error);
}

bool FSpawnActorCommand::ParseParameters(const FString& JsonString, FActorSpawnParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    // Get required parameters
    if (!JsonObject->TryGetStringField(TEXT("type"), OutParams.Type))
    {
        OutError = TEXT("Missing 'type' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("name"), OutParams.Name))
    {
        OutError = TEXT("Missing 'name' parameter");
        return false;
    }

    // Get optional transform parameters
    if (JsonObject->HasField(TEXT("location")))
    {
        OutParams.Location = FUnrealMCPCommonUtils::GetVectorFromJson(JsonObject, TEXT("location"));
    }
    if (JsonObject->HasField(TEXT("rotation")))
    {
        OutParams.Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(JsonObject, TEXT("rotation"));
    }
    if (JsonObject->HasField(TEXT("scale")))
    {
        OutParams.Scale = FUnrealMCPCommonUtils::GetVectorFromJson(JsonObject, TEXT("scale"));
    }

    // ========================================
    // StaticMeshActor parameters
    // ========================================
    JsonObject->TryGetStringField(TEXT("mesh_path"), OutParams.MeshPath);

    // ========================================
    // TextRenderActor parameters
    // ========================================
    JsonObject->TryGetStringField(TEXT("text_content"), OutParams.TextContent);
    if (JsonObject->HasField(TEXT("text_size")))
    {
        OutParams.TextSize = JsonObject->GetNumberField(TEXT("text_size"));
    }
    if (JsonObject->HasField(TEXT("text_color")))
    {
        // Parse color as array [R, G, B] or [R, G, B, A] with values 0-1
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (JsonObject->TryGetArrayField(TEXT("text_color"), ColorArray) && ColorArray->Num() >= 3)
        {
            OutParams.TextColor.R = (*ColorArray)[0]->AsNumber();
            OutParams.TextColor.G = (*ColorArray)[1]->AsNumber();
            OutParams.TextColor.B = (*ColorArray)[2]->AsNumber();
            OutParams.TextColor.A = ColorArray->Num() >= 4 ? (*ColorArray)[3]->AsNumber() : 1.0f;
        }
    }
    if (JsonObject->HasField(TEXT("text_halign")))
    {
        FString HAlign = JsonObject->GetStringField(TEXT("text_halign"));
        if (HAlign.Equals(TEXT("Left"), ESearchCase::IgnoreCase))
        {
            OutParams.TextHAlign = 0;
        }
        else if (HAlign.Equals(TEXT("Right"), ESearchCase::IgnoreCase))
        {
            OutParams.TextHAlign = 2;
        }
        else
        {
            OutParams.TextHAlign = 1; // Center (default)
        }
    }
    if (JsonObject->HasField(TEXT("text_valign")))
    {
        FString VAlign = JsonObject->GetStringField(TEXT("text_valign"));
        if (VAlign.Equals(TEXT("Top"), ESearchCase::IgnoreCase))
        {
            OutParams.TextVAlign = 0;
        }
        else if (VAlign.Equals(TEXT("Bottom"), ESearchCase::IgnoreCase))
        {
            OutParams.TextVAlign = 2;
        }
        else
        {
            OutParams.TextVAlign = 1; // Center (default)
        }
    }

    // ========================================
    // Volume parameters
    // ========================================
    if (JsonObject->HasField(TEXT("box_extent")))
    {
        OutParams.BoxExtent = FUnrealMCPCommonUtils::GetVectorFromJson(JsonObject, TEXT("box_extent"));
    }
    if (JsonObject->HasField(TEXT("sphere_radius")))
    {
        OutParams.SphereRadius = JsonObject->GetNumberField(TEXT("sphere_radius"));
    }

    // ========================================
    // PlayerStart parameters
    // ========================================
    JsonObject->TryGetStringField(TEXT("player_start_tag"), OutParams.PlayerStartTag);

    // ========================================
    // DecalActor parameters
    // ========================================
    if (JsonObject->HasField(TEXT("decal_size")))
    {
        OutParams.DecalSize = FUnrealMCPCommonUtils::GetVectorFromJson(JsonObject, TEXT("decal_size"));
    }
    JsonObject->TryGetStringField(TEXT("decal_material"), OutParams.DecalMaterialPath);

    // ========================================
    // InvisibleWall / Collision parameters
    // ========================================
    if (JsonObject->HasField(TEXT("hidden_in_game")))
    {
        OutParams.bHiddenInGame = JsonObject->GetBoolField(TEXT("hidden_in_game"));
    }
    if (JsonObject->HasField(TEXT("blocks_all")))
    {
        OutParams.bBlocksAll = JsonObject->GetBoolField(TEXT("blocks_all"));
    }
    if (JsonObject->HasField(TEXT("show_collision_in_editor")))
    {
        OutParams.bShowCollisionInEditor = JsonObject->GetBoolField(TEXT("show_collision_in_editor"));
    }

    // Validate parameters
    return OutParams.IsValid(OutError);
}

FString FSpawnActorCommand::CreateSuccessResponse(AActor* Actor) const
{
    if (!Actor)
    {
        return CreateErrorResponse(TEXT("Invalid actor"));
    }
    
    TSharedPtr<FJsonObject> ActorJson = FUnrealMCPCommonUtils::ActorToJsonObject(Actor, true);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ActorJson.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FSpawnActorCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    
    return OutputString;
}

