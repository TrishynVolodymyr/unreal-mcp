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

UMaterialExpression* FMaterialExpressionService::CreateExpressionByType(UMaterial* Material, const FString& TypeName)
{
    UClass* ExpressionClass = GetExpressionClassFromTypeName(TypeName);
    if (!ExpressionClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Unknown expression type: %s"), *TypeName);
        return nullptr;
    }

    // Create the expression with the material as the outer
    UMaterialExpression* NewExpression = NewObject<UMaterialExpression>(Material, ExpressionClass);
    if (!NewExpression)
    {
        return nullptr;
    }

    // Generate a unique GUID for this expression
    NewExpression->UpdateMaterialExpressionGuid(true, true);

    return NewExpression;
}

void FMaterialExpressionService::ApplyExpressionProperties(UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Properties)
{
    if (!Expression || !Properties.IsValid())
    {
        return;
    }

    // Handle Constant expression
    if (UMaterialExpressionConstant* ConstExpr = Cast<UMaterialExpressionConstant>(Expression))
    {
        if (Properties->HasField(TEXT("value")))
        {
            ConstExpr->R = Properties->GetNumberField(TEXT("value"));
        }
        if (Properties->HasField(TEXT("R")))
        {
            ConstExpr->R = Properties->GetNumberField(TEXT("R"));
        }
    }
    // Handle Constant2Vector
    else if (UMaterialExpressionConstant2Vector* Const2Expr = Cast<UMaterialExpressionConstant2Vector>(Expression))
    {
        if (Properties->HasField(TEXT("R")))
            Const2Expr->R = Properties->GetNumberField(TEXT("R"));
        if (Properties->HasField(TEXT("G")))
            Const2Expr->G = Properties->GetNumberField(TEXT("G"));
    }
    // Handle Constant3Vector (color)
    else if (UMaterialExpressionConstant3Vector* Const3Expr = Cast<UMaterialExpressionConstant3Vector>(Expression))
    {
        if (Properties->HasField(TEXT("constant")))
        {
            const TArray<TSharedPtr<FJsonValue>>* ColorArray;
            if (Properties->TryGetArrayField(TEXT("constant"), ColorArray) && ColorArray->Num() >= 3)
            {
                Const3Expr->Constant.R = (*ColorArray)[0]->AsNumber();
                Const3Expr->Constant.G = (*ColorArray)[1]->AsNumber();
                Const3Expr->Constant.B = (*ColorArray)[2]->AsNumber();
            }
        }
    }
    // Handle Constant4Vector
    else if (UMaterialExpressionConstant4Vector* Const4Expr = Cast<UMaterialExpressionConstant4Vector>(Expression))
    {
        if (Properties->HasField(TEXT("constant")))
        {
            const TArray<TSharedPtr<FJsonValue>>* ColorArray;
            if (Properties->TryGetArrayField(TEXT("constant"), ColorArray) && ColorArray->Num() >= 4)
            {
                Const4Expr->Constant.R = (*ColorArray)[0]->AsNumber();
                Const4Expr->Constant.G = (*ColorArray)[1]->AsNumber();
                Const4Expr->Constant.B = (*ColorArray)[2]->AsNumber();
                Const4Expr->Constant.A = (*ColorArray)[3]->AsNumber();
            }
        }
    }
    // Handle ScalarParameter - support both camelCase and lowercase
    else if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(Expression))
    {
        if (Properties->HasField(TEXT("parameter_name")) || Properties->HasField(TEXT("ParameterName")))
        {
            FString ParamName = Properties->HasField(TEXT("parameter_name"))
                ? Properties->GetStringField(TEXT("parameter_name"))
                : Properties->GetStringField(TEXT("ParameterName"));
            ScalarParam->SetParameterName(FName(*ParamName));
        }
        if (Properties->HasField(TEXT("default_value")) || Properties->HasField(TEXT("DefaultValue")))
        {
            // Use PreEditChange/PostEditChangeProperty to properly notify the property system
            FProperty* DefaultValueProp = ScalarParam->GetClass()->FindPropertyByName(TEXT("DefaultValue"));
            ScalarParam->PreEditChange(DefaultValueProp);

            ScalarParam->DefaultValue = Properties->HasField(TEXT("default_value"))
                ? Properties->GetNumberField(TEXT("default_value"))
                : Properties->GetNumberField(TEXT("DefaultValue"));

            FPropertyChangedEvent PropertyChangedEvent(DefaultValueProp);
            ScalarParam->PostEditChangeProperty(PropertyChangedEvent);
        }
    }
    // Handle VectorParameter - support both camelCase and lowercase
    else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(Expression))
    {
        if (Properties->HasField(TEXT("parameter_name")) || Properties->HasField(TEXT("ParameterName")))
        {
            FString ParamName = Properties->HasField(TEXT("parameter_name"))
                ? Properties->GetStringField(TEXT("parameter_name"))
                : Properties->GetStringField(TEXT("ParameterName"));
            VectorParam->SetParameterName(FName(*ParamName));
        }
        if (Properties->HasField(TEXT("default_value")) || Properties->HasField(TEXT("DefaultValue")))
        {
            const TArray<TSharedPtr<FJsonValue>>* ColorArray;
            FString FieldName = Properties->HasField(TEXT("default_value")) ? TEXT("default_value") : TEXT("DefaultValue");
            if (Properties->TryGetArrayField(FieldName, ColorArray) && ColorArray->Num() >= 3)
            {
                // Use PreEditChange/PostEditChangeProperty to properly notify the property system
                // This ensures the Material Editor details panel updates
                FProperty* DefaultValueProp = VectorParam->GetClass()->FindPropertyByName(TEXT("DefaultValue"));
                VectorParam->PreEditChange(DefaultValueProp);

                VectorParam->DefaultValue.R = (*ColorArray)[0]->AsNumber();
                VectorParam->DefaultValue.G = (*ColorArray)[1]->AsNumber();
                VectorParam->DefaultValue.B = (*ColorArray)[2]->AsNumber();
                if (ColorArray->Num() >= 4)
                {
                    VectorParam->DefaultValue.A = (*ColorArray)[3]->AsNumber();
                }

                FPropertyChangedEvent PropertyChangedEvent(DefaultValueProp);
                VectorParam->PostEditChangeProperty(PropertyChangedEvent);
            }
        }
    }
    // Handle TextureSample
    else if (UMaterialExpressionTextureSample* TextureSampleExpr = Cast<UMaterialExpressionTextureSample>(Expression))
    {
        if (Properties->HasField(TEXT("texture")))
        {
            FString TexturePath = Properties->GetStringField(TEXT("texture"));
            UTexture* Texture = LoadObject<UTexture>(nullptr, *TexturePath);
            if (Texture)
            {
                TextureSampleExpr->Texture = Texture;
            }
        }
        // Handle SamplerType property
        // Values: 0=Color, 1=Grayscale, 2=Alpha, 3=Normal, 4=Masks, 5=DistanceFieldFont, 6=LinearColor, 7=LinearGrayscale
        if (Properties->HasField(TEXT("SamplerType")) || Properties->HasField(TEXT("sampler_type")))
        {
            int32 SamplerTypeValue = Properties->HasField(TEXT("SamplerType"))
                ? (int32)Properties->GetNumberField(TEXT("SamplerType"))
                : (int32)Properties->GetNumberField(TEXT("sampler_type"));

            // Use PreEditChange/PostEditChangeProperty for proper notification
            FProperty* SamplerTypeProp = TextureSampleExpr->GetClass()->FindPropertyByName(TEXT("SamplerType"));
            if (SamplerTypeProp)
            {
                TextureSampleExpr->PreEditChange(SamplerTypeProp);
                TextureSampleExpr->SamplerType = (EMaterialSamplerType)SamplerTypeValue;
                FPropertyChangedEvent PropertyChangedEvent(SamplerTypeProp);
                TextureSampleExpr->PostEditChangeProperty(PropertyChangedEvent);
            }
            else
            {
                // Fallback if property not found
                TextureSampleExpr->SamplerType = (EMaterialSamplerType)SamplerTypeValue;
            }
        }
    }
    // Handle TextureCoordinate
    else if (UMaterialExpressionTextureCoordinate* TexCoordExpr = Cast<UMaterialExpressionTextureCoordinate>(Expression))
    {
        if (Properties->HasField(TEXT("coordinate_index")))
        {
            TexCoordExpr->CoordinateIndex = (int32)Properties->GetNumberField(TEXT("coordinate_index"));
        }
        if (Properties->HasField(TEXT("u_tiling")))
        {
            TexCoordExpr->UTiling = Properties->GetNumberField(TEXT("u_tiling"));
        }
        if (Properties->HasField(TEXT("v_tiling")))
        {
            TexCoordExpr->VTiling = Properties->GetNumberField(TEXT("v_tiling"));
        }
    }
    // Handle Panner - support both camelCase and lowercase
    else if (UMaterialExpressionPanner* PannerExpr = Cast<UMaterialExpressionPanner>(Expression))
    {
        if (Properties->HasField(TEXT("speed_x")) || Properties->HasField(TEXT("SpeedX")))
        {
            PannerExpr->SpeedX = Properties->HasField(TEXT("speed_x"))
                ? Properties->GetNumberField(TEXT("speed_x"))
                : Properties->GetNumberField(TEXT("SpeedX"));
        }
        if (Properties->HasField(TEXT("speed_y")) || Properties->HasField(TEXT("SpeedY")))
        {
            PannerExpr->SpeedY = Properties->HasField(TEXT("speed_y"))
                ? Properties->GetNumberField(TEXT("speed_y"))
                : Properties->GetNumberField(TEXT("SpeedY"));
        }
    }
    // Handle ComponentMask
    else if (UMaterialExpressionComponentMask* MaskExpr = Cast<UMaterialExpressionComponentMask>(Expression))
    {
        if (Properties->HasField(TEXT("R")) || Properties->HasField(TEXT("r")))
        {
            MaskExpr->R = Properties->HasField(TEXT("R"))
                ? Properties->GetBoolField(TEXT("R"))
                : Properties->GetBoolField(TEXT("r"));
        }
        if (Properties->HasField(TEXT("G")) || Properties->HasField(TEXT("g")))
        {
            MaskExpr->G = Properties->HasField(TEXT("G"))
                ? Properties->GetBoolField(TEXT("G"))
                : Properties->GetBoolField(TEXT("g"));
        }
        if (Properties->HasField(TEXT("B")) || Properties->HasField(TEXT("b")))
        {
            MaskExpr->B = Properties->HasField(TEXT("B"))
                ? Properties->GetBoolField(TEXT("B"))
                : Properties->GetBoolField(TEXT("b"));
        }
        if (Properties->HasField(TEXT("A")) || Properties->HasField(TEXT("a")))
        {
            MaskExpr->A = Properties->HasField(TEXT("A"))
                ? Properties->GetBoolField(TEXT("A"))
                : Properties->GetBoolField(TEXT("a"));
        }
    }
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
        // Update preview material and refresh expression previews
        MaterialEditor->UpdateMaterialAfterGraphChange();
        MaterialEditor->ForceRefreshExpressionPreviews();
    }

    UE_LOG(LogTemp, Log, TEXT("Material recompiled and editor notified: %s"), *Material->GetName());
}

