#include "Commands/Material/CreateMaterialFunctionCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Materials/MaterialFunction.h"
#include "Factories/MaterialFunctionFactoryNew.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

FString FCreateMaterialFunctionCommand::Execute(const FString& Parameters)
{
	FParams Params;
	FString Error;

	if (!ParseParameters(Parameters, Params, Error))
	{
		return CreateErrorResponse(Error);
	}

	// Build full path
	FString PackagePath = Params.Path / Params.Name;
	FString PackageName = FPackageName::ObjectPathToPackageName(PackagePath);

	// Check if already exists
	UPackage* ExistingPackage = FindPackage(nullptr, *PackageName);
	if (ExistingPackage)
	{
		ExistingPackage->FullyLoad();
		UMaterialFunction* Existing = FindObject<UMaterialFunction>(ExistingPackage, *Params.Name);
		if (Existing)
		{
			return CreateSuccessResponse(PackagePath);
		}
	}

	// Create package
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to create package: %s"), *PackageName));
	}
	Package->FullyLoad();

	// Create via factory
	UMaterialFunctionFactoryNew* Factory = NewObject<UMaterialFunctionFactoryNew>();
	UMaterialFunction* MatFunc = Cast<UMaterialFunction>(Factory->FactoryCreateNew(
		UMaterialFunction::StaticClass(),
		Package,
		*Params.Name,
		RF_Public | RF_Standalone,
		nullptr,
		GWarn
	));

	if (!MatFunc)
	{
		return CreateErrorResponse(TEXT("Failed to create MaterialFunction asset"));
	}

	// Set metadata
	MatFunc->Description = Params.Description;
	MatFunc->bExposeToLibrary = Params.bExposeToLibrary;

	for (const FString& Category : Params.LibraryCategories)
	{
		MatFunc->LibraryCategoriesText.Add(FText::FromString(Category));
	}

	// If no categories specified, add a default
	if (MatFunc->LibraryCategoriesText.Num() == 0 && Params.bExposeToLibrary)
	{
		MatFunc->LibraryCategoriesText.Add(FText::FromString(TEXT("Custom")));
	}

	// Register and save
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MatFunc);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, MatFunc, *PackageFileName, SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("Created MaterialFunction: %s"), *PackagePath);

	return CreateSuccessResponse(PackagePath);
}

FString FCreateMaterialFunctionCommand::GetCommandName() const
{
	return TEXT("create_material_function");
}

bool FCreateMaterialFunctionCommand::ValidateParams(const FString& Parameters) const
{
	FParams Params;
	FString Error;
	return ParseParameters(Parameters, Params, Error);
}

bool FCreateMaterialFunctionCommand::ParseParameters(const FString& JsonString, FParams& OutParams, FString& OutError) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		OutError = TEXT("Invalid JSON parameters");
		return false;
	}

	if (!JsonObject->TryGetStringField(TEXT("name"), OutParams.Name))
	{
		OutError = TEXT("Missing 'name' parameter");
		return false;
	}

	JsonObject->TryGetStringField(TEXT("path"), OutParams.Path);
	JsonObject->TryGetStringField(TEXT("description"), OutParams.Description);
	JsonObject->TryGetBoolField(TEXT("expose_to_library"), OutParams.bExposeToLibrary);

	const TArray<TSharedPtr<FJsonValue>>* CategoriesArray;
	if (JsonObject->TryGetArrayField(TEXT("library_categories"), CategoriesArray))
	{
		for (const auto& Val : *CategoriesArray)
		{
			OutParams.LibraryCategories.Add(Val->AsString());
		}
	}

	return OutParams.IsValid(OutError);
}

FString FCreateMaterialFunctionCommand::CreateSuccessResponse(const FString& FunctionPath) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), true);
	ResponseObj->SetStringField(TEXT("function_path"), FunctionPath);
	ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("MaterialFunction created at %s"), *FunctionPath));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
	return OutputString;
}

FString FCreateMaterialFunctionCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
	ErrorObj->SetBoolField(TEXT("success"), false);
	ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
	return OutputString;
}
