#include "Services/MaterialExpressionService.h"
#include "MaterialEditingLibrary.h"  // Official UE material editing API
#include "MaterialEditorUtilities.h"
#include "IMaterialEditor.h"
#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraphNode_Root.h"  // For UMaterialGraphNode_Root
#include "MaterialGraph/MaterialGraphSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "MaterialShared.h"  // For FMaterialUpdateContext
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionFrac.h"
// Particle/VFX expressions
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionSphereMask.h"
#include "Materials/MaterialExpressionParticleRandom.h"
#include "Materials/MaterialExpressionParticleSubUV.h"
// Noise and math expressions
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionLength.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionDistance.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionSquareRoot.h"
// Material Function support
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Dom/JsonValue.h"
#include "Engine/Texture.h"
#include "Toolkits/ToolkitManager.h"  // For FToolkitManager - correct API for finding Material Editor

// Singleton instance
TUniquePtr<FMaterialExpressionService> FMaterialExpressionService::Instance;

FMaterialExpressionService::FMaterialExpressionService()
{
    UE_LOG(LogTemp, Log, TEXT("FMaterialExpressionService initialized"));
}

FMaterialExpressionService& FMaterialExpressionService::Get()
{
    if (!Instance.IsValid())
    {
        Instance = MakeUnique<FMaterialExpressionService>();
    }
    return *Instance;
}