TArray<TSharedPtr<FJsonValue>> FMaterialExpressionService::GetInputPinInfo(UMaterialExpression* Expression)
{
    TArray<TSharedPtr<FJsonValue>> InputPins;

    if (!Expression)
    {
        return InputPins;
    }

    // Iterate through inputs using the expression's interface
    for (int32 i = 0; i < Expression->GetInputsView().Num(); ++i)
    {
        FExpressionInput* Input = Expression->GetInput(i);
        if (Input)
        {
            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
            PinObj->SetNumberField(TEXT("index"), i);
            PinObj->SetStringField(TEXT("name"), Expression->GetInputName(i).ToString());
            PinObj->SetBoolField(TEXT("is_connected"), Input->Expression != nullptr);

            if (Input->Expression)
            {
                PinObj->SetStringField(TEXT("connected_expression_id"), Input->Expression->MaterialExpressionGuid.ToString());
                PinObj->SetNumberField(TEXT("connected_output_index"), Input->OutputIndex);
            }

            InputPins.Add(MakeShared<FJsonValueObject>(PinObj));
        }
    }

    return InputPins;
}

TArray<TSharedPtr<FJsonValue>> FMaterialExpressionService::GetOutputPinInfo(UMaterialExpression* Expression)
{
    TArray<TSharedPtr<FJsonValue>> OutputPins;

    if (!Expression)
    {
        return OutputPins;
    }

    // Get outputs from the expression
    const TArray<FExpressionOutput>& Outputs = Expression->GetOutputs();
    for (int32 i = 0; i < Outputs.Num(); ++i)
    {
        TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
        PinObj->SetNumberField(TEXT("index"), i);
        PinObj->SetStringField(TEXT("name"), Outputs[i].OutputName.ToString());

        OutputPins.Add(MakeShared<FJsonValueObject>(PinObj));
    }

    return OutputPins;
}

