#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IBlueprintService;

/**
 * Change the parent class of an existing Blueprint.
 *
 * Params (JSON):
 *   blueprint_name    (string, required) - simple name or full content path of the Blueprint
 *   new_parent_class  (string, required) - target parent class. Accepts:
 *                                          - Native class short name: "Character", "Actor"
 *                                          - Native class path: "/Script/Engine.Character"
 *                                          - Project module short name: "PawnBase"
 *                                          - Project module path: "/Script/<ProjectName>.PawnBase"
 *                                          - Blueprint asset path: "/Game/Blueprints/BP_Base"
 *
 * Reparents `Blueprint->ParentClass` and recompiles the Blueprint. Marks the package dirty.
 * Mirrors the pattern used by `set_widget_parent_class` but for regular UBlueprint.
 */
class UNREALMCP_API FSetBlueprintParentClassCommand : public IUnrealMCPCommand
{
public:
    FSetBlueprintParentClassCommand(IBlueprintService& InBlueprintService);
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBlueprintService& BlueprintService;
};
