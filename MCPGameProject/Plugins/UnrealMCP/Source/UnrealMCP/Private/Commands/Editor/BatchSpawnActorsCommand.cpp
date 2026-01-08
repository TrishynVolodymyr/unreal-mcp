#include "Commands/Editor/BatchSpawnActorsCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FBatchSpawnActorsCommand::FBatchSpawnActorsCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FBatchSpawnActorsCommand::Execute(const FString& Parameters)
{
    TArray<TSharedPtr<FJsonObject>> ActorConfigs;
    FString Error;

    if (!ParseParameters(Parameters, ActorConfigs, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (ActorConfigs.Num() == 0)
    {
        return CreateErrorResponse(TEXT("No actor configurations provided"));
    }

    TArray<TSharedPtr<FJsonObject>> Results;
    int32 SuccessCount = 0;
    int32 FailedCount = 0;

    for (const TSharedPtr<FJsonObject>& ActorConfig : ActorConfigs)
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();

        // Get actor name for result tracking
        FString ActorName;
        ActorConfig->TryGetStringField(TEXT("name"), ActorName);
        ResultObj->SetStringField(TEXT("name"), ActorName);

        // Parse actor configuration
        FActorSpawnParams SpawnParams;
        FString ParseError;
        if (!ParseActorConfig(ActorConfig, SpawnParams, ParseError))
        {
            ResultObj->SetBoolField(TEXT("success"), false);
            ResultObj->SetStringField(TEXT("error"), ParseError);
            FailedCount++;
            Results.Add(ResultObj);
            continue;
        }

        // Attempt to spawn the actor
        FString SpawnError;
        AActor* SpawnedActor = EditorService.SpawnActor(SpawnParams, SpawnError);
        if (SpawnedActor)
        {
            ResultObj->SetBoolField(TEXT("success"), true);
            // Include actor details in response
            TSharedPtr<FJsonObject> ActorJson = FUnrealMCPCommonUtils::ActorToJsonObject(SpawnedActor, true);
            ResultObj->SetObjectField(TEXT("actor"), ActorJson);
            SuccessCount++;
        }
        else
        {
            ResultObj->SetBoolField(TEXT("success"), false);
            ResultObj->SetStringField(TEXT("error"), SpawnError);
            FailedCount++;
        }

        Results.Add(ResultObj);
    }

    return CreateSuccessResponse(Results, ActorConfigs.Num(), SuccessCount, FailedCount);
}

FString FBatchSpawnActorsCommand::GetCommandName() const
{
    return TEXT("batch_spawn_actors");
}

bool FBatchSpawnActorsCommand::ValidateParams(const FString& Parameters) const
{
    TArray<TSharedPtr<FJsonObject>> ActorConfigs;
    FString Error;
    return ParseParameters(Parameters, ActorConfigs, Error);
}

bool FBatchSpawnActorsCommand::ParseParameters(const FString& JsonString, TArray<TSharedPtr<FJsonObject>>& OutActorConfigs, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    // Get the actors array
    const TArray<TSharedPtr<FJsonValue>>* ActorsArray;
    if (!JsonObject->TryGetArrayField(TEXT("actors"), ActorsArray))
    {
        OutError = TEXT("Missing 'actors' array parameter");
        return false;
    }

    // Parse each actor configuration
    for (const TSharedPtr<FJsonValue>& Value : *ActorsArray)
    {
        if (Value.IsValid() && Value->Type == EJson::Object)
        {
            OutActorConfigs.Add(Value->AsObject());
        }
    }

    return true;
}

bool FBatchSpawnActorsCommand::ParseActorConfig(const TSharedPtr<FJsonObject>& ActorConfig, FActorSpawnParams& OutParams, FString& OutError) const
{
    if (!ActorConfig.IsValid())
    {
        OutError = TEXT("Invalid actor configuration");
        return false;
    }

    // Get required parameters
    if (!ActorConfig->TryGetStringField(TEXT("type"), OutParams.Type))
    {
        OutError = TEXT("Missing 'type' parameter");
        return false;
    }

    if (!ActorConfig->TryGetStringField(TEXT("name"), OutParams.Name))
    {
        OutError = TEXT("Missing 'name' parameter");
        return false;
    }

    // Get optional transform parameters
    if (ActorConfig->HasField(TEXT("location")))
    {
        OutParams.Location = FUnrealMCPCommonUtils::GetVectorFromJson(ActorConfig, TEXT("location"));
    }
    if (ActorConfig->HasField(TEXT("rotation")))
    {
        OutParams.Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(ActorConfig, TEXT("rotation"));
    }
    if (ActorConfig->HasField(TEXT("scale")))
    {
        OutParams.Scale = FUnrealMCPCommonUtils::GetVectorFromJson(ActorConfig, TEXT("scale"));
    }

    // StaticMeshActor parameters
    ActorConfig->TryGetStringField(TEXT("mesh_path"), OutParams.MeshPath);

    // TextRenderActor parameters
    ActorConfig->TryGetStringField(TEXT("text_content"), OutParams.TextContent);
    if (ActorConfig->HasField(TEXT("text_size")))
    {
        OutParams.TextSize = ActorConfig->GetNumberField(TEXT("text_size"));
    }
    if (ActorConfig->HasField(TEXT("text_color")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (ActorConfig->TryGetArrayField(TEXT("text_color"), ColorArray) && ColorArray->Num() >= 3)
        {
            OutParams.TextColor.R = (*ColorArray)[0]->AsNumber();
            OutParams.TextColor.G = (*ColorArray)[1]->AsNumber();
            OutParams.TextColor.B = (*ColorArray)[2]->AsNumber();
            OutParams.TextColor.A = ColorArray->Num() >= 4 ? (*ColorArray)[3]->AsNumber() : 1.0f;
        }
    }
    if (ActorConfig->HasField(TEXT("text_halign")))
    {
        FString HAlign = ActorConfig->GetStringField(TEXT("text_halign"));
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
            OutParams.TextHAlign = 1;
        }
    }
    if (ActorConfig->HasField(TEXT("text_valign")))
    {
        FString VAlign = ActorConfig->GetStringField(TEXT("text_valign"));
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
            OutParams.TextVAlign = 1;
        }
    }

    // Volume parameters
    if (ActorConfig->HasField(TEXT("box_extent")))
    {
        OutParams.BoxExtent = FUnrealMCPCommonUtils::GetVectorFromJson(ActorConfig, TEXT("box_extent"));
    }
    if (ActorConfig->HasField(TEXT("sphere_radius")))
    {
        OutParams.SphereRadius = ActorConfig->GetNumberField(TEXT("sphere_radius"));
    }

    // PlayerStart parameters
    ActorConfig->TryGetStringField(TEXT("player_start_tag"), OutParams.PlayerStartTag);

    // DecalActor parameters
    if (ActorConfig->HasField(TEXT("decal_size")))
    {
        OutParams.DecalSize = FUnrealMCPCommonUtils::GetVectorFromJson(ActorConfig, TEXT("decal_size"));
    }
    ActorConfig->TryGetStringField(TEXT("decal_material"), OutParams.DecalMaterialPath);

    // Validate parameters
    return OutParams.IsValid(OutError);
}

FString FBatchSpawnActorsCommand::CreateSuccessResponse(const TArray<TSharedPtr<FJsonObject>>& Results, int32 TotalCount, int32 SuccessCount, int32 FailedCount) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();

    // Convert results array to JSON values
    TArray<TSharedPtr<FJsonValue>> ResultsArray;
    for (const TSharedPtr<FJsonObject>& Result : Results)
    {
        ResultsArray.Add(MakeShared<FJsonValueObject>(Result));
    }

    ResponseObj->SetArrayField(TEXT("results"), ResultsArray);
    ResponseObj->SetNumberField(TEXT("total"), TotalCount);
    ResponseObj->SetNumberField(TEXT("succeeded"), SuccessCount);
    ResponseObj->SetNumberField(TEXT("failed"), FailedCount);
    ResponseObj->SetBoolField(TEXT("success"), true);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FBatchSpawnActorsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
