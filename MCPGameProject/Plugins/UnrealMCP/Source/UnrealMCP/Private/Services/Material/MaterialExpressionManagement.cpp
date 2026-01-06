#include "Services/MaterialExpressionService.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "MaterialShared.h"  // For FMaterialUpdateContext
#include "Dom/JsonValue.h"

bool FMaterialExpressionService::DeleteExpression(
    const FString& MaterialPath,
    const FGuid& ExpressionId,
    FString& OutError)
{
    // Find the material
    UMaterial* Material = FindAndValidateMaterial(MaterialPath, OutError);
    if (!Material)
    {
        return false;
    }

    // Find the expression
    UMaterialExpression* Expression = FindExpressionByGuid(Material, ExpressionId);
    if (!Expression)
    {
        OutError = FString::Printf(TEXT("Expression not found: %s"), *ExpressionId.ToString());
        return false;
    }

    // Get editor data
    UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
    if (!EditorData)
    {
        OutError = TEXT("Could not access material editor data");
        return false;
    }

    // Close Material Editor if open (we'll reopen after save)
    bool bEditorWasOpen = false;
    if (GEditor)
    {
        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        if (AssetEditorSubsystem)
        {
            IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Material, false);
            if (EditorInstance)
            {
                bEditorWasOpen = true;
                // Save package BEFORE closing to avoid save dialog prompt
                UPackage* Package = Material->GetOutermost();
                if (Package && Package->IsDirty())
                {
                    FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
                    FSavePackageArgs SaveArgs;
                    SaveArgs.TopLevelFlags = RF_Standalone;
                    UPackage::SavePackage(Package, Material, *PackageFileName, SaveArgs);
                }
                AssetEditorSubsystem->CloseAllEditorsForAsset(Material);
            }
        }
    }

    // Disconnect all connections to/from this expression
    for (UMaterialExpression* OtherExpr : EditorData->ExpressionCollection.Expressions)
    {
        if (!OtherExpr || OtherExpr == Expression) continue;

        for (int32 i = 0; i < OtherExpr->GetInputsView().Num(); ++i)
        {
            FExpressionInput* Input = OtherExpr->GetInput(i);
            if (Input && Input->Expression == Expression)
            {
                Input->Expression = nullptr;
                Input->OutputIndex = 0;
            }
        }
    }

    // Also disconnect from material outputs
    auto DisconnectFromOutput = [&](EMaterialProperty Prop) {
        FExpressionInput* Input = Material->GetExpressionInputForProperty(Prop);
        if (Input && Input->Expression == Expression)
        {
            Input->Expression = nullptr;
            Input->OutputIndex = 0;
        }
    };

    DisconnectFromOutput(MP_BaseColor);
    DisconnectFromOutput(MP_Metallic);
    DisconnectFromOutput(MP_Specular);
    DisconnectFromOutput(MP_Roughness);
    DisconnectFromOutput(MP_Normal);
    DisconnectFromOutput(MP_EmissiveColor);
    DisconnectFromOutput(MP_Opacity);
    DisconnectFromOutput(MP_OpacityMask);
    DisconnectFromOutput(MP_WorldPositionOffset);
    DisconnectFromOutput(MP_AmbientOcclusion);

    // Remove from expression collection
    EditorData->ExpressionCollection.RemoveExpression(Expression);

    // Recompile the material
    RecompileMaterial(Material);

    // Save the package
    UPackage* Package = Material->GetOutermost();
    if (Package)
    {
        FString PackageFileName = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Standalone;
        UPackage::SavePackage(Package, Material, *PackageFileName, SaveArgs);
    }

    // Reopen editor if it was open
    if (bEditorWasOpen && GEditor)
    {
        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        if (AssetEditorSubsystem)
        {
            AssetEditorSubsystem->OpenEditorForAsset(Material);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Deleted expression from material %s"), *MaterialPath);

    return true;
}

bool FMaterialExpressionService::SetExpressionProperty(
    const FString& MaterialPath,
    const FGuid& ExpressionId,
    const FString& PropertyName,
    const TSharedPtr<FJsonValue>& Value,
    FString& OutError)
{
    // Find the material
    UMaterial* Material = FindAndValidateMaterial(MaterialPath, OutError);
    if (!Material)
    {
        return false;
    }

    // Find the expression
    UMaterialExpression* Expression = FindExpressionByGuid(Material, ExpressionId);
    if (!Expression)
    {
        OutError = FString::Printf(TEXT("Expression not found: %s"), *ExpressionId.ToString());
        return false;
    }

    // Mark for modification
    Expression->Modify();
    Material->Modify();

    // Build a properties object with just this property
    TSharedPtr<FJsonObject> Properties = MakeShared<FJsonObject>();
    Properties->SetField(PropertyName, Value);

    // Apply the property
    ApplyExpressionProperties(Expression, Properties);

    // Check if material editor is open and close it (we'll reopen after save)
    bool bEditorWasOpen = false;
    if (GEditor)
    {
        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        if (AssetEditorSubsystem)
        {
            IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Material, false);
            if (EditorInstance)
            {
                bEditorWasOpen = true;
                // Save package BEFORE closing to avoid save dialog prompt
                UPackage* Package = Material->GetOutermost();
                if (Package && Package->IsDirty())
                {
                    FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
                    FSavePackageArgs SaveArgs;
                    SaveArgs.TopLevelFlags = RF_Standalone;
                    UPackage::SavePackage(Package, Material, *PackageFileName, SaveArgs);
                }
                AssetEditorSubsystem->CloseAllEditorsForAsset(Material);
            }
        }
    }

    // Use RecompileMaterial which does full refresh including RebuildGraph()
    RecompileMaterial(Material);

    // Save the package to persist changes to disk
    UPackage* Package = Material->GetOutermost();
    if (Package)
    {
        FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Standalone;
        UPackage::SavePackage(Package, Material, *PackageFileName, SaveArgs);
    }

    // Reopen the editor if it was open
    if (bEditorWasOpen && GEditor)
    {
        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        if (AssetEditorSubsystem)
        {
            AssetEditorSubsystem->OpenEditorForAsset(Material);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Set property %s on expression in material %s"), *PropertyName, *MaterialPath);

    return true;
}

bool FMaterialExpressionService::CompileMaterial(
    const FString& MaterialPath,
    TSharedPtr<FJsonObject>& OutResult,
    FString& OutError)
{
    // Find the material
    UMaterial* Material = FindAndValidateMaterial(MaterialPath, OutError);
    if (!Material)
    {
        return false;
    }

    OutResult = MakeShared<FJsonObject>();

    // Recompile the material (this triggers shader compilation)
    RecompileMaterial(Material);

    // Capture shader compilation errors
    TArray<TSharedPtr<FJsonValue>> CompileErrorsArray;
    bool bHasCompileErrors = false;

    // Get errors from material resources for each quality level
    // Use current shader platform (GetFeatureLevelShaderPlatform converts feature level to shader platform)
    EShaderPlatform ShaderPlatform = GetFeatureLevelShaderPlatform(GMaxRHIFeatureLevel);

    for (int32 QualityLevel = 0; QualityLevel < EMaterialQualityLevel::Num; ++QualityLevel)
    {
        const FMaterialResource* MaterialResource = Material->GetMaterialResource(
            ShaderPlatform,
            (EMaterialQualityLevel::Type)QualityLevel);

        if (MaterialResource)
        {
            const TArray<FString>& Errors = MaterialResource->GetCompileErrors();
            for (const FString& Error : Errors)
            {
                TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
                ErrorObj->SetStringField(TEXT("error"), Error);
                ErrorObj->SetNumberField(TEXT("quality_level"), QualityLevel);
                CompileErrorsArray.Add(MakeShared<FJsonValueObject>(ErrorObj));
                bHasCompileErrors = true;
            }
        }
    }

    // Get editor data for orphan detection
    UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
    if (!EditorData)
    {
        OutResult->SetBoolField(TEXT("success"), !bHasCompileErrors);
        OutResult->SetStringField(TEXT("material_path"), MaterialPath);
        OutResult->SetBoolField(TEXT("has_orphans"), false);
        OutResult->SetNumberField(TEXT("orphan_count"), 0);
        OutResult->SetArrayField(TEXT("compile_errors"), CompileErrorsArray);
        OutResult->SetBoolField(TEXT("has_compile_errors"), bHasCompileErrors);
        OutResult->SetNumberField(TEXT("compile_error_count"), CompileErrorsArray.Num());
        OutResult->SetStringField(TEXT("message"), bHasCompileErrors
            ? FString::Printf(TEXT("Material has %d compile errors"), CompileErrorsArray.Num())
            : TEXT("Material compiled successfully"));
        return true;
    }

    const TArray<TObjectPtr<UMaterialExpression>>& Expressions = EditorData->ExpressionCollection.Expressions;

    // Build set of used expressions (same logic as GetGraphMetadata)
    TSet<UMaterialExpression*> UsedExpressions;

    // Check connections to other expressions' inputs
    for (UMaterialExpression* Expr : Expressions)
    {
        if (!Expr) continue;
        for (int32 i = 0; i < Expr->GetInputsView().Num(); ++i)
        {
            FExpressionInput* Input = Expr->GetInput(i);
            if (Input && Input->Expression)
            {
                UsedExpressions.Add(Input->Expression);
            }
        }
    }

    // Check connections to material outputs
    auto CheckMaterialOutput = [&](EMaterialProperty Prop) {
        FExpressionInput* Input = Material->GetExpressionInputForProperty(Prop);
        if (Input && Input->Expression)
        {
            UsedExpressions.Add(Input->Expression);
        }
    };

    CheckMaterialOutput(MP_BaseColor);
    CheckMaterialOutput(MP_Metallic);
    CheckMaterialOutput(MP_Specular);
    CheckMaterialOutput(MP_Roughness);
    CheckMaterialOutput(MP_Normal);
    CheckMaterialOutput(MP_EmissiveColor);
    CheckMaterialOutput(MP_Opacity);
    CheckMaterialOutput(MP_OpacityMask);
    CheckMaterialOutput(MP_WorldPositionOffset);
    CheckMaterialOutput(MP_AmbientOcclusion);
    CheckMaterialOutput(MP_Refraction);
    CheckMaterialOutput(MP_SubsurfaceColor);

    // Find orphans
    TArray<TSharedPtr<FJsonValue>> OrphanArray;
    for (UMaterialExpression* Expr : Expressions)
    {
        if (!Expr) continue;
        if (!UsedExpressions.Contains(Expr))
        {
            TSharedPtr<FJsonObject> OrphanObj = MakeShared<FJsonObject>();
            OrphanObj->SetStringField(TEXT("expression_id"), Expr->MaterialExpressionGuid.ToString());
            OrphanObj->SetStringField(TEXT("expression_type"), Expr->GetClass()->GetName().Replace(TEXT("MaterialExpression"), TEXT("")));
            OrphanObj->SetStringField(TEXT("description"), Expr->GetDescription());
            OrphanArray.Add(MakeShared<FJsonValueObject>(OrphanObj));
        }
    }

    // Build result
    OutResult->SetBoolField(TEXT("success"), !bHasCompileErrors);
    OutResult->SetStringField(TEXT("material_path"), MaterialPath);
    OutResult->SetArrayField(TEXT("orphans"), OrphanArray);
    OutResult->SetBoolField(TEXT("has_orphans"), OrphanArray.Num() > 0);
    OutResult->SetNumberField(TEXT("orphan_count"), OrphanArray.Num());
    OutResult->SetNumberField(TEXT("expression_count"), Expressions.Num());
    OutResult->SetArrayField(TEXT("compile_errors"), CompileErrorsArray);
    OutResult->SetBoolField(TEXT("has_compile_errors"), bHasCompileErrors);
    OutResult->SetNumberField(TEXT("compile_error_count"), CompileErrorsArray.Num());

    if (bHasCompileErrors)
    {
        OutResult->SetStringField(TEXT("message"), FString::Printf(TEXT("Material has %d compile errors. %d expressions, %d orphans"), CompileErrorsArray.Num(), Expressions.Num(), OrphanArray.Num()));
    }
    else
    {
        OutResult->SetStringField(TEXT("message"), FString::Printf(TEXT("Material compiled successfully. %d expressions, %d orphans"), Expressions.Num(), OrphanArray.Num()));
    }

    UE_LOG(LogTemp, Log, TEXT("Compiled material %s: %d expressions, %d orphans, %d compile errors"), *MaterialPath, Expressions.Num(), OrphanArray.Num(), CompileErrorsArray.Num());

    return true;
}