UClass* FMaterialExpressionService::GetExpressionClassFromTypeName(const FString& TypeName)
{
    // Static map of type names to expression classes
    static TMap<FString, UClass*> ExpressionTypeMap;

    // Initialize on first call
    if (ExpressionTypeMap.Num() == 0)
    {
        // Constants
        ExpressionTypeMap.Add(TEXT("Constant"), UMaterialExpressionConstant::StaticClass());
        ExpressionTypeMap.Add(TEXT("Constant2Vector"), UMaterialExpressionConstant2Vector::StaticClass());
        ExpressionTypeMap.Add(TEXT("Constant3Vector"), UMaterialExpressionConstant3Vector::StaticClass());
        ExpressionTypeMap.Add(TEXT("Constant4Vector"), UMaterialExpressionConstant4Vector::StaticClass());

        // Math operations
        ExpressionTypeMap.Add(TEXT("Add"), UMaterialExpressionAdd::StaticClass());
        ExpressionTypeMap.Add(TEXT("Multiply"), UMaterialExpressionMultiply::StaticClass());
        ExpressionTypeMap.Add(TEXT("Divide"), UMaterialExpressionDivide::StaticClass());
        ExpressionTypeMap.Add(TEXT("Subtract"), UMaterialExpressionSubtract::StaticClass());
        ExpressionTypeMap.Add(TEXT("Power"), UMaterialExpressionPower::StaticClass());
        ExpressionTypeMap.Add(TEXT("Abs"), UMaterialExpressionAbs::StaticClass());
        ExpressionTypeMap.Add(TEXT("Clamp"), UMaterialExpressionClamp::StaticClass());
        ExpressionTypeMap.Add(TEXT("Lerp"), UMaterialExpressionLinearInterpolate::StaticClass());
        ExpressionTypeMap.Add(TEXT("OneMinus"), UMaterialExpressionOneMinus::StaticClass());
        ExpressionTypeMap.Add(TEXT("Sine"), UMaterialExpressionSine::StaticClass());
        ExpressionTypeMap.Add(TEXT("Frac"), UMaterialExpressionFrac::StaticClass());

        // Textures
        ExpressionTypeMap.Add(TEXT("TextureSample"), UMaterialExpressionTextureSample::StaticClass());
        ExpressionTypeMap.Add(TEXT("TextureSampleParameter2D"), UMaterialExpressionTextureSampleParameter2D::StaticClass());

        // Parameters
        ExpressionTypeMap.Add(TEXT("ScalarParameter"), UMaterialExpressionScalarParameter::StaticClass());
        ExpressionTypeMap.Add(TEXT("VectorParameter"), UMaterialExpressionVectorParameter::StaticClass());
        ExpressionTypeMap.Add(TEXT("TextureParameter"), UMaterialExpressionTextureObjectParameter::StaticClass());

        // Utilities
        ExpressionTypeMap.Add(TEXT("AppendVector"), UMaterialExpressionAppendVector::StaticClass());
        ExpressionTypeMap.Add(TEXT("ComponentMask"), UMaterialExpressionComponentMask::StaticClass());
        ExpressionTypeMap.Add(TEXT("Time"), UMaterialExpressionTime::StaticClass());
        ExpressionTypeMap.Add(TEXT("Panner"), UMaterialExpressionPanner::StaticClass());
        ExpressionTypeMap.Add(TEXT("TexCoord"), UMaterialExpressionTextureCoordinate::StaticClass());
        ExpressionTypeMap.Add(TEXT("TextureCoordinate"), UMaterialExpressionTextureCoordinate::StaticClass());

        // Math operations (additional)
        ExpressionTypeMap.Add(TEXT("Dot"), UMaterialExpressionDotProduct::StaticClass());
        ExpressionTypeMap.Add(TEXT("DotProduct"), UMaterialExpressionDotProduct::StaticClass());
        ExpressionTypeMap.Add(TEXT("Distance"), UMaterialExpressionDistance::StaticClass());
        ExpressionTypeMap.Add(TEXT("Normalize"), UMaterialExpressionNormalize::StaticClass());
        ExpressionTypeMap.Add(TEXT("Saturate"), UMaterialExpressionSaturate::StaticClass());
        ExpressionTypeMap.Add(TEXT("Sqrt"), UMaterialExpressionSquareRoot::StaticClass());
        ExpressionTypeMap.Add(TEXT("SquareRoot"), UMaterialExpressionSquareRoot::StaticClass());

        // Particle/VFX expressions
        ExpressionTypeMap.Add(TEXT("ParticleColor"), UMaterialExpressionParticleColor::StaticClass());
        ExpressionTypeMap.Add(TEXT("VertexColor"), UMaterialExpressionVertexColor::StaticClass());
        ExpressionTypeMap.Add(TEXT("SphereMask"), UMaterialExpressionSphereMask::StaticClass());
        ExpressionTypeMap.Add(TEXT("ParticleRandom"), UMaterialExpressionParticleRandom::StaticClass());
        ExpressionTypeMap.Add(TEXT("ParticleSubUV"), UMaterialExpressionParticleSubUV::StaticClass());

        // Noise expression - procedural noise generation
        ExpressionTypeMap.Add(TEXT("Noise"), UMaterialExpressionNoise::StaticClass());

        // Length expression - vector magnitude
        ExpressionTypeMap.Add(TEXT("Length"), UMaterialExpressionLength::StaticClass());

        // Material Function Call - allows using any Material Function by path
        ExpressionTypeMap.Add(TEXT("MaterialFunctionCall"), UMaterialExpressionMaterialFunctionCall::StaticClass());
        ExpressionTypeMap.Add(TEXT("FunctionCall"), UMaterialExpressionMaterialFunctionCall::StaticClass());
    }

    return ExpressionTypeMap.FindRef(TypeName);
}