TSharedPtr<FJsonObject> FMaterialExpressionService::BuildExpressionMetadata(UMaterialExpression* Expression)
{
    TSharedPtr<FJsonObject> Metadata = MakeShared<FJsonObject>();

    if (!Expression)
    {
        return Metadata;
    }

    Metadata->SetStringField(TEXT("expression_id"), Expression->MaterialExpressionGuid.ToString());
    Metadata->SetStringField(TEXT("expression_type"), Expression->GetClass()->GetName().Replace(TEXT("MaterialExpression"), TEXT("")));
    Metadata->SetNumberField(TEXT("position_x"), Expression->MaterialExpressionEditorX);
    Metadata->SetNumberField(TEXT("position_y"), Expression->MaterialExpressionEditorY);
    Metadata->SetStringField(TEXT("description"), Expression->GetDescription());

    // Add input and output pin info
    Metadata->SetArrayField(TEXT("inputs"), GetInputPinInfo(Expression));
    Metadata->SetArrayField(TEXT("outputs"), GetOutputPinInfo(Expression));

    return Metadata;
}

UMaterialExpression* FMaterialExpressionService::AddExpression(
    const FMaterialExpressionCreationParams& Params,
    TSharedPtr<FJsonObject>& OutExpressionInfo,
    FString& OutError)
{
    // Validate parameters
    if (!Params.IsValid(OutError))
    {
        return nullptr;
    }

    // Find the material
    UMaterial* Material = FindAndValidateMaterial(Params.MaterialPath, OutError);
    if (!Material)
    {
        return nullptr;
    }

    // Get expression class
    UClass* ExpressionClass = GetExpressionClassFromTypeName(Params.ExpressionType);
    if (!ExpressionClass)
    {
        OutError = FString::Printf(TEXT("Unknown expression type: %s"), *Params.ExpressionType);
        return nullptr;
    }

    UMaterialExpression* NewExpression = nullptr;
    FVector2D NodePos(Params.Position.X, Params.Position.Y);

    // Try to use IMaterialEditor if the material is open in an editor
    // This provides proper UI refresh including SGraphEditor notification
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
        // Use the editor's CreateNewMaterialExpression which handles UI refresh properly
        NewExpression = MaterialEditor->CreateNewMaterialExpression(
            ExpressionClass,
            NodePos,
            /*bAutoSelect*/ false,
            /*bAutoAssignResource*/ false,
            Material->MaterialGraph
        );

        if (NewExpression)
        {
            // Ensure expression is in the ExpressionCollection (for proper serialization/querying)
            UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
            if (EditorData && !EditorData->ExpressionCollection.Expressions.Contains(NewExpression))
            {
                EditorData->ExpressionCollection.AddExpression(NewExpression);
            }

            // Apply type-specific properties after creation
            if (Params.Properties.IsValid())
            {
                ApplyExpressionProperties(NewExpression, Params.Properties);
            }

            // Mark dirty after property changes
            Material->MarkPackageDirty();

            // IMPORTANT: Refresh the Material Editor UI after creating the expression
            // CreateNewMaterialExpression handles the initial node creation, but we need
            // to explicitly refresh for the node to become visible in the graph view
            if (Material->MaterialGraph)
            {
                Material->MaterialGraph->NotifyGraphChanged();
            }
            MaterialEditor->UpdateMaterialAfterGraphChange();
            MaterialEditor->ForceRefreshExpressionPreviews();
        }
    }
    else
    {
        // Fallback: Material editor not open, create expression manually
        NewExpression = CreateExpressionByType(Material, Params.ExpressionType);
        if (!NewExpression)
        {
            OutError = FString::Printf(TEXT("Failed to create expression type: %s"), *Params.ExpressionType);
            return nullptr;
        }

        // Set position
        NewExpression->MaterialExpressionEditorX = (int32)Params.Position.X;
        NewExpression->MaterialExpressionEditorY = (int32)Params.Position.Y;

        // Apply type-specific properties
        if (Params.Properties.IsValid())
        {
            ApplyExpressionProperties(NewExpression, Params.Properties);
        }

        // Add to material's expression collection
        UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
        if (EditorData)
        {
            EditorData->ExpressionCollection.AddExpression(NewExpression);
        }

        // Add to the material graph to create the visual node
        if (Material->MaterialGraph)
        {
            Material->MaterialGraph->AddExpression(NewExpression, /*bUserInvoked*/ true);
        }

        // Recompile the material
        RecompileMaterial(Material);
    }

    if (!NewExpression)
    {
        OutError = TEXT("Failed to create expression");
        return nullptr;
    }

    // Build output info
    OutExpressionInfo = MakeShared<FJsonObject>();
    OutExpressionInfo->SetBoolField(TEXT("success"), true);
    OutExpressionInfo->SetStringField(TEXT("expression_id"), NewExpression->MaterialExpressionGuid.ToString());
    OutExpressionInfo->SetStringField(TEXT("expression_type"), Params.ExpressionType);

    TArray<TSharedPtr<FJsonValue>> PositionArray;
    PositionArray.Add(MakeShared<FJsonValueNumber>(NewExpression->MaterialExpressionEditorX));
    PositionArray.Add(MakeShared<FJsonValueNumber>(NewExpression->MaterialExpressionEditorY));
    OutExpressionInfo->SetArrayField(TEXT("position"), PositionArray);

    OutExpressionInfo->SetArrayField(TEXT("inputs"), GetInputPinInfo(NewExpression));
    OutExpressionInfo->SetArrayField(TEXT("outputs"), GetOutputPinInfo(NewExpression));
    OutExpressionInfo->SetStringField(TEXT("message"), FString::Printf(TEXT("Expression %s added successfully"), *Params.ExpressionType));

    UE_LOG(LogTemp, Log, TEXT("Added expression %s to material %s (via %s)"),
        *Params.ExpressionType, *Params.MaterialPath,
        MaterialEditor.IsValid() ? TEXT("MaterialEditor") : TEXT("manual"));

    return NewExpression;
}

