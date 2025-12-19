#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to get project metadata with selective field querying.
 * Consolidates: list_input_actions, list_input_mapping_contexts, show_struct_variables, list_folder_contents
 *
 * Fields:
 * - input_actions: List Enhanced Input Action assets (requires path)
 * - input_contexts: List Input Mapping Context assets (requires path)
 * - structs: Show struct variables (requires struct_name, optional path)
 * - folder_contents: List folder contents (requires folder_path)
 * - *: Return all available fields (default)
 */
class UNREALMCP_API FGetProjectMetadataCommand : public IUnrealMCPCommand
{
public:
    explicit FGetProjectMetadataCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FGetProjectMetadataCommand() = default;

    // IUnrealMCPCommand interface
    virtual FString GetCommandName() const override;
    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;

    // Field builders
    TSharedPtr<FJsonObject> BuildInputActionsInfo(const FString& Path) const;
    TSharedPtr<FJsonObject> BuildInputContextsInfo(const FString& Path) const;
    TSharedPtr<FJsonObject> BuildStructInfo(const FString& StructName, const FString& Path) const;
    TSharedPtr<FJsonObject> BuildFolderContentsInfo(const FString& FolderPath) const;

    // Helper to check if field is requested
    bool IsFieldRequested(const TArray<TSharedPtr<FJsonValue>>* FieldsArray, const FString& FieldName) const;
};
