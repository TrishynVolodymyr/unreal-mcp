#include "Commands/Blueprint/CreateBlueprintInterfaceCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "Engine/BlueprintGeneratedClass.h"

FCreateBlueprintInterfaceCommand::FCreateBlueprintInterfaceCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FCreateBlueprintInterfaceCommand::Execute(const FString& Parameters)
{
    FString Name, FolderPath, ParseError;
    
    if (!ParseParameters(Parameters, Name, FolderPath, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Validate name
    if (Name.IsEmpty())
    {
        return CreateErrorResponse(TEXT("Interface name cannot be empty"));
    }
    
    // Add BPI_ prefix if not present
    if (!Name.StartsWith(TEXT("BPI_")))
    {
        Name = TEXT("BPI_") + Name;
    }
    
    // Set default folder path if empty
    if (FolderPath.IsEmpty())
    {
        FolderPath = TEXT("/Game/Blueprints");
    }
    
    // Ensure folder path starts with /Game/
    if (!FolderPath.StartsWith(TEXT("/Game/")))
    {
        FolderPath = TEXT("/Game/") + FolderPath;
    }
    
    // Create full package path
    FString PackagePath = FolderPath + TEXT("/") + Name;
    
    // Check if asset already exists
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    FAssetData ExistingAsset = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(PackagePath + TEXT(".") + Name));
    if (ExistingAsset.IsValid())
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint Interface '%s' already exists at path '%s'"), *Name, *PackagePath));
    }
    
    // Create package
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return CreateErrorResponse(TEXT("Failed to create package for Blueprint Interface"));
    }
    
    // Create Blueprint Interface
    UBlueprint* NewInterface = FKismetEditorUtilities::CreateBlueprint(
        UInterface::StaticClass(),
        Package,
        *Name,
        BPTYPE_Interface,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        FName("CreateBlueprintInterfaceCommand")
    );
    
    if (!NewInterface)
    {
        return CreateErrorResponse(TEXT("Failed to create Blueprint Interface"));
    }
    
    // Mark package as dirty and save
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewInterface);
    
    return CreateSuccessResponse(Name, PackagePath);
}

FString FCreateBlueprintInterfaceCommand::GetCommandName() const
{
    return TEXT("create_blueprint_interface");
}

bool FCreateBlueprintInterfaceCommand::ValidateParams(const FString& Parameters) const
{
    FString Name, FolderPath, ParseError;
    return ParseParameters(Parameters, Name, FolderPath, ParseError);
}

bool FCreateBlueprintInterfaceCommand::ParseParameters(const FString& JsonString, FString& OutName, FString& OutFolderPath, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required name parameter
    if (!JsonObject->TryGetStringField(TEXT("name"), OutName))
    {
        OutError = TEXT("Missing required 'name' parameter");
        return false;
    }
    
    // Parse optional folder_path parameter
    JsonObject->TryGetStringField(TEXT("folder_path"), OutFolderPath);
    
    return true;
}

FString FCreateBlueprintInterfaceCommand::CreateSuccessResponse(const FString& InterfaceName, const FString& InterfacePath) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("name"), InterfaceName);
    ResponseObj->SetStringField(TEXT("path"), InterfacePath);
    ResponseObj->SetBoolField(TEXT("already_exists"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FCreateBlueprintInterfaceCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}



