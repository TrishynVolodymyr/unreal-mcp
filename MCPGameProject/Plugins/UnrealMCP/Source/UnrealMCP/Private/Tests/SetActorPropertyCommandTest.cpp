#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Dom/JsonValue.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Services/EditorService.h"
#include "Tests/SetActorPropertyNotificationTestActor.h"

namespace
{
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
	TestEqual(TEXT("successful write emits one PostEditChangeProperty"), Actor->PostEditChangeCount, 1);
	TestEqual(TEXT("notification identifies the changed property"), Actor->LastChangedProperty, FName(TEXT("TestValue")));
	return true;
}

#endif
