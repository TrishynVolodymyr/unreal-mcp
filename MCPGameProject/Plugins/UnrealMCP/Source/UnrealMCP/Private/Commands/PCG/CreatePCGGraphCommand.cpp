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
    FString Error;

    if (!ParseParameters(Parameters, Name, Path, Error))
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

    // Create the PCG Graph - constructor auto-creates InputNode and OutputNode
    UPCGGraph* Graph = NewObject<UPCGGraph>(Package, UPCGGraph::StaticClass(), FName(*Name), RF_Public | RF_Standalone);
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

    return CreateSuccessResponse(FullPath);
}

FString FCreatePCGGraphCommand::GetCommandName() const
{
    return TEXT("create_pcg_graph");
}

bool FCreatePCGGraphCommand::ValidateParams(const FString& Parameters) const
{
    FString Name;
    FString Path;
    FString Error;
    return ParseParameters(Parameters, Name, Path, Error);
}

bool FCreatePCGGraphCommand::ParseParameters(const FString& JsonString, FString& OutName, FString& OutPath, FString& OutError) const
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

    return true;
}

FString FCreatePCGGraphCommand::CreateSuccessResponse(const FString& GraphPath) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("graph_path"), GraphPath);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Created PCG Graph with Input and Output nodes"));

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