bool FMaterialExpressionService::ConnectExpressions(
    const FMaterialExpressionConnectionParams& Params,
    FString& OutError)
{
    // Validate parameters
    if (!Params.IsValid(OutError))
    {
        return false;
    }

    // Find the material
    UMaterial* Material = FindAndValidateMaterial(Params.MaterialPath, OutError);
    if (!Material)
    {
        return false;
    }

    // Find source expression
    UMaterialExpression* SourceExpr = FindExpressionByGuid(Material, Params.SourceExpressionId);
    if (!SourceExpr)
    {
        OutError = FString::Printf(TEXT("Source expression not found: %s"), *Params.SourceExpressionId.ToString());
        return false;
    }

    // Find target expression
    UMaterialExpression* TargetExpr = FindExpressionByGuid(Material, Params.TargetExpressionId);
    if (!TargetExpr)
    {
        OutError = FString::Printf(TEXT("Target expression not found: %s"), *Params.TargetExpressionId.ToString());
        return false;
    }

    // Validate output index
    if (Params.SourceOutputIndex < 0 || Params.SourceOutputIndex >= SourceExpr->GetOutputs().Num())
    {
        OutError = FString::Printf(TEXT("Invalid source output index: %d (expression has %d outputs)"),
            Params.SourceOutputIndex, SourceExpr->GetOutputs().Num());
        return false;
    }

    // Find target input by name
    int32 TargetInputIndex = -1;
    int32 NumInputs = TargetExpr->GetInputsView().Num();
    for (int32 i = 0; i < NumInputs; ++i)
    {
        FName InputName = TargetExpr->GetInputName(i);
        if (InputName.ToString().Equals(Params.TargetInputName, ESearchCase::IgnoreCase))
        {
            TargetInputIndex = i;
            break;
        }
    }

    if (TargetInputIndex < 0)
    {
        // Build list of available input names for error message
        TArray<FString> AvailableInputs;
        for (int32 i = 0; i < NumInputs; ++i)
        {
            AvailableInputs.Add(TargetExpr->GetInputName(i).ToString());
        }
        OutError = FString::Printf(TEXT("Input '%s' not found on target expression. Available inputs: %s"),
            *Params.TargetInputName, *FString::Join(AvailableInputs, TEXT(", ")));
        return false;
    }

    // Get the target input
    FExpressionInput* TargetInput = TargetExpr->GetInput(TargetInputIndex);
    if (!TargetInput)
    {
        OutError = FString::Printf(TEXT("Failed to get input at index %d on target expression"), TargetInputIndex);
        return false;
    }

    // Close Material Editor if open (we'll reopen after save)
    // This ensures our changes persist and aren't overwritten by the editor's in-memory state
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

    // Mark objects for modification (Undo/Redo support)
    SourceExpr->Modify();
    TargetExpr->Modify();
    Material->Modify();
    if (Material->MaterialGraph)
    {
        Material->MaterialGraph->Modify();
    }

    // Use UE5's built-in ConnectExpression() method - this correctly sets ALL fields
    // including the Mask fields (MaskR, MaskG, MaskB, MaskA) that direct assignment misses
    SourceExpr->ConnectExpression(TargetInput, Params.SourceOutputIndex);

    UE_LOG(LogTemp, Log, TEXT("Connected %s[%d] -> %s.%s using ConnectExpression()"),
        *SourceExpr->GetName(), Params.SourceOutputIndex,
        *TargetExpr->GetName(), *Params.TargetInputName);

    // Ensure MaterialGraph exists for visual sync
    if (!Material->MaterialGraph)
    {
        Material->MaterialGraph = CastChecked<UMaterialGraph>(
            FBlueprintEditorUtils::CreateNewGraph(Material, NAME_None, UMaterialGraph::StaticClass(), UMaterialGraphSchema::StaticClass()));
        Material->MaterialGraph->Material = Material;
    }

    // Sync the visual graph FROM expressions (expressions are the source of truth)
    Material->MaterialGraph->LinkGraphNodesFromMaterial();
    Material->MaterialGraph->NotifyGraphChanged();

    // Mark package dirty and save
    Material->MarkPackageDirty();

    UPackage* Package = Material->GetOutermost();
    if (Package)
    {
        FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
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

    UE_LOG(LogTemp, Log, TEXT("Connected expressions in material %s: %s -> %s.%s"),
        *Params.MaterialPath, *SourceExpr->GetName(), *TargetExpr->GetName(), *Params.TargetInputName);

    return true;
}

bool FMaterialExpressionService::ConnectExpressionsBatch(
    const FString& MaterialPath,
    const TArray<FMaterialExpressionConnectionParams>& Connections,
    TArray<FString>& OutResults,
    FString& OutError)
{
    if (Connections.Num() == 0)
    {
        OutError = TEXT("No connections provided");
        return false;
    }

    // Find the material once
    UMaterial* Material = FindAndValidateMaterial(MaterialPath, OutError);
    if (!Material)
    {
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

    // Mark material for modification once
    Material->Modify();
    if (Material->MaterialGraph)
    {
        Material->MaterialGraph->Modify();
    }

    // Process each connection
    int32 SuccessCount = 0;
    for (const FMaterialExpressionConnectionParams& Conn : Connections)
    {
        // Validate connection
        FString ValidationError;
        if (!Conn.SourceExpressionId.IsValid() || !Conn.TargetExpressionId.IsValid() || Conn.TargetInputName.IsEmpty())
        {
            OutResults.Add(FString::Printf(TEXT("FAILED: Invalid connection parameters")));
            continue;
        }

        // Find expressions
        UMaterialExpression* SourceExpr = FindExpressionByGuid(Material, Conn.SourceExpressionId);
        if (!SourceExpr)
        {
            OutResults.Add(FString::Printf(TEXT("FAILED: Source expression not found: %s"), *Conn.SourceExpressionId.ToString()));
            continue;
        }

        UMaterialExpression* TargetExpr = FindExpressionByGuid(Material, Conn.TargetExpressionId);
        if (!TargetExpr)
        {
            OutResults.Add(FString::Printf(TEXT("FAILED: Target expression not found: %s"), *Conn.TargetExpressionId.ToString()));
            continue;
        }

        // Validate output index
        if (Conn.SourceOutputIndex < 0 || Conn.SourceOutputIndex >= SourceExpr->GetOutputs().Num())
        {
            OutResults.Add(FString::Printf(TEXT("FAILED: Invalid output index %d"), Conn.SourceOutputIndex));
            continue;
        }

        // Find target input by name
        int32 TargetInputIndex = -1;
        int32 NumInputs = TargetExpr->GetInputsView().Num();
        for (int32 i = 0; i < NumInputs; ++i)
        {
            FName InputName = TargetExpr->GetInputName(i);
            if (InputName.ToString().Equals(Conn.TargetInputName, ESearchCase::IgnoreCase))
            {
                TargetInputIndex = i;
                break;
            }
        }

        if (TargetInputIndex < 0)
        {
            OutResults.Add(FString::Printf(TEXT("FAILED: Input '%s' not found on target"), *Conn.TargetInputName));
            continue;
        }

        FExpressionInput* TargetInput = TargetExpr->GetInput(TargetInputIndex);
        if (!TargetInput)
        {
            OutResults.Add(FString::Printf(TEXT("FAILED: Could not get input at index %d"), TargetInputIndex));
            continue;
        }

        // Make the connection using ConnectExpression
        SourceExpr->Modify();
        TargetExpr->Modify();
        SourceExpr->ConnectExpression(TargetInput, Conn.SourceOutputIndex);

        OutResults.Add(FString::Printf(TEXT("OK: %s[%d] -> %s.%s"),
            *SourceExpr->GetName(), Conn.SourceOutputIndex, *TargetExpr->GetName(), *Conn.TargetInputName));
        SuccessCount++;
    }

    // Sync graph once after all connections
    if (!Material->MaterialGraph)
    {
        Material->MaterialGraph = CastChecked<UMaterialGraph>(
            FBlueprintEditorUtils::CreateNewGraph(Material, NAME_None, UMaterialGraph::StaticClass(), UMaterialGraphSchema::StaticClass()));
        Material->MaterialGraph->Material = Material;
    }
    Material->MaterialGraph->LinkGraphNodesFromMaterial();
    Material->MaterialGraph->NotifyGraphChanged();

    // Save once
    Material->MarkPackageDirty();
    UPackage* Package = Material->GetOutermost();
    if (Package)
    {
        FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
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

    UE_LOG(LogTemp, Log, TEXT("Batch connected %d/%d expressions in material %s"),
        SuccessCount, Connections.Num(), *MaterialPath);

    if (SuccessCount == 0)
    {
        OutError = TEXT("All connections failed");
        return false;
    }

    return true;
}

bool FMaterialExpressionService::ConnectToMaterialOutput(
    const FString& MaterialPath,
    const FGuid& ExpressionId,
    int32 OutputIndex,
    const FString& MaterialProperty,
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

    // Validate output index
    if (OutputIndex < 0 || OutputIndex >= Expression->GetOutputs().Num())
    {
        OutError = FString::Printf(TEXT("Invalid output index: %d"), OutputIndex);
        return false;
    }

    // Get the material property enum
    EMaterialProperty MatProperty = GetMaterialPropertyFromString(MaterialProperty);

    // Get the material's input for this property
    FExpressionInput* MaterialInput = Material->GetExpressionInputForProperty(MatProperty);
    if (!MaterialInput)
    {
        OutError = FString::Printf(TEXT("Material property not found: %s"), *MaterialProperty);
        return false;
    }

    // Close Material Editor if open (we'll reopen after save)
    // This ensures our changes persist and aren't overwritten by the editor's in-memory state
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

    // Mark objects for modification (Undo/Redo support)
    Expression->Modify();
    Material->Modify();
    if (Material->MaterialGraph)
    {
        Material->MaterialGraph->Modify();
    }

    // Connect at material data level using UE5's built-in ConnectExpression()
    Expression->ConnectExpression(MaterialInput, OutputIndex);

    // Ensure MaterialGraph exists for visual sync
    if (!Material->MaterialGraph)
    {
        Material->MaterialGraph = CastChecked<UMaterialGraph>(
            FBlueprintEditorUtils::CreateNewGraph(Material, NAME_None, UMaterialGraph::StaticClass(), UMaterialGraphSchema::StaticClass()));
        Material->MaterialGraph->Material = Material;
    }

    // Sync the visual graph FROM expressions
    Material->MaterialGraph->LinkGraphNodesFromMaterial();
    Material->MaterialGraph->NotifyGraphChanged();

    // Mark package dirty and save
    Material->MarkPackageDirty();

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

    UE_LOG(LogTemp, Log, TEXT("Connected expression %s to %s in material %s"),
        *Expression->GetName(), *MaterialProperty, *MaterialPath);

    return true;
}

bool FMaterialExpressionService::GetGraphMetadata(
    const FString& MaterialPath,
    const TArray<FString>* Fields,
    TSharedPtr<FJsonObject>& OutMetadata)
{
    FString Error;
    UMaterial* Material = FindAndValidateMaterial(MaterialPath, Error);
    if (!Material)
    {
        OutMetadata = MakeShared<FJsonObject>();
        OutMetadata->SetBoolField(TEXT("success"), false);
        OutMetadata->SetStringField(TEXT("error"), Error);
        return false;
    }

    OutMetadata = MakeShared<FJsonObject>();
    OutMetadata->SetBoolField(TEXT("success"), true);
    OutMetadata->SetStringField(TEXT("material_path"), MaterialPath);

    // Determine which fields to include
    bool bIncludeAll = !Fields || Fields->Num() == 0 || Fields->Contains(TEXT("*"));
    bool bIncludeExpressions = bIncludeAll || Fields->Contains(TEXT("expressions"));
    bool bIncludeConnections = bIncludeAll || Fields->Contains(TEXT("connections"));
    bool bIncludeMaterialOutputs = bIncludeAll || Fields->Contains(TEXT("material_outputs"));
    bool bIncludeOrphans = bIncludeAll || Fields->Contains(TEXT("orphans"));
    bool bIncludeFlow = Fields && Fields->Contains(TEXT("flow"));  // Flow is opt-in, not included in "*"

    UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
    if (!EditorData)
    {
        OutMetadata->SetNumberField(TEXT("expression_count"), 0);
        return true;
    }

    const TArray<TObjectPtr<UMaterialExpression>>& Expressions = EditorData->ExpressionCollection.Expressions;
    OutMetadata->SetNumberField(TEXT("expression_count"), Expressions.Num());

    // Build expressions list
    if (bIncludeExpressions)
    {
        TArray<TSharedPtr<FJsonValue>> ExpressionArray;
        for (UMaterialExpression* Expr : Expressions)
        {
            if (Expr)
            {
                ExpressionArray.Add(MakeShared<FJsonValueObject>(BuildExpressionMetadata(Expr)));
            }
        }
        OutMetadata->SetArrayField(TEXT("expressions"), ExpressionArray);
    }

    // Build connections list
    if (bIncludeConnections)
    {
        UE_LOG(LogTemp, Warning, TEXT("METADATA: Material=%p"), Material);

        TArray<TSharedPtr<FJsonValue>> ConnectionArray;
        for (UMaterialExpression* Expr : Expressions)
        {
            if (!Expr) continue;

            int32 NumInputs = Expr->GetInputsView().Num();
            UE_LOG(LogTemp, Warning, TEXT("Checking expr %p %s (%s) - has %d inputs"),
                Expr, *Expr->GetName(), *Expr->MaterialExpressionGuid.ToString(), NumInputs);

            for (int32 i = 0; i < NumInputs; ++i)
            {
                FExpressionInput* Input = Expr->GetInput(i);
                UE_LOG(LogTemp, Warning, TEXT("  Input %d: Input=%p, Expression=%p"),
                    i, Input, Input ? Input->Expression : nullptr);

                if (Input && Input->Expression)
                {
                    TSharedPtr<FJsonObject> ConnectionObj = MakeShared<FJsonObject>();
                    ConnectionObj->SetStringField(TEXT("source_expression_id"), Input->Expression->MaterialExpressionGuid.ToString());
                    ConnectionObj->SetNumberField(TEXT("source_output_index"), Input->OutputIndex);
                    ConnectionObj->SetStringField(TEXT("target_expression_id"), Expr->MaterialExpressionGuid.ToString());
                    ConnectionObj->SetNumberField(TEXT("target_input_index"), i);
                    ConnectionArray.Add(MakeShared<FJsonValueObject>(ConnectionObj));
                }
            }
        }
        OutMetadata->SetArrayField(TEXT("connections"), ConnectionArray);
    }

    // Build material outputs info
    if (bIncludeMaterialOutputs)
    {
        TSharedPtr<FJsonObject> OutputsObj = MakeShared<FJsonObject>();

        // Check each material property
        auto AddOutputIfConnected = [&](EMaterialProperty Prop, const FString& PropName) {
            FExpressionInput* Input = Material->GetExpressionInputForProperty(Prop);
            if (Input && Input->Expression)
            {
                TSharedPtr<FJsonObject> OutputInfo = MakeShared<FJsonObject>();
                OutputInfo->SetStringField(TEXT("expression_id"), Input->Expression->MaterialExpressionGuid.ToString());
                OutputInfo->SetNumberField(TEXT("output_index"), Input->OutputIndex);
                OutputsObj->SetObjectField(PropName, OutputInfo);
            }
        };

        AddOutputIfConnected(MP_BaseColor, TEXT("BaseColor"));
        AddOutputIfConnected(MP_Metallic, TEXT("Metallic"));
        AddOutputIfConnected(MP_Specular, TEXT("Specular"));
        AddOutputIfConnected(MP_Roughness, TEXT("Roughness"));
        AddOutputIfConnected(MP_Normal, TEXT("Normal"));
        AddOutputIfConnected(MP_EmissiveColor, TEXT("EmissiveColor"));
        AddOutputIfConnected(MP_Opacity, TEXT("Opacity"));
        AddOutputIfConnected(MP_OpacityMask, TEXT("OpacityMask"));
        AddOutputIfConnected(MP_WorldPositionOffset, TEXT("WorldPositionOffset"));
        AddOutputIfConnected(MP_AmbientOcclusion, TEXT("AmbientOcclusion"));

        OutMetadata->SetObjectField(TEXT("material_outputs"), OutputsObj);
    }

    // Orphan detection - find expressions whose outputs are not used anywhere
    if (bIncludeOrphans)
    {
        // Build a set of all expressions that have their output connected somewhere
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

        // Find orphans - expressions not in UsedExpressions
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

        OutMetadata->SetArrayField(TEXT("orphans"), OrphanArray);
        OutMetadata->SetBoolField(TEXT("has_orphans"), OrphanArray.Num() > 0);
        OutMetadata->SetNumberField(TEXT("orphan_count"), OrphanArray.Num());
    }

    // Flow visualization - trace paths from source nodes to material outputs
    if (bIncludeFlow)
    {
        TSharedPtr<FJsonObject> FlowObj = MakeShared<FJsonObject>();

        // Helper to trace path from a material output back to source nodes
        auto TraceFlow = [&](EMaterialProperty Prop, const FString& PropName) {
            FExpressionInput* Input = Material->GetExpressionInputForProperty(Prop);
            if (!Input || !Input->Expression) return;

            TArray<TSharedPtr<FJsonValue>> PathArray;
            TSet<UMaterialExpression*> Visited;
            TArray<UMaterialExpression*> Stack;
            Stack.Push(Input->Expression);

            while (Stack.Num() > 0)
            {
                UMaterialExpression* Current = Stack.Pop();
                if (!Current || Visited.Contains(Current)) continue;
                Visited.Add(Current);

                TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
                NodeObj->SetStringField(TEXT("expression_id"), Current->MaterialExpressionGuid.ToString());
                NodeObj->SetStringField(TEXT("expression_type"), Current->GetClass()->GetName().Replace(TEXT("MaterialExpression"), TEXT("")));
                NodeObj->SetStringField(TEXT("description"), Current->GetDescription());

                // Find what this node connects to (downstream)
                TArray<TSharedPtr<FJsonValue>> DownstreamArray;
                for (UMaterialExpression* OtherExpr : Expressions)
                {
                    if (!OtherExpr) continue;
                    for (int32 i = 0; i < OtherExpr->GetInputsView().Num(); ++i)
                    {
                        FExpressionInput* OtherInput = OtherExpr->GetInput(i);
                        if (OtherInput && OtherInput->Expression == Current)
                        {
                            TSharedPtr<FJsonObject> DownObj = MakeShared<FJsonObject>();
                            DownObj->SetStringField(TEXT("target_id"), OtherExpr->MaterialExpressionGuid.ToString());
                            DownObj->SetStringField(TEXT("target_input"), OtherExpr->GetInputName(i).ToString());
                            DownstreamArray.Add(MakeShared<FJsonValueObject>(DownObj));
                        }
                    }
                }
                NodeObj->SetArrayField(TEXT("connects_to"), DownstreamArray);

                PathArray.Add(MakeShared<FJsonValueObject>(NodeObj));

                // Add upstream nodes to stack
                for (int32 i = 0; i < Current->GetInputsView().Num(); ++i)
                {
                    FExpressionInput* UpstreamInput = Current->GetInput(i);
                    if (UpstreamInput && UpstreamInput->Expression)
                    {
                        Stack.Push(UpstreamInput->Expression);
                    }
                }
            }

            if (PathArray.Num() > 0)
            {
                FlowObj->SetArrayField(PropName, PathArray);
            }
        };

        TraceFlow(MP_BaseColor, TEXT("BaseColor"));
        TraceFlow(MP_Metallic, TEXT("Metallic"));
        TraceFlow(MP_Specular, TEXT("Specular"));
        TraceFlow(MP_Roughness, TEXT("Roughness"));
        TraceFlow(MP_Normal, TEXT("Normal"));
        TraceFlow(MP_EmissiveColor, TEXT("EmissiveColor"));
        TraceFlow(MP_Opacity, TEXT("Opacity"));
        TraceFlow(MP_OpacityMask, TEXT("OpacityMask"));
        TraceFlow(MP_WorldPositionOffset, TEXT("WorldPositionOffset"));
        TraceFlow(MP_AmbientOcclusion, TEXT("AmbientOcclusion"));

        OutMetadata->SetObjectField(TEXT("flow"), FlowObj);
    }

    return true;
}

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
