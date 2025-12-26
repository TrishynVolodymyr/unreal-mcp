#pragma once

#include "CoreMinimal.h"
#include "Services/IProjectService.h"

/**
 * Concrete implementation of Project service
 * Handles all project-related operations including input mappings, folders, structs, and enhanced input
 */
class UNREALMCP_API FProjectService : public IProjectService
{
public:
    FProjectService();
    virtual ~FProjectService() = default;

    // IProjectService interface
    virtual bool CreateInputMapping(const FString& ActionName, const FString& Key, const TSharedPtr<FJsonObject>& Modifiers, FString& OutError) override;
    virtual bool CreateFolder(const FString& FolderPath, bool& bOutAlreadyExists, FString& OutError) override;
    virtual TArray<FString> ListFolderContents(const FString& FolderPath, bool& bOutSuccess, FString& OutError) override;
    virtual bool CreateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutFullPath, FString& OutError) override;
    virtual bool UpdateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutError) override;
    virtual TArray<TSharedPtr<FJsonObject>> ShowStructVariables(const FString& StructName, const FString& Path, bool& bOutSuccess, FString& OutError) override;
    virtual bool CreateEnum(const FString& EnumName, const FString& Path, const FString& Description, const TArray<FString>& Values, const TMap<FString, FString>& ValueDescriptions, FString& OutFullPath, FString& OutError) override;
    virtual bool CreateEnhancedInputAction(const FString& ActionName, const FString& Path, const FString& Description, const FString& ValueType, FString& OutAssetPath, FString& OutError) override;
    virtual bool CreateInputMappingContext(const FString& ContextName, const FString& Path, const FString& Description, FString& OutAssetPath, FString& OutError) override;
    virtual bool AddMappingToContext(const FString& ContextPath, const FString& ActionPath, const FString& Key, const TSharedPtr<FJsonObject>& Modifiers, FString& OutError) override;
    virtual TArray<TSharedPtr<FJsonObject>> ListInputActions(const FString& Path, bool& bOutSuccess, FString& OutError) override;
    virtual TArray<TSharedPtr<FJsonObject>> ListInputMappingContexts(const FString& Path, bool& bOutSuccess, FString& OutError) override;
    virtual FString GetProjectDirectory() const override;
    virtual bool DuplicateAsset(const FString& SourcePath, const FString& DestinationPath, const FString& NewName, FString& OutNewAssetPath, FString& OutError) override;

private:
    // Helper methods for struct operations
    FString GetPropertyTypeString(const FProperty* Property) const;
    bool ResolvePropertyType(const FString& PropertyType, FEdGraphPinType& OutPinType) const;
    bool CreateStructProperty(class UUserDefinedStruct* Struct, const TSharedPtr<FJsonObject>& PropertyObj) const;
    class UScriptStruct* FindCustomStruct(const FString& StructName) const;
};