EMaterialProperty FMaterialExpressionService::GetMaterialPropertyFromString(const FString& PropertyName)
{
    if (PropertyName.Equals(TEXT("BaseColor"), ESearchCase::IgnoreCase))
        return MP_BaseColor;
    if (PropertyName.Equals(TEXT("Metallic"), ESearchCase::IgnoreCase))
        return MP_Metallic;
    if (PropertyName.Equals(TEXT("Specular"), ESearchCase::IgnoreCase))
        return MP_Specular;
    if (PropertyName.Equals(TEXT("Roughness"), ESearchCase::IgnoreCase))
        return MP_Roughness;
    if (PropertyName.Equals(TEXT("Normal"), ESearchCase::IgnoreCase))
        return MP_Normal;
    if (PropertyName.Equals(TEXT("EmissiveColor"), ESearchCase::IgnoreCase))
        return MP_EmissiveColor;
    if (PropertyName.Equals(TEXT("Opacity"), ESearchCase::IgnoreCase))
        return MP_Opacity;
    if (PropertyName.Equals(TEXT("OpacityMask"), ESearchCase::IgnoreCase))
        return MP_OpacityMask;
    if (PropertyName.Equals(TEXT("WorldPositionOffset"), ESearchCase::IgnoreCase))
        return MP_WorldPositionOffset;
    if (PropertyName.Equals(TEXT("AmbientOcclusion"), ESearchCase::IgnoreCase))
        return MP_AmbientOcclusion;
    if (PropertyName.Equals(TEXT("Refraction"), ESearchCase::IgnoreCase))
        return MP_Refraction;
    if (PropertyName.Equals(TEXT("SubsurfaceColor"), ESearchCase::IgnoreCase))
        return MP_SubsurfaceColor;

    // Default to emissive for unrecognized properties
    return MP_EmissiveColor;
}

UMaterial* FMaterialExpressionService::FindAndValidateMaterial(const FString& MaterialPath, FString& OutError)
{
    if (MaterialPath.IsEmpty())
    {
        OutError = TEXT("Material path cannot be empty");
        return nullptr;
    }

    // IMPORTANT: First try FindObject to get already-loaded in-memory objects
    // This is critical because LoadObject would load a fresh copy from disk,
    // discarding any in-memory modifications (like connections we just made)
    UMaterialInterface* MaterialInterface = FindObject<UMaterialInterface>(nullptr, *MaterialPath);

    // If not found in memory, fall back to LoadObject
    if (!MaterialInterface)
    {
        MaterialInterface = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    }

    if (!MaterialInterface)
    {
        OutError = FString::Printf(TEXT("Material not found: %s"), *MaterialPath);
        return nullptr;
    }

    // Must be a base material, not an instance
    UMaterial* Material = Cast<UMaterial>(MaterialInterface);
    if (!Material)
    {
        OutError = TEXT("Cannot modify expressions on Material Instances. Use a base Material.");
        return nullptr;
    }

    return Material;
}

UMaterial* FMaterialExpressionService::FindWorkingMaterial(const FString& MaterialPath, FString& OutError, TSharedPtr<IMaterialEditor>* OutMaterialEditor)
{
    // First, find the original material asset
    UMaterial* OriginalMaterial = FindAndValidateMaterial(MaterialPath, OutError);
    if (!OriginalMaterial)
    {
        return nullptr;
    }

    // Check if Material Editor is open for this material
    if (GEditor)
    {
        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        if (AssetEditorSubsystem)
        {
            IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(OriginalMaterial, /*bFocusIfOpen*/ false);
            if (EditorInstance)
            {
                // Material Editor is open - get the editor's working copy
                TSharedPtr<IMaterialEditor> MaterialEditor = StaticCastSharedPtr<IMaterialEditor>(
                    TSharedPtr<IAssetEditorInstance>(EditorInstance, [](IAssetEditorInstance*){}));

                if (MaterialEditor.IsValid())
                {
                    // Return the editor pointer if requested
                    if (OutMaterialEditor)
                    {
                        *OutMaterialEditor = MaterialEditor;
                    }

                    // GetMaterialInterface returns the editor's transient working copy
                    UMaterialInterface* EditorMaterial = MaterialEditor->GetMaterialInterface();
                    UMaterial* WorkingMaterial = Cast<UMaterial>(EditorMaterial);

                    if (WorkingMaterial)
                    {
                        UE_LOG(LogTemp, Log, TEXT("FindWorkingMaterial: Using Material Editor's transient copy for %s"), *MaterialPath);
                        return WorkingMaterial;
                    }
                }
            }
        }
    }

    // Material Editor not open - return the original asset
    UE_LOG(LogTemp, Log, TEXT("FindWorkingMaterial: Using original asset for %s"), *MaterialPath);
    return OriginalMaterial;
}

