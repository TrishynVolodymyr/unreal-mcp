#include "Commands/Material/CreateMPCCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Materials/MaterialParameterCollection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

FString FCreateMPCCommand::Execute(const FString& Parameters)
{
	FString Response;
	if (!ParseAndExecute(Parameters, Response))
	{
		return Response; // already error JSON
	}
	return Response;
}

FString FCreateMPCCommand::GetCommandName() const
{
	return TEXT("create_material_parameter_collection");
}

bool FCreateMPCCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
		return false;
	FString Name;
	return JsonObject->TryGetStringField(TEXT("name"), Name) && !Name.IsEmpty();
}

bool FCreateMPCCommand::ParseAndExecute(const FString& JsonString, FString& OutResponse)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		OutResponse = CreateErrorResponse(TEXT("Invalid JSON parameters"));
		return false;
	}

	FString Name;
	if (!JsonObject->TryGetStringField(TEXT("name"), Name))
	{
		OutResponse = CreateErrorResponse(TEXT("Missing 'name' parameter"));
		return false;
	}

	FString Path = TEXT("/Game/Materials");
	JsonObject->TryGetStringField(TEXT("path"), Path);

	// Build package path
	FString PackagePath = Path / Name;
	FString PackageName = FPackageName::ObjectPathToPackageName(PackagePath);

	// Check existing
	UPackage* ExistingPackage = FindPackage(nullptr, *PackageName);
	if (ExistingPackage)
	{
		ExistingPackage->FullyLoad();
		UMaterialParameterCollection* Existing = FindObject<UMaterialParameterCollection>(ExistingPackage, *Name);
		if (Existing)
		{
			// Return existing
			TSharedPtr<FJsonObject> Resp = MakeShared<FJsonObject>();
			Resp->SetBoolField(TEXT("success"), true);
			Resp->SetStringField(TEXT("mpc_path"), PackagePath);
			Resp->SetStringField(TEXT("message"), TEXT("MPC already exists, returning existing"));
			FString Out;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
			FJsonSerializer::Serialize(Resp.ToSharedRef(), Writer);
			OutResponse = Out;
			return true;
		}
	}

	// Create package
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		OutResponse = CreateErrorResponse(FString::Printf(TEXT("Failed to create package: %s"), *PackageName));
		return false;
	}
	Package->FullyLoad();

	// Create MPC asset
	UMaterialParameterCollection* MPC = NewObject<UMaterialParameterCollection>(
		Package, FName(*Name), RF_Public | RF_Standalone);

	if (!MPC)
	{
		OutResponse = CreateErrorResponse(TEXT("Failed to create MPC asset"));
		return false;
	}

	// Add scalar parameters
	const TArray<TSharedPtr<FJsonValue>>* ScalarParams;
	if (JsonObject->TryGetArrayField(TEXT("scalar_params"), ScalarParams))
	{
		for (const auto& ParamVal : *ScalarParams)
		{
			const TSharedPtr<FJsonObject>* ParamObj;
			if (ParamVal->TryGetObject(ParamObj))
			{
				FCollectionScalarParameter Param;
				FString ParamName;
				if ((*ParamObj)->TryGetStringField(TEXT("name"), ParamName))
				{
					Param.ParameterName = FName(*ParamName);
					double DefaultVal = 0.0;
					(*ParamObj)->TryGetNumberField(TEXT("default_value"), DefaultVal);
					Param.DefaultValue = (float)DefaultVal;
					MPC->ScalarParameters.Add(Param);
				}
			}
		}
	}

	// Add vector parameters
	const TArray<TSharedPtr<FJsonValue>>* VectorParams;
	if (JsonObject->TryGetArrayField(TEXT("vector_params"), VectorParams))
	{
		for (const auto& ParamVal : *VectorParams)
		{
			const TSharedPtr<FJsonObject>* ParamObj;
			if (ParamVal->TryGetObject(ParamObj))
			{
				FCollectionVectorParameter Param;
				FString ParamName;
				if ((*ParamObj)->TryGetStringField(TEXT("name"), ParamName))
				{
					Param.ParameterName = FName(*ParamName);

					// Parse default value as R,G,B,A
					double R = 0, G = 0, B = 0, A = 1;
					(*ParamObj)->TryGetNumberField(TEXT("r"), R);
					(*ParamObj)->TryGetNumberField(TEXT("g"), G);
					(*ParamObj)->TryGetNumberField(TEXT("b"), B);
					(*ParamObj)->TryGetNumberField(TEXT("a"), A);
					Param.DefaultValue = FLinearColor((float)R, (float)G, (float)B, (float)A);
					MPC->VectorParameters.Add(Param);
				}
			}
		}
	}

	// Register with worlds
	MPC->SetupWorldParameterCollectionInstances();

	// Register and save
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MPC);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, MPC, *PackageFileName, SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("Created MPC: %s (%d scalar, %d vector params)"),
		*PackagePath, MPC->ScalarParameters.Num(), MPC->VectorParameters.Num());

	// Build response
	TSharedPtr<FJsonObject> Resp = MakeShared<FJsonObject>();
	Resp->SetBoolField(TEXT("success"), true);
	Resp->SetStringField(TEXT("mpc_path"), PackagePath);
	Resp->SetNumberField(TEXT("scalar_count"), MPC->ScalarParameters.Num());
	Resp->SetNumberField(TEXT("vector_count"), MPC->VectorParameters.Num());
	Resp->SetStringField(TEXT("message"),
		FString::Printf(TEXT("MPC created at %s (%d scalar, %d vector params)"),
			*PackagePath, MPC->ScalarParameters.Num(), MPC->VectorParameters.Num()));

	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Resp.ToSharedRef(), Writer);
	OutResponse = Out;
	return true;
}

FString FCreateMPCCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
	ErrorObj->SetBoolField(TEXT("success"), false);
	ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
	return OutputString;
}
