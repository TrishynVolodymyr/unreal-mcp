#pragma once

#include "CoreMinimal.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Dom/JsonObject.h"

/**
 * Parameters for creating a material expression
 */
struct UNREALMCP_API FMaterialExpressionCreationParams
{
    /** Path to the material to modify */
    FString MaterialPath;

    /** Type of expression to create (e.g., "Constant3Vector", "Add", "TextureSample") */
    FString ExpressionType;

    /** Position in the material editor graph [X, Y] */
    FVector2D Position = FVector2D(0, 0);

    /** Expression-specific properties as JSON */
    TSharedPtr<FJsonObject> Properties;

    /** Default constructor */
    FMaterialExpressionCreationParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (MaterialPath.IsEmpty())
        {
            OutError = TEXT("Material path cannot be empty");
            return false;
        }
        if (ExpressionType.IsEmpty())
        {
            OutError = TEXT("Expression type cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Parameters for connecting two material expressions
 */
struct UNREALMCP_API FMaterialExpressionConnectionParams
{
    /** Path to the material containing the expressions */
    FString MaterialPath;

    /** GUID of the source expression */
    FGuid SourceExpressionId;

    /** Output pin index on the source expression (0-based) */
    int32 SourceOutputIndex = 0;

    /** GUID of the target expression */
    FGuid TargetExpressionId;

    /** Input pin name on the target expression (e.g., "A", "B", "Alpha") */
    FString TargetInputName;

    /** Default constructor */
    FMaterialExpressionConnectionParams() = default;

    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (MaterialPath.IsEmpty())
        {
            OutError = TEXT("Material path cannot be empty");
            return false;
        }
        if (!SourceExpressionId.IsValid())
        {
            OutError = TEXT("Source expression ID is invalid");
            return false;
        }
        if (!TargetExpressionId.IsValid())
        {
            OutError = TEXT("Target expression ID is invalid");
            return false;
        }
        if (TargetInputName.IsEmpty())
        {
            OutError = TEXT("Target input name cannot be empty");
            return false;
        }
        return true;
    }
};

/**
 * Service for material expression operations
 * Handles creation, connection, and management of material expression nodes
 */
class UNREALMCP_API FMaterialExpressionService
{
public:
    /** Constructor */
    FMaterialExpressionService();

    /** Destructor */
    virtual ~FMaterialExpressionService() = default;

    /**
     * Get singleton instance
     * @return Reference to the singleton instance
     */
    static FMaterialExpressionService& Get();

    /**
     * Add a new expression to a material
     * @param Params - Expression creation parameters
     * @param OutExpressionInfo - Output JSON with expression details
     * @param OutError - Error message if creation fails
     * @return Created expression or nullptr if failed
     */
    UMaterialExpression* AddExpression(
        const FMaterialExpressionCreationParams& Params,
        TSharedPtr<FJsonObject>& OutExpressionInfo,
        FString& OutError);

    /**
     * Connect two expressions together
     * @param Params - Connection parameters
     * @param OutError - Error message if connection fails
     * @return true if connected successfully
     */
    bool ConnectExpressions(
        const FMaterialExpressionConnectionParams& Params,
        FString& OutError);

    /**
     * Connect multiple expressions in batch (more efficient - only saves once)
     * @param MaterialPath - Path to the material
     * @param Connections - Array of connection parameters
     * @param OutResults - Results for each connection
     * @param OutError - Error message if any connection fails
     * @return true if all connections succeeded
     */
    bool ConnectExpressionsBatch(
        const FString& MaterialPath,
        const TArray<FMaterialExpressionConnectionParams>& Connections,
        TArray<FString>& OutResults,
        FString& OutError);

    /**
     * Connect an expression to a material output property
     * @param MaterialPath - Path to the material
     * @param ExpressionId - GUID of the expression to connect
     * @param OutputIndex - Output pin index on the expression
     * @param MaterialProperty - Material property to connect to (e.g., "BaseColor", "EmissiveColor")
     * @param OutError - Error message if connection fails
     * @return true if connected successfully
     */
    bool ConnectToMaterialOutput(
        const FString& MaterialPath,
        const FGuid& ExpressionId,
        int32 OutputIndex,
        const FString& MaterialProperty,
        FString& OutError);

    /**
     * Get metadata about a material's expression graph
     * @param MaterialPath - Path to the material
     * @param Fields - Optional list of fields to include (nullptr = all)
     * @param OutMetadata - Output JSON with graph metadata
     * @return true if metadata retrieved successfully
     */
    bool GetGraphMetadata(
        const FString& MaterialPath,
        const TArray<FString>* Fields,
        TSharedPtr<FJsonObject>& OutMetadata);

    /**
     * Delete an expression from a material
     * @param MaterialPath - Path to the material
     * @param ExpressionId - GUID of the expression to delete
     * @param OutError - Error message if deletion fails
     * @return true if deleted successfully
     */
    bool DeleteExpression(
        const FString& MaterialPath,
        const FGuid& ExpressionId,
        FString& OutError);

    /**
     * Set a property on an existing expression
     * @param MaterialPath - Path to the material
     * @param ExpressionId - GUID of the expression
     * @param PropertyName - Name of the property to set
     * @param Value - Value to set (as JSON)
     * @param OutError - Error message if setting fails
     * @return true if property was set successfully
     */
    bool SetExpressionProperty(
        const FString& MaterialPath,
        const FGuid& ExpressionId,
        const FString& PropertyName,
        const TSharedPtr<FJsonValue>& Value,
        FString& OutError);

    /**
     * Compile/apply a material and return validation info including orphans
     * @param MaterialPath - Path to the material
     * @param OutResult - Output JSON with compile results including orphan info
     * @param OutError - Error message if compilation fails
     * @return true if compiled successfully
     */
    bool CompileMaterial(
        const FString& MaterialPath,
        TSharedPtr<FJsonObject>& OutResult,
        FString& OutError);

private:
    /** Singleton instance */
    static TUniquePtr<FMaterialExpressionService> Instance;

    /**
     * Find and validate a material by path
     * @param MaterialPath - Path to the material
     * @param OutError - Error message if not found
     * @return Material or nullptr if not found/invalid
     */
    UMaterial* FindAndValidateMaterial(const FString& MaterialPath, FString& OutError);

    /**
     * Ensure the MaterialGraph exists for a material
     * Creates it if it doesn't exist, similar to how Material Editor does
     * @param Material - Material to ensure graph for
     * @return true if graph exists or was created successfully
     */
    bool EnsureMaterialGraph(UMaterial* Material);

    /**
     * Find an expression by its GUID
     * @param Material - Material to search in
     * @param ExpressionId - GUID to find
     * @return Expression or nullptr if not found
     */
    UMaterialExpression* FindExpressionByGuid(UMaterial* Material, const FGuid& ExpressionId);

    /**
     * Create an expression by type name
     * @param Material - Material to create the expression in
     * @param TypeName - Type name (e.g., "Constant3Vector", "Add")
     * @return Created expression or nullptr if type not found
     */
    UMaterialExpression* CreateExpressionByType(UMaterial* Material, const FString& TypeName);

    /**
     * Get expression type class from string name
     * @param TypeName - Type name string
     * @return Expression class or nullptr if not found
     */
    UClass* GetExpressionClassFromTypeName(const FString& TypeName);

    /**
     * Convert material property string to EMaterialProperty enum
     * @param PropertyName - Property name string (e.g., "BaseColor", "EmissiveColor")
     * @return Material property enum value
     */
    EMaterialProperty GetMaterialPropertyFromString(const FString& PropertyName);

    /**
     * Apply type-specific properties to an expression
     * @param Expression - Expression to modify
     * @param Properties - Properties to apply
     */
    void ApplyExpressionProperties(UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Properties);

    /**
     * Build JSON metadata for an expression
     * @param Expression - Expression to describe
     * @return JSON object with expression metadata
     */
    TSharedPtr<FJsonObject> BuildExpressionMetadata(UMaterialExpression* Expression);

    /**
     * Recompile a material after modifications
     * @param Material - Material to recompile
     */
    void RecompileMaterial(UMaterial* Material);

    /**
     * Get input pin info for an expression
     * @param Expression - Expression to query
     * @return Array of input pin info as JSON
     */
    TArray<TSharedPtr<FJsonValue>> GetInputPinInfo(UMaterialExpression* Expression);

    /**
     * Get output pin info for an expression
     * @param Expression - Expression to query
     * @return Array of output pin info as JSON
     */
    TArray<TSharedPtr<FJsonValue>> GetOutputPinInfo(UMaterialExpression* Expression);
};