bool FMaterialExpressionService::EnsureMaterialGraph(UMaterial* Material)
{
    if (!Material)
    {
        return false;
    }

    // Create the MaterialGraph if it doesn't exist
    // This is the same pattern the Material Editor uses
    if (!Material->MaterialGraph)
    {
        Material->MaterialGraph = CastChecked<UMaterialGraph>(
            FBlueprintEditorUtils::CreateNewGraph(
                Material,
                NAME_None,
                UMaterialGraph::StaticClass(),
                UMaterialGraphSchema::StaticClass()
            )
        );
        Material->MaterialGraph->Material = Material;
        Material->MaterialGraph->RebuildGraph();

        UE_LOG(LogTemp, Log, TEXT("Created MaterialGraph for material %s"), *Material->GetName());
    }

    return Material->MaterialGraph != nullptr;
}

UMaterialExpression* FMaterialExpressionService::FindExpressionByGuid(UMaterial* Material, const FGuid& ExpressionId)
{
    if (!Material || !ExpressionId.IsValid())
    {
        return nullptr;
    }

    // Get editor-only data which contains the expression collection
    UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
    if (!EditorData)
    {
        return nullptr;
    }

    // Search through all expressions
    for (UMaterialExpression* Expression : EditorData->ExpressionCollection.Expressions)
    {
        if (Expression && Expression->MaterialExpressionGuid == ExpressionId)
        {
            return Expression;
        }
    }

    return nullptr;
}

void FMaterialExpressionService::RecompileMaterial(UMaterial* Material)
{
    if (!Material)
    {
        return;
    }

    // Notify the material that it's about to change
    Material->PreEditChange(nullptr);

    // Notify the material that it has changed
    Material->PostEditChange();

    // Mark the package as dirty
    Material->MarkPackageDirty();

    // Rebuild the material graph to update the visual representation
    if (Material->MaterialGraph)
    {
        // RebuildGraph creates graph nodes from expressions
        Material->MaterialGraph->RebuildGraph();

        // LinkGraphNodesFromMaterial syncs the visual graph links (wires)
        // from the material expression connections - CRITICAL for connection updates
        Material->MaterialGraph->LinkGraphNodesFromMaterial();

        // NotifyGraphChanged triggers the Slate SGraphEditor widget to refresh
        // This is the key step that makes changes appear in the Material Editor UI
        Material->MaterialGraph->NotifyGraphChanged();
    }

    // Notify any open Material Editor to refresh its view
    // NOTE: Must use UAssetEditorSubsystem->FindEditorForAsset, NOT FMaterialEditorUtilities::GetIMaterialEditorForObject
    // because GetIMaterialEditorForObject expects an object inside the material (like a graph), not the material itself
    TSharedPtr<IMaterialEditor> MaterialEditor;
    if (GEditor)
    {
        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        if (AssetEditorSubsystem)
        {
            IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Material, /*bFocusIfOpen*/ false);
            if (EditorInstance)
            {
                MaterialEditor = StaticCastSharedPtr<IMaterialEditor>(TSharedPtr<IAssetEditorInstance>(EditorInstance, [](IAssetEditorInstance*){}));
            }
        }
    }
    if (MaterialEditor.IsValid())
    {
        // IMPORTANT: Do NOT call UpdateMaterialAfterGraphChange() here!
        // That method calls LinkMaterialExpressionsFromGraph() which syncs Graph → Expressions,
        // potentially overwriting the expression connections we just made.
        // Instead, we've already synced Expressions → Graph via RebuildGraph + LinkGraphNodesFromMaterial,
        // so we only need to refresh the UI and mark dirty.

        // Mark the material as dirty in the editor so it prompts to save
        MaterialEditor->MarkMaterialDirty();

        // Refresh expression previews to show updated connections
        MaterialEditor->ForceRefreshExpressionPreviews();
    }

    UE_LOG(LogTemp, Log, TEXT("Material recompiled and editor notified: %s"), *Material->GetName());
}
