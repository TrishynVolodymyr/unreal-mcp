#if WITH_DEV_AUTOMATION_TESTS

#include "Commands/Material/CreateMaterialInstanceCommand.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Services/IMaterialService.h"
#include "Services/MaterialService.h"

namespace
{
class FCreateMaterialInstanceFakeService final : public IMaterialService
{
public:
	FMaterialInstanceCreationParams Received;
	int32 CreateCalls = 0;

	virtual UMaterial* CreateMaterial(const FMaterialCreationParams&, FString&, FString&) override { return nullptr; }
	virtual UMaterialInterface* CreateMaterialInstance(
		const FMaterialInstanceCreationParams& Params,
		FString& OutPath,
		FString&) override
	{
		++CreateCalls;
		Received = Params;
		OutPath = TEXT("/Game/Test/MI_Test.MI_Test");
		return NewObject<UMaterial>();
	}
	virtual UMaterialInterface* FindMaterial(const FString&) override { return nullptr; }
	virtual bool GetMaterialMetadata(const FString&, const TArray<FString>*, TSharedPtr<FJsonObject>&) override { return false; }
	virtual bool SetScalarParameter(const FString&, const FString&, float, FString&) override { return false; }
	virtual bool SetVectorParameter(const FString&, const FString&, const FLinearColor&, FString&) override { return false; }
	virtual bool SetTextureParameter(const FString&, const FString&, const FString&, FString&) override { return false; }
	virtual bool GetScalarParameter(const FString&, const FString&, float&, FString&) override { return false; }
	virtual bool GetVectorParameter(const FString&, const FString&, FLinearColor&, FString&) override { return false; }
	virtual bool GetTextureParameter(const FString&, const FString&, FString&, FString&) override { return false; }
	virtual bool ApplyMaterialToActor(const FString&, const FString&, int32, const FString&, FString&) override { return false; }
	virtual bool DuplicateMaterialInstance(const FString&, const FString&, const FString&, FString&, FString&, FString&) override { return false; }
};

TSharedPtr<FJsonObject> ParseCreateInstanceResponse(const FString& Json)
{
	TSharedPtr<FJsonObject> Result;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	FJsonSerializer::Deserialize(Reader, Result);
	return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCreateMaterialInstanceParametersTest,
	"UnrealMCP.Material.CreateMaterialInstance.InitialParameters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCreateMaterialInstanceParametersTest::RunTest(const FString& Parameters)
{
	FCreateMaterialInstanceFakeService Service;
	FCreateMaterialInstanceCommand Command(Service);

	const TSharedPtr<FJsonObject> Result = ParseCreateInstanceResponse(Command.Execute(TEXT(
		R"({"name":"MI_Test","parent_material":"/Game/Test/M.M","folder_path":"/Game/Test","scalar_params":{"Wind":0.03},"vector_params":{"Direction":[1,0.5,0.25,0]},"texture_params":{"Albedo":"/Game/Test/T.T"}})")));
	TestTrue(TEXT("parameterized create succeeds"), Result && Result->GetBoolField(TEXT("success")));
	TestEqual(TEXT("service is called once"), Service.CreateCalls, 1);
	TestEqual(TEXT("scalar map reaches service"), Service.Received.ScalarParameters.FindRef(TEXT("Wind")), 0.03f);
	TestEqual(TEXT("vector R reaches service"), Service.Received.VectorParameters.FindRef(TEXT("Direction")).R, 1.0f);
	TestEqual(TEXT("vector G reaches service"), Service.Received.VectorParameters.FindRef(TEXT("Direction")).G, 0.5f);
	TestEqual(TEXT("vector B reaches service"), Service.Received.VectorParameters.FindRef(TEXT("Direction")).B, 0.25f);
	TestEqual(TEXT("vector A reaches service"), Service.Received.VectorParameters.FindRef(TEXT("Direction")).A, 0.0f);
	TestEqual(TEXT("texture map reaches service"), Service.Received.TextureParameters.FindRef(TEXT("Albedo")), FString(TEXT("/Game/Test/T.T")));

	const TSharedPtr<FJsonObject> RgbResult = ParseCreateInstanceResponse(Command.Execute(TEXT(
		R"({"name":"MI_RGB","parent_material":"/Game/Test/M.M","vector_params":{"Direction":[0.1,0.2,0.3]}})")));
	TestTrue(TEXT("three-component RGB vector succeeds"), RgbResult && RgbResult->GetBoolField(TEXT("success")));
	TestEqual(TEXT("RGB R is preserved"), Service.Received.VectorParameters.FindRef(TEXT("Direction")).R, 0.1f);
	TestEqual(TEXT("RGB default alpha is one"), Service.Received.VectorParameters.FindRef(TEXT("Direction")).A, 1.0f);

	const TSharedPtr<FJsonObject> Invalid = ParseCreateInstanceResponse(Command.Execute(TEXT(
		R"({"name":"MI_Bad","parent_material":"/Game/Test/M.M","scalar_params":{"Wind":"fast"}})")));
	TestFalse(TEXT("wrong scalar type fails"), Invalid && Invalid->GetBoolField(TEXT("success")));
	TestEqual(TEXT("invalid payload never reaches service"), Service.CreateCalls, 2);

	const TSharedPtr<FJsonObject> InvalidVector = ParseCreateInstanceResponse(Command.Execute(TEXT(
		R"({"name":"MI_Bad","parent_material":"/Game/Test/M.M","vector_params":{"Direction":[1,0]}})")));
	TestFalse(TEXT("short vector fails"), InvalidVector && InvalidVector->GetBoolField(TEXT("success")));
	const TSharedPtr<FJsonObject> InvalidVectorMember = ParseCreateInstanceResponse(Command.Execute(TEXT(
		R"({"name":"MI_Bad","parent_material":"/Game/Test/M.M","vector_params":{"Direction":[1,"bad",0]}})")));
	TestFalse(TEXT("non-numeric vector component fails"), InvalidVectorMember && InvalidVectorMember->GetBoolField(TEXT("success")));
	const TSharedPtr<FJsonObject> InvalidTexture = ParseCreateInstanceResponse(Command.Execute(TEXT(
		R"({"name":"MI_Bad","parent_material":"/Game/Test/M.M","texture_params":{"Albedo":42}})")));
	TestFalse(TEXT("non-string texture path fails"), InvalidTexture && InvalidTexture->GetBoolField(TEXT("success")));
	const TSharedPtr<FJsonObject> OverlongVector = ParseCreateInstanceResponse(Command.Execute(TEXT(
		R"({"name":"MI_Bad","parent_material":"/Game/Test/M.M","vector_params":{"Direction":[1,0,0,1,2]}})")));
	TestFalse(TEXT("overlong create vector fails"), OverlongVector && OverlongVector->GetBoolField(TEXT("success")));
	const TSharedPtr<FJsonObject> WrongScalarMap = ParseCreateInstanceResponse(Command.Execute(TEXT(
		R"({"name":"MI_Bad","parent_material":"/Game/Test/M.M","scalar_params":"not-an-object"})")));
	TestFalse(TEXT("wrong scalar map shape fails"), WrongScalarMap && WrongScalarMap->GetBoolField(TEXT("success")));
	const TSharedPtr<FJsonObject> WrongVectorMap = ParseCreateInstanceResponse(Command.Execute(TEXT(
		R"({"name":"MI_Bad","parent_material":"/Game/Test/M.M","vector_params":[]})")));
	TestFalse(TEXT("wrong vector map shape fails"), WrongVectorMap && WrongVectorMap->GetBoolField(TEXT("success")));
	const TSharedPtr<FJsonObject> WrongTextureMap = ParseCreateInstanceResponse(Command.Execute(TEXT(
		R"({"name":"MI_Bad","parent_material":"/Game/Test/M.M","texture_params":42})")));
	TestFalse(TEXT("wrong texture map shape fails"), WrongTextureMap && WrongTextureMap->GetBoolField(TEXT("success")));
	TestEqual(TEXT("all invalid payloads stay above service seam"), Service.CreateCalls, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterialServiceInitialParametersIntegrationTest,
	"UnrealMCP.Material.CreateMaterialInstance.ServiceAppliesInitialParameters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialServiceInitialParametersIntegrationTest::RunTest(const FString& Parameters)
{
	UMaterial* Parent = NewObject<UMaterial>(GetTransientPackage(), TEXT("M_UnrealMCPInitialParamsParent"));
	UMaterialExpressionScalarParameter* Scalar = NewObject<UMaterialExpressionScalarParameter>(Parent);
	Scalar->ParameterName = TEXT("Wind");
	Scalar->DefaultValue = 1.0f;
	Parent->GetExpressionCollection().AddExpression(Scalar);

	UMaterialExpressionVectorParameter* Vector = NewObject<UMaterialExpressionVectorParameter>(Parent);
	Vector->ParameterName = TEXT("Direction");
	Vector->DefaultValue = FLinearColor::White;
	Parent->GetExpressionCollection().AddExpression(Vector);

	UTexture2D* ParentTexture = NewObject<UTexture2D>(GetTransientPackage(), TEXT("T_UnrealMCPInitialParamsParent"));
	UTexture2D* OverrideTexture = NewObject<UTexture2D>(GetTransientPackage(), TEXT("T_UnrealMCPInitialParamsOverride"));
	UMaterialExpressionTextureSampleParameter2D* TextureParameter =
		NewObject<UMaterialExpressionTextureSampleParameter2D>(Parent);
	TextureParameter->ParameterName = TEXT("Albedo");
	TextureParameter->Texture = ParentTexture;
	Parent->GetExpressionCollection().AddExpression(TextureParameter);
	Parent->PostEditChange();

	FMaterialInstanceCreationParams Params;
	Params.Name = TEXT("MID_UnrealMCPInitialParams");
	Params.ParentMaterialPath = Parent->GetPathName();
	Params.bIsDynamic = true;
	Params.ScalarParameters.Add(TEXT("Wind"), 0.03f);
	Params.VectorParameters.Add(TEXT("Direction"), FLinearColor(1.0f, 0.5f, 0.25f, 0.0f));
	Params.TextureParameters.Add(TEXT("Albedo"), OverrideTexture->GetPathName());

	FString InstancePath;
	FString Error;
	UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(
		FMaterialService::Get().CreateMaterialInstance(Params, InstancePath, Error));
	TestNotNull(*FString::Printf(TEXT("real service creates MID: %s"), *Error), MID);
	if (!MID)
	{
		return false;
	}

	float ScalarValue = 0.0f;
	FLinearColor VectorValue = FLinearColor::Black;
	UTexture* TextureValue = nullptr;
	TestTrue(TEXT("scalar override exists"), MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Wind")), ScalarValue));
	TestEqual(TEXT("real service applies scalar override"), ScalarValue, 0.03f);
	TestTrue(TEXT("vector override exists"), MID->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Direction")), VectorValue));
	TestEqual(TEXT("real service applies vector override"), VectorValue, FLinearColor(1.0f, 0.5f, 0.25f, 0.0f));
	TestTrue(TEXT("texture override exists"), MID->GetTextureParameterValue(FMaterialParameterInfo(TEXT("Albedo")), TextureValue));
	TestEqual(TEXT("real service applies texture override"), TextureValue, static_cast<UTexture*>(OverrideTexture));

	FMaterialInstanceCreationParams StaticParams = Params;
	StaticParams.bIsDynamic = false;
	StaticParams.Name = TEXT("MI_UnrealMCPInitialParams_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	StaticParams.Path = TEXT("/Game/Tests/UnrealMCP");
	Error.Reset();
	UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(
		FMaterialService::Get().CreateMaterialInstance(StaticParams, InstancePath, Error));
	TestNotNull(*FString::Printf(TEXT("real service creates MIC: %s"), *Error), MIC);
	if (MIC)
	{
		ScalarValue = 0.0f;
		VectorValue = FLinearColor::Black;
		TextureValue = nullptr;
		TestTrue(TEXT("MIC scalar override exists"), MIC->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Wind")), ScalarValue));
		TestEqual(TEXT("real service applies MIC scalar override"), ScalarValue, 0.03f);
		TestTrue(TEXT("MIC vector override exists"), MIC->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Direction")), VectorValue));
		TestEqual(TEXT("real service applies MIC vector override"), VectorValue, FLinearColor(1.0f, 0.5f, 0.25f, 0.0f));
		TestTrue(TEXT("MIC texture override exists"), MIC->GetTextureParameterValue(FMaterialParameterInfo(TEXT("Albedo")), TextureValue));
		TestEqual(TEXT("real service applies MIC texture override"), TextureValue, static_cast<UTexture*>(OverrideTexture));

	}
	const FString PackageName = StaticParams.Path / StaticParams.Name;
	const FString PackageFile = FPackageName::LongPackageNameToFilename(
		PackageName,
		FPackageName::GetAssetPackageExtension());
	IFileManager::Get().Delete(*PackageFile, false, true, true);
	if (MIC)
	{
		FAssetRegistryModule::AssetDeleted(MIC);
		MIC->ClearFlags(RF_Public | RF_Standalone);
		MIC->MarkAsGarbage();
	}
	return true;
}

#endif
