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
	struct FScalarCall { FString Path; FString Name; float Value = 0.0f; };
	struct FVectorCall { FString Path; FString Name; FLinearColor Value; };
	struct FTextureCall { FString Path; FString Name; FString Value; };
	TArray<FScalarCall> ScalarCalls;
	TArray<FVectorCall> VectorCalls;
	TArray<FTextureCall> TextureCalls;
	FString FailingName;

	virtual UMaterial* CreateMaterial(const FMaterialCreationParams&, FString&, FString&) override { return nullptr; }
	virtual UMaterialInterface* CreateMaterialInstance(const FMaterialInstanceCreationParams&, FString&, FString&) override { return nullptr; }
	virtual UMaterialInterface* FindMaterial(const FString&) override { return nullptr; }
	virtual bool GetMaterialMetadata(const FString&, const TArray<FString>*, TSharedPtr<FJsonObject>&) override { return false; }
	virtual bool SetScalarParameter(const FString& Path, const FString& Name, float Value, FString& OutError) override
	{
		if (Name == FailingName)
		{
			OutError = TEXT("synthetic write failure");
			return false;
		}
		ScalarCalls.Add({Path, Name, Value});
		return true;
	}
	virtual bool SetVectorParameter(const FString& Path, const FString& Name, const FLinearColor& Value, FString& OutError) override
	{
		if (Name == FailingName) { OutError = TEXT("synthetic write failure"); return false; }
		VectorCalls.Add({Path, Name, Value});
		return true;
	}
	virtual bool SetTextureParameter(const FString& Path, const FString& Name, const FString& Value, FString& OutError) override
	{
		if (Name == FailingName) { OutError = TEXT("synthetic write failure"); return false; }
		TextureCalls.Add({Path, Name, Value});
		return true;
	}
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
		R"({"material_instance":"/Game/Test/MI.MI","scalar_params":{"WindStrength":0.03},"vector_params":{"WindDirection":[1,0.5,0.25,0]},"texture_params":{"Albedo":"/Game/Test/T.T"}})"));
	const TSharedPtr<FJsonObject> Valid = ParseResponse(ValidResponse);
	TestTrue(TEXT("Valid response parses"), Valid.IsValid());
	TestTrue(TEXT("Valid object payload succeeds"), Valid && Valid->GetBoolField(TEXT("success")));
	TestEqual(TEXT("Scalar setter is called once"), Service.ScalarCalls.Num(), 1);
	if (!Service.ScalarCalls.IsEmpty())
	{
		TestEqual(TEXT("Scalar setter receives material path"), Service.ScalarCalls[0].Path, FString(TEXT("/Game/Test/MI.MI")));
		TestEqual(TEXT("Scalar setter receives parameter name"), Service.ScalarCalls[0].Name, FString(TEXT("WindStrength")));
		TestEqual(TEXT("Scalar setter receives value"), Service.ScalarCalls[0].Value, 0.03f);
	}
	TestEqual(TEXT("Vector setter is called once"), Service.VectorCalls.Num(), 1);
	if (!Service.VectorCalls.IsEmpty())
	{
		TestEqual(TEXT("Vector setter receives material path"), Service.VectorCalls[0].Path, FString(TEXT("/Game/Test/MI.MI")));
		TestEqual(TEXT("Vector setter receives parameter name"), Service.VectorCalls[0].Name, FString(TEXT("WindDirection")));
		TestEqual(TEXT("Vector R is preserved"), Service.VectorCalls[0].Value.R, 1.0f);
		TestEqual(TEXT("Vector G is preserved"), Service.VectorCalls[0].Value.G, 0.5f);
		TestEqual(TEXT("Vector B is preserved"), Service.VectorCalls[0].Value.B, 0.25f);
		TestEqual(TEXT("Vector alpha is preserved"), Service.VectorCalls[0].Value.A, 0.0f);
	}
	TestEqual(TEXT("Texture setter is called once"), Service.TextureCalls.Num(), 1);
	if (!Service.TextureCalls.IsEmpty())
	{
		TestEqual(TEXT("Texture setter receives material path"), Service.TextureCalls[0].Path, FString(TEXT("/Game/Test/MI.MI")));
		TestEqual(TEXT("Texture setter receives parameter name"), Service.TextureCalls[0].Name, FString(TEXT("Albedo")));
		TestEqual(TEXT("Texture path is preserved"), Service.TextureCalls[0].Value, FString(TEXT("/Game/Test/T.T")));
	}

	const FString StringResponse = Command.Execute(TEXT(
		R"({"material_instance":"/Game/Test/MI.MI","scalar_params":"{\"WindStrength\":0.03}"})"));
	const TSharedPtr<FJsonObject> StringResult = ParseResponse(StringResponse);
	TestTrue(TEXT("String response parses"), StringResult.IsValid());
	TestFalse(TEXT("JSON-encoded string payload is rejected"), StringResult && StringResult->GetBoolField(TEXT("success")));
	TestTrue(TEXT("Wrong-type diagnostic identifies scalar_params"), StringResult && StringResult->GetStringField(TEXT("error")).Contains(TEXT("scalar_params")));

	const int32 CallsBeforeInvalidVector = Service.ScalarCalls.Num() + Service.VectorCalls.Num() + Service.TextureCalls.Num();
	const TSharedPtr<FJsonObject> InvalidVector = ParseResponse(Command.Execute(TEXT(
		R"({"material_instance":"/Game/Test/MI.MI","scalar_params":{"WouldMutate":1},"vector_params":{"Broken":[1,"bad",0]}})")));
	TestFalse(TEXT("invalid vector fails validation"), InvalidVector && InvalidVector->GetBoolField(TEXT("success")));
	TestEqual(
		TEXT("full payload validation happens before any setter"),
		Service.ScalarCalls.Num() + Service.VectorCalls.Num() + Service.TextureCalls.Num(),
		CallsBeforeInvalidVector);

	const TSharedPtr<FJsonObject> TooLongVector = ParseResponse(Command.Execute(TEXT(
		R"({"material_instance":"/Game/Test/MI.MI","scalar_params":{"WouldMutate":1},"vector_params":{"Broken":[1,0,0,1,2]}})")));
	TestFalse(TEXT("vector with more than four components fails"), TooLongVector && TooLongVector->GetBoolField(TEXT("success")));
	TestEqual(TEXT("overlong vector causes no mutation"), Service.ScalarCalls.Num() + Service.VectorCalls.Num() + Service.TextureCalls.Num(), CallsBeforeInvalidVector);

	const TSharedPtr<FJsonObject> WrongScalarMember = ParseResponse(Command.Execute(TEXT(
		R"({"material_instance":"/Game/Test/MI.MI","scalar_params":{"Broken":"fast"},"texture_params":{"WouldMutate":"/Game/Test/T.T"}})")));
	TestFalse(TEXT("wrong scalar member type fails"), WrongScalarMember && WrongScalarMember->GetBoolField(TEXT("success")));
	TestEqual(TEXT("wrong scalar member causes no mutation"), Service.ScalarCalls.Num() + Service.VectorCalls.Num() + Service.TextureCalls.Num(), CallsBeforeInvalidVector);

	const TSharedPtr<FJsonObject> WrongTextureMember = ParseResponse(Command.Execute(TEXT(
		R"({"material_instance":"/Game/Test/MI.MI","scalar_params":{"WouldMutate":1},"texture_params":{"Broken":42}})")));
	TestFalse(TEXT("wrong texture member type fails"), WrongTextureMember && WrongTextureMember->GetBoolField(TEXT("success")));
	TestEqual(TEXT("wrong texture member causes no mutation"), Service.ScalarCalls.Num() + Service.VectorCalls.Num() + Service.TextureCalls.Num(), CallsBeforeInvalidVector);

	Service.FailingName = TEXT("Fails");
	const FString FailureResponse = Command.Execute(TEXT(
		R"({"material_instance":"/Game/Test/MI.MI","scalar_params":{"Applied":0.09},"vector_params":{"Fails":[1,0,0,1]},"texture_params":{"AppliedTexture":"/Game/Test/T.T"}})"));
	const TSharedPtr<FJsonObject> Failure = ParseResponse(FailureResponse);
	TestFalse(TEXT("Setter failure makes the batch fail"), Failure && Failure->GetBoolField(TEXT("success")));
	TestTrue(TEXT("Setter diagnostic is preserved"), Failure && Failure->GetStringField(TEXT("error")).Contains(TEXT("synthetic write failure")));
	TestTrue(TEXT("partial update is explicit"), Failure && Failure->GetBoolField(TEXT("partial_update")));
	const TSharedPtr<FJsonObject>* FailureResults = nullptr;
	TestTrue(TEXT("failure response contains results"), Failure && Failure->TryGetObjectField(TEXT("results"), FailureResults));
	if (FailureResults)
	{
		const TArray<TSharedPtr<FJsonValue>>* Scalars = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Vectors = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Textures = nullptr;
		TestTrue(TEXT("failure results contain scalar names"), (*FailureResults)->TryGetArrayField(TEXT("scalar"), Scalars));
		TestTrue(TEXT("failure results contain vector names"), (*FailureResults)->TryGetArrayField(TEXT("vector"), Vectors));
		TestTrue(TEXT("failure results contain texture names"), (*FailureResults)->TryGetArrayField(TEXT("texture"), Textures));
		TestTrue(TEXT("applied scalar is reported"), Scalars && Scalars->Num() == 1 && (*Scalars)[0]->AsString() == TEXT("Applied"));
		TestTrue(TEXT("failed vector is omitted"), Vectors && Vectors->IsEmpty());
		TestTrue(TEXT("later texture is reported"), Textures && Textures->Num() == 1 && (*Textures)[0]->AsString() == TEXT("AppliedTexture"));
	}
	TestTrue(TEXT("texture setter still runs after vector failure"), Service.TextureCalls.ContainsByPredicate([](const FBatchMaterialFakeService::FTextureCall& Call)
	{
		return Call.Name == TEXT("AppliedTexture");
	}));

	return true;
}

#endif
