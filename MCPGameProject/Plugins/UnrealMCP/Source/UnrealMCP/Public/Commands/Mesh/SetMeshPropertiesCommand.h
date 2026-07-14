#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class UStaticMesh;

class UNREALMCP_API FSetMeshPropertiesCommand : public IUnrealMCPCommand
{
public:
	FSetMeshPropertiesCommand() = default;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

#if WITH_DEV_AUTOMATION_TESTS
	static bool ApplyCollisionSettingForTest(UStaticMesh& Mesh, bool bHasCollision);
	static void SetSaveResultOverrideForTest(TOptional<bool> ResultOverride);
	static void SetCollisionResultOverrideForTest(TOptional<bool> ResultOverride);
	static void SetAddCollisionResultOverrideForTest(TOptional<int32> ResultOverride);
	static void ResetExecutionCountersForTest();
	static int32 GetPostEditChangeCallCountForTest();
	static int32 GetSaveAttemptCountForTest();
#endif

private:
	static bool ApplyCollisionSetting(UStaticMesh& Mesh, bool bHasCollision, bool& bOutMeshChanged);

#if WITH_DEV_AUTOMATION_TESTS
	static TOptional<bool> SaveResultOverrideForTest;
	static TOptional<bool> CollisionResultOverrideForTest;
	static TOptional<int32> AddCollisionResultOverrideForTest;
	static int32 PostEditChangeCallCountForTest;
	static int32 SaveAttemptCountForTest;
#endif
};
