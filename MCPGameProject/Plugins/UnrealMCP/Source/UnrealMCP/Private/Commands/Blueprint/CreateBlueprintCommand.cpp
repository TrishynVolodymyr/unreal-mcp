#include "Commands/Blueprint/CreateBlueprintCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Services/AssetDiscoveryService.h"
#include "AssetRegistry/AssetRegistryModule.h"

FCreateBlueprintCommand::FCreateBlueprintCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FCreateBlueprintCommand::Execute(const FString& Parameters)
{
    FBlueprintCreationParams Params;
    FString ParseError;
    
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Validate parameters
    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }
    
    // Create the blueprint using the service
    UBlueprint* CreatedBlueprint = BlueprintService.CreateBlueprint(Params);
    if (!CreatedBlueprint)
    {
        return CreateErrorResponse(TEXT("Failed to create blueprint"));
    }
    
    return CreateSuccessResponse(CreatedBlueprint);
}

FString FCreateBlueprintCommand::GetCommandName() const
{
    return TEXT("create_blueprint");
}

bool FCreateBlueprintCommand::ValidateParams(const FString& Parameters) const
{
    FBlueprintCreationParams Params;
    FString ParseError;
    
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FCreateBlueprintCommand::ParseParameters(const FString& JsonString, FBlueprintCreationParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required name parameter
    if (!JsonObject->TryGetStringField(TEXT("name"), OutParams.Name))
    {
        OutError = TEXT("Missing required 'name' parameter");
        return false;
    }
    
    // Parse optional folder_path parameter
    JsonObject->TryGetStringField(TEXT("folder_path"), OutParams.FolderPath);
    
    // Parse optional parent_class parameter
    FString ParentClassName;
    if (JsonObject->TryGetStringField(TEXT("parent_class"), ParentClassName))
    {
        // Resolve parent class using the service's resolution logic
        OutParams.ParentClass = ResolveParentClass(ParentClassName);
        if (!OutParams.ParentClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("Could not resolve parent class '%s', defaulting to AActor"), *ParentClassName);
            OutParams.ParentClass = AActor::StaticClass();
        }
    }
    else
    {
        // Default to Actor if no parent class specified
        OutParams.ParentClass = AActor::StaticClass();
    }
    
    // Parse optional compile_on_creation parameter
    JsonObject->TryGetBoolField(TEXT("compile_on_creation"), OutParams.bCompileOnCreation);
    
    return true;
}

FString FCreateBlueprintCommand::CreateSuccessResponse(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("name"), Blueprint->GetName());
    ResponseObj->SetStringField(TEXT("path"), Blueprint->GetPathName());
    ResponseObj->SetBoolField(TEXT("already_exists"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FCreateBlueprintCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

UClass* FCreateBlueprintCommand::ResolveParentClass(const FString& ParentClassName) const
{
    if (ParentClassName.IsEmpty())
    {
        return AActor::StaticClass(); // Default
    }

    // First, try to find a Blueprint parent
    UClass* BlueprintParentClass = FindBlueprintParentClass(ParentClassName);
    if (BlueprintParentClass)
    {
        return BlueprintParentClass;
    }

    // Fall back to native C++ class resolution
    FString ClassName = ParentClassName;

    // Add appropriate prefix if not present
    if (!ClassName.StartsWith(TEXT("A")) && !ClassName.StartsWith(TEXT("U")))
    {
        if (ClassName.EndsWith(TEXT("Component")))
        {
            ClassName = TEXT("U") + ClassName;
        }
        else
        {
            ClassName = TEXT("A") + ClassName;
        }
    }

    // Try direct StaticClass lookup for common classes
    if (ClassName == TEXT("APawn"))
    {
        return APawn::StaticClass();
    }
    else if (ClassName == TEXT("AActor"))
    {
        return AActor::StaticClass();
    }
    else if (ClassName == TEXT("ACharacter"))
    {
        return ACharacter::StaticClass();
    }
    else if (ClassName == TEXT("APlayerController"))
    {
        return APlayerController::StaticClass();
    }
    else if (ClassName == TEXT("AGameModeBase"))
    {
        return AGameModeBase::StaticClass();
    }
    else if (ClassName == TEXT("UActorComponent"))
    {
        return UActorComponent::StaticClass();
    }
    else if (ClassName == TEXT("USceneComponent"))
    {
        return USceneComponent::StaticClass();
    }

    // Try loading from common module paths
    TArray<FString> ModulePaths = {
        TEXT("/Script/Engine"),
        TEXT("/Script/GameplayAbilities"),
        TEXT("/Script/AIModule"),
        TEXT("/Script/Game"),
        TEXT("/Script/CoreUObject")
    };

    for (const FString& ModulePath : ModulePaths)
    {
        const FString ClassPath = FString::Printf(TEXT("%s.%s"), *ModulePath, *ClassName);
        if (UClass* FoundClass = LoadClass<UObject>(nullptr, *ClassPath))
        {
            return FoundClass;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("FCreateBlueprintCommand::ResolveParentClass: Could not resolve parent class '%s'"), *ParentClassName);
    return AActor::StaticClass(); // Fallback to Actor
}

UClass* FCreateBlueprintCommand::FindBlueprintParentClass(const FString& ParentClassName) const
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    // Handle full path (e.g., /Game/Blueprints/BP_Base or /Game/Blueprints/BP_Base.BP_Base_C)
    if (ParentClassName.StartsWith(TEXT("/Game/")) || ParentClassName.StartsWith(TEXT("/Script/")))
    {
        UBlueprint* BP = nullptr;

        // Try to load directly first (handles both package path and object path)
        BP = LoadObject<UBlueprint>(nullptr, *ParentClassName);

        if (!BP)
        {
            // If that failed and path doesn't have the _C suffix, try constructing the full object path
            if (!ParentClassName.Contains(TEXT(".")))
            {
                // Extract the asset name from the path
                FString AssetName;
                if (ParentClassName.Split(TEXT("/"), nullptr, &AssetName, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
                {
                    // Construct full object path: /Game/Path/AssetName.AssetName_C
                    const FString FullObjectPath = FString::Printf(TEXT("%s.%s_C"), *ParentClassName, *AssetName);
                    BP = LoadObject<UBlueprint>(nullptr, *FullObjectPath);

                    if (BP)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Found Blueprint parent by constructing object path: %s -> %s"), *ParentClassName, *FullObjectPath);
                    }
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Found Blueprint parent by direct load: %s"), *ParentClassName);
        }

        // Validate and return
        if (BP && BP->BlueprintType == BPTYPE_Normal && BP->GeneratedClass)
        {
            UE_LOG(LogTemp, Log, TEXT("Resolved Blueprint parent: %s -> %s"), *ParentClassName, *BP->GeneratedClass->GetName());
            return BP->GeneratedClass;
        }
    }

    // Handle simple name (e.g., BP_Base) - use FAssetDiscoveryService
    TArray<FString> FoundBlueprints = FAssetDiscoveryService::Get().FindBlueprints(ParentClassName);
    if (FoundBlueprints.Num() > 0)
    {
        UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *FoundBlueprints[0]);
        if (BP && BP->BlueprintType == BPTYPE_Normal && BP->GeneratedClass)
        {
            UE_LOG(LogTemp, Log, TEXT("Found Blueprint parent by name: %s -> %s (path: %s)"),
                *ParentClassName, *BP->GeneratedClass->GetName(), *FoundBlueprints[0]);
            return BP->GeneratedClass;
        }
    }

    return nullptr;
}



