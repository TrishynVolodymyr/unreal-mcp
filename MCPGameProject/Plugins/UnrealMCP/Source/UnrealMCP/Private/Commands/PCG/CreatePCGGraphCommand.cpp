#include "Commands/PCG/CreatePCGGraphCommand.h"
#include "PCGGraph.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

FString FCreatePCGGraphCommand::Execute(const FString& Parameters)
{
    FString Name;
    FString Path;
    FString TemplateName;
    FString Error;

    if (!ParseParameters(Parameters, Name, Path, TemplateName, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Build full asset path
    FString FullPath = Path / Name;
    FString PackagePath = FullPath;

    // Create the package
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to create package at %s"), *PackagePath));
    }

    UPCGGraph* Graph = nullptr;

    if (!TemplateName.IsEmpty())
    {
        // Find the template graph by name in the asset registry
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        UPCGGraph* TemplateGraph = nullptr;

        // Search all PCG Graph assets for one matching the template name
        TArray<FAssetData> AllPCGGraphs;
        AssetRegistry.GetAssetsByClass(UPCGGraph::StaticClass()->GetClassPathName(), AllPCGGraphs, /*bSearchSubClasses=*/true);

        for (const FAssetData& AssetData : AllPCGGraphs)
        {
            FString AssetName = AssetData.AssetName.ToString();
            if (AssetName.Equals(TemplateName, ESearchCase::IgnoreCase))
            {
                TemplateGraph = Cast<UPCGGraph>(AssetData.GetAsset());
                break;
            }
        }

        if (!TemplateGraph)
        {
            return CreateErrorResponse(FString::Printf(TEXT("Template not found: %s"), *TemplateName));
        }

        // Duplicate the template graph (same as UE's PCGGraphFactory)
        Graph = Cast<UPCGGraph>(StaticDuplicateObject(TemplateGraph, Package, FName(*Name), RF_Public | RF_Standalone));
        if (Graph)
        {
            Graph->bIsTemplate = false;
        }
    }
    else
    {
        // Create empty PCG Graph - constructor auto-creates InputNode and OutputNode
        Graph = NewObject<UPCGGraph>(Package, UPCGGraph::StaticClass(), FName(*Name), RF_Public | RF_Standalone);
    }

    if (!Graph)
    {
        return CreateErrorResponse(TEXT("Failed to create PCG Graph object"));
    }

    // Mark package dirty and notify asset registry
    Graph->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Graph);

    // Save the asset
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    bool bSaved = UPackage::SavePackage(Package, Graph, *PackageFileName, SaveArgs);

    if (!bSaved)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to save package at %s"), *PackageFileName));
    }

    return CreateSuccessResponse(FullPath, TemplateName);
}

FString FCreatePCGGraphCommand::GetCommandName() const
{
    return TEXT("create_pcg_graph");
}

bool FCreatePCGGraphCommand::ValidateParams(const FString& Parameters) const
{
    FString Name, Path, Template;
    FString Error;
    return ParseParameters(Parameters, Name, Path, Template, Error);
}

bool FCreatePCGGraphCommand::ParseParameters(const FString& JsonString, FString& OutName, FString& OutPath, FString& OutTemplateName, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("name"), OutName))
    {
        OutError = TEXT("Missing 'name' parameter");
        return false;
    }

    // Optional path with default
    if (!JsonObject->TryGetStringField(TEXT("path"), OutPath))
    {
        OutPath = TEXT("/Game/PCG");
    }

    // Optional template name (e.g. "TPL_Showcase_SimpleForest", "_Default_Loop")
    JsonObject->TryGetStringField(TEXT("template"), OutTemplateName);

    return true;
}

FString FCreatePCGGraphCommand::CreateSuccessResponse(const FString& GraphPath, const FString& TemplateName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("graph_path"), GraphPath);

    if (TemplateName.IsEmpty())
    {
        ResponseObj->SetStringField(TEXT("message"), TEXT("Created PCG Graph with Input and Output nodes"));
    }
    else
    {
        ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Created PCG Graph from template '%s'"), *TemplateName));
        ResponseObj->SetStringField(TEXT("template"), TemplateName);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FCreatePCGGraphCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
