#if WITH_DEV_AUTOMATION_TESTS

#include "Commands/Material/BatchSetMaterialParamsCommand.h"

#include "Dom/JsonObject.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Services/IMaterialService.h"

namespace
{
class FBatchMaterialFakeService final : public IMaterialService
{
public:
	TArray<FString> ScalarCalls;
	bool bFailWrites = false;

	virtual UMaterial* CreateMaterial(const FMaterialCreationParams&, FString&, FString&) override { return nullptr; }
	virtual UMaterialInterface* CreateMaterialInstance(const FMaterialInstanceCreationParams&, FString&, FString&) override { return nullptr; }
	virtual UMaterialInterface* FindMaterial(const FString&) override { return nullptr; }
	virtual bool GetMaterialMetadata(const FString&, const TArray<FString>*, TSharedPtr<FJsonObject>&) override { return false; }
	virtual bool SetScalarParameter(const FString&, const FString& Name, float, FString& OutError) override
	{
		ScalarCalls.Add(Name);
		if (bFailWrites)
		{
			OutError = TEXT("synthetic write failure");
			return false;
		}
		return true;
	}
	virtual bool SetVectorParameter(const FString&, const FString&, const FLinearColor&, FString&) override { return true; }
	virtual bool SetTextureParameter(const FString&, const FString&, const FString&, FString&) override { return true; }
	virtual bool GetScalarParameter(const FString&, const FString&, float&, FString&) override { return false; }
	virtual bool GetVectorParameter(const FString&, const FString&, FLinearColor&, FString&) override { return false; }
	virtual bool GetTextureParameter(const FString&, const FString&, FString&, FString&) override { return false; }
	virtual bool ApplyMaterialToActor(const FString&, const FString&, int32, const FString&, FString&) override { return false; }
	virtual bool DuplicateMaterialInstance(const FString&, const FString&, const FString&, FString&, FString&, FString&) override { return false; }
};

TSharedPtr<FJsonObject> ParseResponse(const FString& Json)
{
	TSharedPtr<FJsonObject> Result;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	FJsonSerializer::Deserialize(Reader, Result);
	return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatchSetMaterialParamsCommandTest,
	"UnrealMCP.Material.BatchSetMaterialParams.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatchSetMaterialParamsCommandTest::RunTest(const FString& Parameters)
{
	FBatchMaterialFakeService Service;
	FBatchSetMaterialParamsCommand Command(Service);

	const FString ValidResponse = Command.Execute(TEXT(
		R"({"material_instance":"/Game/Test/MI.MI","scalar_params":{"WindStrength":0.03}})"));
	const TSharedPtr<FJsonObject> Valid = ParseResponse(ValidResponse);
	TestTrue(TEXT("Valid response parses"), Valid.IsValid());
	TestTrue(TEXT("Valid object payload succeeds"), Valid && Valid->GetBoolField(TEXT("success")));
	TestEqual(TEXT("Scalar setter is called once"), Service.ScalarCalls.Num(), 1);
	if (!Service.ScalarCalls.IsEmpty())
	{
		TestEqual(TEXT("Scalar setter receives parameter name"), Service.ScalarCalls[0], FString(TEXT("WindStrength")));
	}

	const FString StringResponse = Command.Execute(TEXT(
		R"({"material_instance":"/Game/Test/MI.MI","scalar_params":"{\"WindStrength\":0.03}"})"));
	const TSharedPtr<FJsonObject> StringResult = ParseResponse(StringResponse);
	TestTrue(TEXT("String response parses"), StringResult.IsValid());
	TestFalse(TEXT("JSON-encoded string payload is rejected"), StringResult && StringResult->GetBoolField(TEXT("success")));
	TestTrue(TEXT("Wrong-type diagnostic identifies scalar_params"), StringResult && StringResult->GetStringField(TEXT("error")).Contains(TEXT("scalar_params")));

	Service.bFailWrites = true;
	const FString FailureResponse = Command.Execute(TEXT(
		R"({"material_instance":"/Game/Test/MI.MI","scalar_params":{"WindStrength":0.09}})"));
	const TSharedPtr<FJsonObject> Failure = ParseResponse(FailureResponse);
	TestFalse(TEXT("Setter failure makes the batch fail"), Failure && Failure->GetBoolField(TEXT("success")));
	TestTrue(TEXT("Setter diagnostic is preserved"), Failure && Failure->GetStringField(TEXT("error")).Contains(TEXT("synthetic write failure")));

	return true;
}

#endif
