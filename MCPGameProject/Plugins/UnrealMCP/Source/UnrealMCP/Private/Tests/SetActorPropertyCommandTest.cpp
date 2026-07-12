#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Commands/Editor/SetActorPropertyCommand.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Services/EditorService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Tests/SetActorPropertyNotificationTestActor.h"

namespace
{
bool ResponseSucceeded(const FString& Json)
{
	TSharedPtr<FJsonObject> Response;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	return FJsonSerializer::Deserialize(Reader, Response)
		&& Response.IsValid()
		&& Response->GetBoolField(TEXT("success"));
}

class FScopedActorPropertyTestWorld final
{
public:
	FScopedActorPropertyTestWorld()
	{
		World = UWorld::CreateWorld(
			EWorldType::EditorPreview,
			false,
			TEXT("UnrealMCPActorPropertyNotificationWorld"),
			GetTransientPackage());
		if (World && GEngine)
		{
			WorldContext = &GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
			WorldContext->SetCurrentWorld(World);
		}
	}

	~FScopedActorPropertyTestWorld()
	{
		if (World && GEngine && WorldContext)
		{
			GEngine->DestroyWorldContext(World);
		}
		if (World)
		{
			World->DestroyWorld(false);
		}
	}

	UWorld* Get() const { return World; }

private:
	UWorld* World = nullptr;
	FWorldContext* WorldContext = nullptr;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSetActorPropertySendsEditorNotificationTest,
	"UnrealMCP.Editor.SetActorProperty.SendsEditorNotification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSetActorPropertySendsEditorNotificationTest::RunTest(const FString& Parameters)
{
	FScopedActorPropertyTestWorld ScopedWorld;
	UWorld* World = ScopedWorld.Get();
	TestNotNull(TEXT("test world exists"), World);
	if (!World)
	{
		return false;
	}

	AUnrealMCPActorPropertyNotificationTestActor* Actor =
		World->SpawnActor<AUnrealMCPActorPropertyNotificationTestActor>();
	TestNotNull(TEXT("notification fixture actor spawns"), Actor);
	if (!Actor)
	{
		return false;
	}

	FEditorService Service;
	FString Error;
	const bool bSet = Service.SetActorProperty(
		Actor,
		TEXT("TestValue"),
		MakeShared<FJsonValueNumber>(42.0),
		Error);

	TestTrue(*FString::Printf(TEXT("property set succeeds: %s"), *Error), bSet);
	TestEqual(TEXT("reflected value changes"), Actor->TestValue, 42);
	TestEqual(TEXT("successful write emits one PreEditChange"), Actor->PreEditChangeCount, 1);
	TestEqual(TEXT("successful write emits one PostEditChangeProperty"), Actor->PostEditChangeCount, 1);
	TestEqual(TEXT("PreEditChange observes the old value"), Actor->ValueObservedBeforeChange, 0);
	TestEqual(TEXT("notifications have two lifecycle entries"), Actor->NotificationOrder.Num(), 2);
	if (Actor->NotificationOrder.Num() == 2)
	{
		TestEqual(TEXT("PreEditChange comes first"), Actor->NotificationOrder[0], FString(TEXT("Pre")));
		TestEqual(TEXT("PostEditChangeProperty comes second"), Actor->NotificationOrder[1], FString(TEXT("Post")));
	}
	TestEqual(TEXT("notification identifies the changed property"), Actor->LastChangedProperty, FName(TEXT("TestValue")));

	Error.Reset();
	const bool bInvalidSet = Service.SetActorProperty(
		Actor,
		TEXT("TestValue"),
		MakeShared<FJsonValueString>(TEXT("not-an-integer")),
		Error);
	TestFalse(TEXT("invalid conversion fails"), bInvalidSet);
	TestEqual(TEXT("failed conversion does not change the value"), Actor->TestValue, 42);
	TestEqual(TEXT("failed numeric conversion emits no PreEditChange"), Actor->PreEditChangeCount, 1);
	TestEqual(TEXT("failed conversion emits no PostEditChangeProperty"), Actor->PostEditChangeCount, 1);

	Error.Reset();
	const bool bInvalidEnum = Service.SetActorProperty(
		Actor,
		TEXT("TestMode"),
		MakeShared<FJsonValueString>(TEXT("NotARealMode")),
		Error);
	TestFalse(TEXT("invalid enum conversion fails"), bInvalidEnum);
	TestEqual(TEXT("failed enum conversion preserves value"), Actor->TestMode, EUnrealMCPActorPropertyTestMode::First);
	TestEqual(TEXT("failed enum conversion emits no PreEditChange"), Actor->PreEditChangeCount, 1);
	TestEqual(TEXT("failed enum conversion emits no PostEditChangeProperty"), Actor->PostEditChangeCount, 1);

	Error.Reset();
	const bool bInvalidString = Service.SetActorProperty(
		Actor,
		TEXT("TestString"),
		MakeShared<FJsonValueNumber>(123.0),
		Error);
	TestFalse(TEXT("numeric payload for string property fails"), bInvalidString);
	TestEqual(TEXT("failed string conversion preserves value"), Actor->TestString, FString(TEXT("original")));
	TestEqual(TEXT("failed string conversion emits no PreEditChange"), Actor->PreEditChangeCount, 1);
	TestEqual(TEXT("failed string conversion emits no PostEditChangeProperty"), Actor->PostEditChangeCount, 1);

	TArray<TSharedPtr<FJsonValue>> InvalidVectorComponents;
	InvalidVectorComponents.Add(MakeShared<FJsonValueNumber>(3.0));
	InvalidVectorComponents.Add(MakeShared<FJsonValueBoolean>(true));
	Error.Reset();
	const bool bInvalidStruct = Service.SetActorProperty(
		Actor,
		TEXT("TestVector"),
		MakeShared<FJsonValueArray>(InvalidVectorComponents),
		Error);
	TestFalse(TEXT("non-numeric struct component fails"), bInvalidStruct);
	TestEqual(TEXT("failed struct conversion preserves value"), Actor->TestVector, FVector2D(1.0, 2.0));
	TestEqual(TEXT("failed struct conversion emits no PreEditChange"), Actor->PreEditChangeCount, 1);
	TestEqual(TEXT("failed struct conversion emits no PostEditChangeProperty"), Actor->PostEditChangeCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSetActorPropertyCommandCallSiteTest,
	"UnrealMCP.Editor.SetActorProperty.CommandCallSite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSetActorPropertyCommandCallSiteTest::RunTest(const FString& Parameters)
{
	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	TestNotNull(TEXT("editor world exists"), EditorWorld);
	if (!EditorWorld)
	{
		return false;
	}

	AUnrealMCPActorPropertyNotificationTestActor* Actor =
		EditorWorld->SpawnActor<AUnrealMCPActorPropertyNotificationTestActor>();
	TestNotNull(TEXT("command fixture actor spawns"), Actor);
	if (!Actor)
	{
		return false;
	}

	FEditorService Service;
	FSetActorPropertyCommand Command(Service);
	const FString SingleResponse = Command.Execute(FString::Printf(
		TEXT(R"({"name":"%s","property_name":"TestValue","property_value":7})"),
		*Actor->GetName()));
	TestTrue(TEXT("single command path succeeds"), ResponseSucceeded(SingleResponse));
	TestEqual(TEXT("single command reaches lifecycle service"), Actor->TestValue, 7);
	TestEqual(TEXT("single command emits PreEditChange"), Actor->PreEditChangeCount, 1);
	TestEqual(TEXT("single command emits PostEditChangeProperty"), Actor->PostEditChangeCount, 1);

	const FString BatchResponse = Command.Execute(FString::Printf(
		TEXT(R"({"name":"%s","properties":[{"name":"TestValue","value":11}]})"),
		*Actor->GetName()));
	TestTrue(TEXT("batch command path succeeds"), ResponseSucceeded(BatchResponse));
	TestEqual(TEXT("batch command reaches lifecycle service"), Actor->TestValue, 11);
	TestEqual(TEXT("batch command emits PreEditChange"), Actor->PreEditChangeCount, 2);
	TestEqual(TEXT("batch command emits PostEditChangeProperty"), Actor->PostEditChangeCount, 2);
	Actor->Destroy();
	return true;
}

#endif
