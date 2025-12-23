#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IProjectService.h"

/**
 * Command to retrieve pin names (field names) for a user-defined struct.
 * Returns both GUID-based internal names and friendly display names.
 */
class UNREALMCP_API FGetStructPinNamesCommand : public IUnrealMCPCommand
{
public:
    FGetStructPinNamesCommand(TSharedPtr<IProjectService> InProjectService);
    virtual ~FGetStructPinNamesCommand() = default;

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    TSharedPtr<IProjectService> ProjectService;

private:
    /**
     * Find a struct by name or path
     * @param StructName - Struct name or asset path
     * @return Found struct or nullptr
     */
    UScriptStruct* FindStruct(const FString& StructName) const;

    /**
     * Check if a field name contains a GUID suffix
     * @param FieldName - Field name to check
     * @return True if field name has GUID pattern
     */
    bool IsGuidField(const FString& FieldName) const;

    /**
     * Extract friendly name from GUID-based field name
     * @param GuidFieldName - Full field name with GUID suffix
     * @return Friendly name without GUID suffix
     */
    FString ExtractFriendlyName(const FString& GuidFieldName) const;

    /**
     * Create success response with field information
     */
    FString CreateSuccessResponse(const FString& StructName, const FString& StructPath, const TArray<TSharedPtr<FJsonValue>>& Fields) const;

    /**
     * Create error response
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
