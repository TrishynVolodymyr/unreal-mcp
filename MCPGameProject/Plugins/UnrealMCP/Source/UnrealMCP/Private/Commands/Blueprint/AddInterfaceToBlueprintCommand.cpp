#include "Commands/Blueprint/AddInterfaceToBlueprintCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "Engine/BlueprintGeneratedClass.h"

FAddInterfaceToBlueprintCommand::FAddInterfaceToBlueprintCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FAddInterfaceToBlueprintCommand::Execute(const FString& Parameters)
{
    FString BlueprintName, InterfaceName, ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, InterfaceName, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Validate parameters
    if (BlueprintName.IsEmpty())
    {
        return CreateErrorResponse(TEXT("Blueprint name cannot be empty"));
    }
    
    if (InterfaceName.IsEmpty())
    {
        return CreateErrorResponse(TEXT("Interface name cannot be empty"));
    }
    
    // Find the Blueprint
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    
    // Try to find blueprint by name
    TArray<FAssetData> BlueprintAssets;
    AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets);
    
    UBlueprint* TargetBlueprint = nullptr;
    for (const FAssetData& Asset : BlueprintAssets)
    {
        if (Asset.AssetName.ToString() == BlueprintName || Asset.GetObjectPathString().Contains(BlueprintName))
        {
            TargetBlueprint = Cast<UBlueprint>(Asset.GetAsset());
            break;
        }
    }
    
    if (!TargetBlueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    // Find the Interface
    UBlueprint* InterfaceBlueprint = nullptr;
    
    // Handle full path or simple name
    if (InterfaceName.StartsWith(TEXT("/Game/")))
    {
        // Full path provided
        FAssetData InterfaceAsset = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(InterfaceName));
        if (InterfaceAsset.IsValid())
        {
            InterfaceBlueprint = Cast<UBlueprint>(InterfaceAsset.GetAsset());
        }
    }
    else
    {
        // Simple name provided, search for it
        TArray<FAssetData> InterfaceAssets;
        AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), InterfaceAssets);
        
        for (const FAssetData& Asset : InterfaceAssets)
        {
            UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
            if (Blueprint && Blueprint->BlueprintType == BPTYPE_Interface)
            {
                if (Asset.AssetName.ToString() == InterfaceName || 
                    Asset.AssetName.ToString().Contains(InterfaceName) ||
                    Asset.GetObjectPathString().Contains(InterfaceName))
                {
                    InterfaceBlueprint = Blueprint;
                    break;
                }
            }
        }
    }
    
    if (!InterfaceBlueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Interface '%s' not found"), *InterfaceName));
    }
    
    if (InterfaceBlueprint->BlueprintType != BPTYPE_Interface)
    {
        return CreateErrorResponse(FString::Printf(TEXT("'%s' is not a Blueprint Interface"), *InterfaceName));
    }
    
    // Check if interface is already implemented
    UClass* InterfaceClass = InterfaceBlueprint->GeneratedClass;
    if (!InterfaceClass)
    {
        return CreateErrorResponse(TEXT("Interface class is not valid"));
    }
    
    // Check if already implemented
    for (const FBPInterfaceDescription& Interface : TargetBlueprint->ImplementedInterfaces)
    {
        if (Interface.Interface == InterfaceClass)
        {
            return CreateErrorResponse(FString::Printf(TEXT("Interface '%s' is already implemented by Blueprint '%s'"), *InterfaceName, *BlueprintName));
        }
    }
    
    // Add the interface
    FBPInterfaceDescription NewInterface;
    NewInterface.Interface = InterfaceClass;
    NewInterface.Graphs.Empty();
    
    TargetBlueprint->ImplementedInterfaces.Add(NewInterface);
    
    // Reconstruct and recompile the blueprint
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
    FBlueprintEditorUtils::RefreshAllNodes(TargetBlueprint);
    
    // Mark package as dirty
    TargetBlueprint->GetPackage()->MarkPackageDirty();
    
    return CreateSuccessResponse(BlueprintName, InterfaceName);
}

FString FAddInterfaceToBlueprintCommand::GetCommandName() const
{
    return TEXT("add_interface_to_blueprint");
}

bool FAddInterfaceToBlueprintCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, InterfaceName, ParseError;
    return ParseParameters(Parameters, BlueprintName, InterfaceName, ParseError);
}

bool FAddInterfaceToBlueprintCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutInterfaceName, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required blueprint_name parameter
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }
    
    // Parse required interface_name parameter
    if (!JsonObject->TryGetStringField(TEXT("interface_name"), OutInterfaceName))
    {
        OutError = TEXT("Missing required 'interface_name' parameter");
        return false;
    }
    
    return true;
}

FString FAddInterfaceToBlueprintCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& InterfaceName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("interface_name"), InterfaceName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully added interface '%s' to blueprint '%s'"), *InterfaceName, *BlueprintName));
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FAddInterfaceToBlueprintCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}



