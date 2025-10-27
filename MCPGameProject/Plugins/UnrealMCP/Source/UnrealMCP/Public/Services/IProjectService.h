#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Interface for Project-related operations
 * Handles input mappings, folder creation, struct management, and enhanced input
 */
class UNREALMCP_API IProjectService
{
public:
    virtual ~IProjectService() = default;

    // Input mapping operations
    virtual bool CreateInputMapping(const FString& ActionName, const FString& Key, const TSharedPtr<FJsonObject>& Modifiers, FString& OutError) = 0;
    
    // Folder operations
    virtual bool CreateFolder(const FString& FolderPath, bool& bOutAlreadyExists, FString& OutError) = 0;
    virtual TArray<FString> ListFolderContents(const FString& FolderPath, bool& bOutSuccess, FString& OutError) = 0;
    
    // Struct operations
    virtual bool CreateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutFullPath, FString& OutError) = 0;
    virtual bool UpdateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutError) = 0;
    virtual TArray<TSharedPtr<FJsonObject>> ShowStructVariables(const FString& StructName, const FString& Path, bool& bOutSuccess, FString& OutError) = 0;
    
    // Enhanced Input operations
    virtual bool CreateEnhancedInputAction(const FString& ActionName, const FString& Path, const FString& Description, const FString& ValueType, FString& OutAssetPath, FString& OutError) = 0;
    virtual bool CreateInputMappingContext(const FString& ContextName, const FString& Path, const FString& Description, FString& OutAssetPath, FString& OutError) = 0;
    virtual bool AddMappingToContext(const FString& ContextPath, const FString& ActionPath, const FString& Key, const TSharedPtr<FJsonObject>& Modifiers, FString& OutError) = 0;
    virtual TArray<TSharedPtr<FJsonObject>> ListInputActions(const FString& Path, bool& bOutSuccess, FString& OutError) = 0;
    virtual TArray<TSharedPtr<FJsonObject>> ListInputMappingContexts(const FString& Path, bool& bOutSuccess, FString& OutError) = 0;
    
    // Utility operations
    virtual FString GetProjectDirectory() const = 0;
};
