#include "Commands/UnrealMCPEditorCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"

FUnrealMCPEditorCommands::FUnrealMCPEditorCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Actor manipulation commands
    if (CommandType == TEXT("get_actors_in_level"))
    {
        return HandleGetActorsInLevel(Params);
    }
    else if (CommandType == TEXT("find_actors_by_name"))
    {
        return HandleFindActorsByName(Params);
    }
    else if (CommandType == TEXT("spawn_actor") || CommandType == TEXT("create_actor"))
    {
        if (CommandType == TEXT("create_actor"))
        {
            UE_LOG(LogTemp, Warning, TEXT("'create_actor' command is deprecated and will be removed in a future version. Please use 'spawn_actor' instead."));
        }
        return HandleSpawnActor(Params);
    }
    else if (CommandType == TEXT("delete_actor"))
    {
        return HandleDeleteActor(Params);
    }
    else if (CommandType == TEXT("set_actor_transform"))
    {
        return HandleSetActorTransform(Params);
    }
    else if (CommandType == TEXT("get_actor_properties"))
    {
        return HandleGetActorProperties(Params);
    }
    else if (CommandType == TEXT("set_actor_property"))
    {
        return HandleSetActorProperty(Params);
    }
    else if (CommandType == TEXT("set_light_property"))
    {
        return HandleSetLightProperty(Params);
    }
    // Blueprint actor spawning
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    // Editor viewport commands
    else if (CommandType == TEXT("focus_viewport"))
    {
        return HandleFocusViewport(Params);
    }
    else if (CommandType == TEXT("take_screenshot"))
    {
        return HandleTakeScreenshot(Params);
    }
    // Asset discovery commands
    else if (CommandType == TEXT("find_assets_by_type"))
    {
        return HandleFindAssetsByType(Params);
    }
    else if (CommandType == TEXT("find_assets_by_name"))
    {
        return HandleFindAssetsByName(Params);
    }
    else if (CommandType == TEXT("find_widget_blueprints"))
    {
        return HandleFindWidgetBlueprints(Params);
    }
    else if (CommandType == TEXT("find_blueprints"))
    {
        return HandleFindBlueprints(Params);
    }
    else if (CommandType == TEXT("find_data_tables"))
    {
        return HandleFindDataTables(Params);
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (AActor* Actor : AllActors)
    {
        if (Actor)
        {
            ActorArray.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), ActorArray);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FindActorsByName: Searching for pattern '%s'"), *Pattern);

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);

    UE_LOG(LogTemp, Display, TEXT("FindActorsByName: Found %d total actors in level"), AllActors.Num());

    TArray<TSharedPtr<FJsonValue>> MatchingActors;
    for (AActor* Actor : AllActors)
    {
        if (Actor && IsValid(Actor))
        {
            FString ActorName = Actor->GetName();
            bool bMatches = false;

            // Support wildcard matching with *
            if (Pattern.Contains(TEXT("*")))
            {
                // Simple wildcard matching logic
                if (Pattern.StartsWith(TEXT("*")) && Pattern.EndsWith(TEXT("*")))
                {
                    // *text* - contains match
                    FString SearchText = Pattern.Mid(1, Pattern.Len() - 2);
                    bMatches = !SearchText.IsEmpty() && ActorName.Contains(SearchText, ESearchCase::IgnoreCase);
                }
                else if (Pattern.StartsWith(TEXT("*")))
                {
                    // *text - ends with match
                    FString SearchText = Pattern.Mid(1);
                    bMatches = !SearchText.IsEmpty() && ActorName.EndsWith(SearchText, ESearchCase::IgnoreCase);
                }
                else if (Pattern.EndsWith(TEXT("*")))
                {
                    // text* - starts with match
                    FString SearchText = Pattern.LeftChop(1);
                    bMatches = !SearchText.IsEmpty() && ActorName.StartsWith(SearchText, ESearchCase::IgnoreCase);
                }
                else
                {
                    // Complex wildcard pattern - fallback to contains match
                    FString SearchText = Pattern.Replace(TEXT("*"), TEXT(""));
                    bMatches = !SearchText.IsEmpty() && ActorName.Contains(SearchText, ESearchCase::IgnoreCase);
                }
            }
            else
            {
                // Exact match or contains
                bMatches = ActorName.Equals(Pattern, ESearchCase::IgnoreCase) ||
                          ActorName.Contains(Pattern, ESearchCase::IgnoreCase);
            }

            if (bMatches)
            {
                UE_LOG(LogTemp, Display, TEXT("FindActorsByName: Found matching actor '%s'"), *ActorName);
                MatchingActors.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
            }
        }
    }

    UE_LOG(LogTemp, Display, TEXT("FindActorsByName: Found %d matching actors"), MatchingActors.Num());

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);

    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorType;
    if (!Params->TryGetStringField(TEXT("type"), ActorType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    // Get actor name (required parameter)
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Create the actor based on type
    AActor* NewActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Check if an actor with this name already exists
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' already exists"), *ActorName));
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    if (ActorType == TEXT("StaticMeshActor"))
    {
        NewActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("PointLight"))
    {
        NewActor = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("SpotLight"))
    {
        NewActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("DirectionalLight"))
    {
        NewActor = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("CameraActor"))
    {
        NewActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown actor type: %s"), *ActorType));
    }

    if (NewActor)
    {
        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);

        // Return the created actor's details
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            // Store actor info before deletion for the response
            TSharedPtr<FJsonObject> ActorInfo = FUnrealMCPCommonUtils::ActorToJsonObject(Actor);
            
            // Delete the actor
            Actor->Destroy();
            
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
            return ResultObj;
        }
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get transform parameters
    FTransform NewTransform = TargetActor->GetTransform();

    if (Params->HasField(TEXT("location")))
    {
        NewTransform.SetLocation(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        NewTransform.SetRotation(FQuat(FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
    }
    if (Params->HasField(TEXT("scale")))
    {
        NewTransform.SetScale3D(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
    }

    // Set the new transform
    TargetActor->SetActorTransform(NewTransform);

    // Return updated actor info
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Always return detailed properties for this command
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get property name
    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    // Get property value
    if (!Params->HasField(TEXT("property_value")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }
    
    TSharedPtr<FJsonValue> PropertyValue = Params->Values.FindRef(TEXT("property_value"));
    
    // Set the property using our utility function
    FString ErrorMessage;
    if (FUnrealMCPCommonUtils::SetObjectProperty(TargetActor, PropertyName, PropertyValue, ErrorMessage))
    {
        // Property set successfully
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetBoolField(TEXT("success"), true);
        
        // Also include the full actor details
        ResultObj->SetObjectField(TEXT("actor_details"), FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true));
        return ResultObj;
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
    }
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    // Find the blueprint - using FUnrealMCPCommonUtils::FindBlueprint instead of direct path construction
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Spawn the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FQuat(Rotation));
    SpawnTransform.SetScale3D(Scale);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform, SpawnParams);
    if (NewActor)
    {
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFocusViewport(const TSharedPtr<FJsonObject>& Params)
{
    // Get target actor name if provided
    FString TargetActorName;
    bool HasTargetActor = Params->TryGetStringField(TEXT("target"), TargetActorName);

    // Get location if provided
    FVector Location(0.0f, 0.0f, 0.0f);
    bool HasLocation = false;
    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
        HasLocation = true;
    }

    // Get distance
    float Distance = 1000.0f;
    if (Params->HasField(TEXT("distance")))
    {
        Distance = Params->GetNumberField(TEXT("distance"));
    }

    // Get orientation if provided
    FRotator Orientation(0.0f, 0.0f, 0.0f);
    bool HasOrientation = false;
    if (Params->HasField(TEXT("orientation")))
    {
        Orientation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("orientation"));
        HasOrientation = true;
    }

    // Get the active viewport
    FLevelEditorViewportClient* ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
    if (!ViewportClient)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get active viewport"));
    }

    // If we have a target actor, focus on it
    if (HasTargetActor)
    {
        // Find the actor
        AActor* TargetActor = nullptr;
        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
        
        for (AActor* Actor : AllActors)
        {
            if (Actor && Actor->GetName() == TargetActorName)
            {
                TargetActor = Actor;
                break;
            }
        }

        if (!TargetActor)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *TargetActorName));
        }

        // Focus on the actor
        ViewportClient->SetViewLocation(TargetActor->GetActorLocation() - FVector(Distance, 0.0f, 0.0f));
    }
    // Otherwise use the provided location
    else if (HasLocation)
    {
        ViewportClient->SetViewLocation(Location - FVector(Distance, 0.0f, 0.0f));
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Either 'target' or 'location' must be provided"));
    }

    // Set orientation if provided
    if (HasOrientation)
    {
        ViewportClient->SetViewRotation(Orientation);
    }

    // Force viewport to redraw
    ViewportClient->Invalidate();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleTakeScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    // Get file path parameter
    FString FilePath;
    if (!Params->TryGetStringField(TEXT("filepath"), FilePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'filepath' parameter"));
    }
    
    // Ensure the file path has a proper extension
    if (!FilePath.EndsWith(TEXT(".png")))
    {
        FilePath += TEXT(".png");
    }

    // Get the active viewport
    if (GEditor && GEditor->GetActiveViewport())
    {
        FViewport* Viewport = GEditor->GetActiveViewport();
        TArray<FColor> Bitmap;
        FIntRect ViewportRect(0, 0, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
        
        if (Viewport->ReadPixels(Bitmap, FReadSurfaceDataFlags(), ViewportRect))
        {
            TArray<uint8> CompressedBitmap;
            FImageUtils::CompressImageArray(Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y, Bitmap, CompressedBitmap);
            
            if (FFileHelper::SaveArrayToFile(CompressedBitmap, *FilePath))
            {
                TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
                ResultObj->SetStringField(TEXT("filepath"), FilePath);
                return ResultObj;
            }
        }
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to take screenshot"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetLightProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    FString PropertyValue;
    if (!Params->TryGetStringField(TEXT("property_value"), PropertyValue))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }

    // Get the specified actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' not found"), *ActorName));
    }

    // Find the light component
    ULightComponent* LightComponent = nullptr;
    
    // Check if it's one of the built-in light types
    if (APointLight* PointLight = Cast<APointLight>(TargetActor))
    {
        LightComponent = PointLight->GetLightComponent();
    }
    else if (ASpotLight* SpotLight = Cast<ASpotLight>(TargetActor))
    {
        LightComponent = SpotLight->GetLightComponent();
    }
    else if (ADirectionalLight* DirLight = Cast<ADirectionalLight>(TargetActor))
    {
        LightComponent = DirLight->GetLightComponent();
    }
    else
    {
        // Try to find any light component in the actor
        LightComponent = TargetActor->FindComponentByClass<ULightComponent>();
    }

    if (!LightComponent)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Cannot find light component on actor: %s"), *ActorName));
    }

    // Set the property on the light component
    if (PropertyName == TEXT("Intensity"))
    {
        float Value = FCString::Atof(*PropertyValue);
        LightComponent->SetIntensity(Value);
    }
    else if (PropertyName == TEXT("LightColor"))
    {
        TArray<FString> ColorValues;
        PropertyValue.ParseIntoArray(ColorValues, TEXT(","), true);
        
        if (ColorValues.Num() >= 3)
        {
            float R = FCString::Atof(*ColorValues[0]);
            float G = FCString::Atof(*ColorValues[1]);
            float B = FCString::Atof(*ColorValues[2]);
            
            LightComponent->SetLightColor(FLinearColor(R, G, B));
        }
        else
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid color format. Expected R,G,B values."));
        }
    }
    else if (PropertyName == TEXT("AttenuationRadius"))
    {
        float Value = FCString::Atof(*PropertyValue);
        if (UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(LightComponent))
        {
            // Set property directly and mark render state dirty
            PointLightComp->AttenuationRadius = Value;
            PointLightComp->MarkRenderStateDirty();
        }
        else if (USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(LightComponent))
        {
            // Set property directly and mark render state dirty
            SpotLightComp->AttenuationRadius = Value;
            SpotLightComp->MarkRenderStateDirty();
        }
        else
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AttenuationRadius is only applicable for point and spot lights"));
        }
    }
    else if (PropertyName == TEXT("SourceRadius"))
    {
        float Value = FCString::Atof(*PropertyValue);
        // Source radius is only available on certain light types
        if (UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(LightComponent))
        {
            PointLightComp->SourceRadius = Value;
            PointLightComp->MarkRenderStateDirty();
        }
        else if (USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(LightComponent))
        {
            SpotLightComp->SourceRadius = Value;
            SpotLightComp->MarkRenderStateDirty();
        }
        else
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("SourceRadius is only applicable for point and spot lights"));
        }
    }
    else if (PropertyName == TEXT("SoftSourceRadius"))
    {
        float Value = FCString::Atof(*PropertyValue);
        // Soft source radius is only available on certain light types
        if (UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(LightComponent))
        {
            PointLightComp->SoftSourceRadius = Value;
            PointLightComp->MarkRenderStateDirty();
        }
        else if (USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(LightComponent))
        {
            SpotLightComp->SoftSourceRadius = Value;
            SpotLightComp->MarkRenderStateDirty();
        }
        else
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("SoftSourceRadius is only applicable for point and spot lights"));
        }
    }
    else if (PropertyName == TEXT("CastShadows"))
    {
        bool Value = PropertyValue.ToBool();
        LightComponent->SetCastShadows(Value);
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown light property: %s"), *PropertyName));
    }

    // Mark the component as modified
    LightComponent->MarkPackageDirty();
    
    TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
    ResultData->SetStringField(TEXT("actor"), ActorName);
    ResultData->SetStringField(TEXT("property"), PropertyName);
    ResultData->SetBoolField(TEXT("success"), true);
    ResultData->SetStringField(TEXT("message"), FString::Printf(TEXT("Set light property %s on %s"), *PropertyName, *ActorName));
    
    return ResultData;
}

