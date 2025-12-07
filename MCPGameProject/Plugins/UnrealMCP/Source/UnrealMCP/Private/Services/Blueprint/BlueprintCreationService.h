#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "Services/IBlueprintService.h"

// Forward declarations
class FBlueprintCache;

/**
 * Service for creating blueprints and blueprint interfaces
 * Handles blueprint creation, directory management, and path normalization
 */
class UNREALMCP_API FBlueprintCreationService
{
public:
    /**
     * Create a new blueprint
     * @param Params - Blueprint creation parameters
     * @param Cache - Blueprint cache for caching created blueprints
     * @param CompileFunc - Function to call for blueprint compilation
     * @return Created blueprint or nullptr if creation failed
     */
    UBlueprint* CreateBlueprint(
        const FBlueprintCreationParams& Params,
        FBlueprintCache& Cache,
        TFunction<bool(UBlueprint*, FString&)> CompileFunc);

    /**
     * Create a blueprint interface
     * @param InterfaceName - Name of the interface
     * @param FolderPath - Folder path where interface should be created
     * @param Cache - Blueprint cache for caching created interfaces
     * @return Created interface blueprint or nullptr if creation failed
     */
    UBlueprint* CreateBlueprintInterface(
        const FString& InterfaceName,
        const FString& FolderPath,
        FBlueprintCache& Cache);

private:
    /**
     * Resolve parent class from string representation
     * @param ParentClassName - String name of the parent class
     * @return UClass pointer or nullptr if not found
     */
    UClass* ResolveParentClass(const FString& ParentClassName) const;

    /**
     * Create directory structure for blueprint path
     * @param FolderPath - Folder path to create
     * @param OutError - Error message if creation fails
     * @return true if directories were created successfully
     */
    bool CreateDirectoryStructure(const FString& FolderPath, FString& OutError) const;

    /**
     * Normalize and validate blueprint path
     * @param InputPath - Input path to normalize
     * @param OutNormalizedPath - Normalized output path
     * @param OutError - Error message if validation fails
     * @return true if path is valid
     */
    bool NormalizeBlueprintPath(const FString& InputPath, FString& OutNormalizedPath, FString& OutError) const;
};