// ==================== Asset Discovery Command Implementations ====================

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindAssetsByType(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetType;
    if (!Params->TryGetStringField(TEXT("asset_type"), AssetType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_type' parameter"));
    }
    
    FString SearchPath = TEXT("/Game");
    Params->TryGetStringField(TEXT("search_path"), SearchPath);
    
    TArray<FString> FoundAssets = FUnrealMCPCommonUtils::FindAssetsByType(AssetType, SearchPath);
    
    TArray<TSharedPtr<FJsonValue>> AssetArray;
    for (const FString& AssetPath : FoundAssets)
    {
        AssetArray.Add(MakeShared<FJsonValueString>(AssetPath));
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("assets"), AssetArray);
    ResultObj->SetNumberField(TEXT("count"), FoundAssets.Num());
    ResultObj->SetStringField(TEXT("asset_type"), AssetType);
    ResultObj->SetStringField(TEXT("search_path"), SearchPath);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindAssetsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetName;
    if (!Params->TryGetStringField(TEXT("asset_name"), AssetName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_name' parameter"));
    }
    
    FString SearchPath = TEXT("/Game");
    Params->TryGetStringField(TEXT("search_path"), SearchPath);
    
    TArray<FString> FoundAssets = FUnrealMCPCommonUtils::FindAssetsByName(AssetName, SearchPath);
    
    TArray<TSharedPtr<FJsonValue>> AssetArray;
    for (const FString& AssetPath : FoundAssets)
    {
        AssetArray.Add(MakeShared<FJsonValueString>(AssetPath));
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("assets"), AssetArray);
    ResultObj->SetNumberField(TEXT("count"), FoundAssets.Num());
    ResultObj->SetStringField(TEXT("asset_name"), AssetName);
    ResultObj->SetStringField(TEXT("search_path"), SearchPath);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindWidgetBlueprints(const TSharedPtr<FJsonObject>& Params)
{
    FString WidgetName = TEXT("");
    Params->TryGetStringField(TEXT("widget_name"), WidgetName);
    
    FString SearchPath = TEXT("/Game");
    Params->TryGetStringField(TEXT("search_path"), SearchPath);
    
    TArray<FString> FoundWidgets = FUnrealMCPCommonUtils::FindWidgetBlueprints(WidgetName, SearchPath);
    
    TArray<TSharedPtr<FJsonValue>> WidgetArray;
    for (const FString& WidgetPath : FoundWidgets)
    {
        WidgetArray.Add(MakeShared<FJsonValueString>(WidgetPath));
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("widgets"), WidgetArray);
    ResultObj->SetNumberField(TEXT("count"), FoundWidgets.Num());
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("search_path"), SearchPath);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindBlueprints(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName = TEXT("");
    Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
    
    FString SearchPath = TEXT("/Game");
    Params->TryGetStringField(TEXT("search_path"), SearchPath);
    
    TArray<FString> FoundBlueprints = FUnrealMCPCommonUtils::FindBlueprints(BlueprintName, SearchPath);
    
    TArray<TSharedPtr<FJsonValue>> BlueprintArray;
    for (const FString& BlueprintPath : FoundBlueprints)
    {
        BlueprintArray.Add(MakeShared<FJsonValueString>(BlueprintPath));
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("blueprints"), BlueprintArray);
    ResultObj->SetNumberField(TEXT("count"), FoundBlueprints.Num());
    ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResultObj->SetStringField(TEXT("search_path"), SearchPath);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindDataTables(const TSharedPtr<FJsonObject>& Params)
{
    FString TableName = TEXT("");
    Params->TryGetStringField(TEXT("table_name"), TableName);
    
    FString SearchPath = TEXT("/Game");
    Params->TryGetStringField(TEXT("search_path"), SearchPath);
    
    TArray<FString> FoundTables = FUnrealMCPCommonUtils::FindDataTables(TableName, SearchPath);
    
    TArray<TSharedPtr<FJsonValue>> TableArray;
    for (const FString& TablePath : FoundTables)
    {
        TableArray.Add(MakeShared<FJsonValueString>(TablePath));
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("data_tables"), TableArray);
    ResultObj->SetNumberField(TEXT("count"), FoundTables.Num());
    ResultObj->SetStringField(TEXT("table_name"), TableName);
    ResultObj->SetStringField(TEXT("search_path"), SearchPath);
    
    return ResultObj;
} 