#include "Services/MaterialExpressionService.h"
#include "MaterialEditingLibrary.h"  // Official UE material editing API
#include "MaterialEditorUtilities.h"
#include "IMaterialEditor.h"
#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraphSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionPanner.h"
// Material Function support
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialFunctionInterface.h"
// Noise expression
#include "Materials/MaterialExpressionNoise.h"
#include "Dom/JsonValue.h"
#include "Engine/Texture.h"

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

bool FMaterialExpressionService::ApplyExpressionProperties(UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Properties, FString& OutError)
{
    if (!Expression || !Properties.IsValid())
    {
        return true; // No properties to apply is not an error
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
            float NewValue = Properties->HasField(TEXT("default_value"))
                ? Properties->GetNumberField(TEXT("default_value"))
                : Properties->GetNumberField(TEXT("DefaultValue"));

            // Set value directly - PostEditChangeProperty is unsafe because expressions
            // added via EditorData don't have their Material member set, and
            // ScalarParameter::PostEditChangeProperty broadcasts a delegate that expects Material to be valid.
            // RecompileMaterial() is called later anyway to handle recompilation.
            ScalarParam->DefaultValue = NewValue;
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
                // Set values directly - PostEditChangeProperty is unsafe (see ScalarParameter comment above)
                VectorParam->DefaultValue.R = (*ColorArray)[0]->AsNumber();
                VectorParam->DefaultValue.G = (*ColorArray)[1]->AsNumber();
                VectorParam->DefaultValue.B = (*ColorArray)[2]->AsNumber();
                if (ColorArray->Num() >= 4)
                {
                    VectorParam->DefaultValue.A = (*ColorArray)[3]->AsNumber();
                }
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
    // Handle Noise expression
    else if (UMaterialExpressionNoise* NoiseExpr = Cast<UMaterialExpressionNoise>(Expression))
    {
        if (Properties->HasField(TEXT("Scale")) || Properties->HasField(TEXT("scale")))
        {
            NoiseExpr->Scale = Properties->HasField(TEXT("Scale"))
                ? Properties->GetNumberField(TEXT("Scale"))
                : Properties->GetNumberField(TEXT("scale"));
        }
        if (Properties->HasField(TEXT("Quality")) || Properties->HasField(TEXT("quality")))
        {
            NoiseExpr->Quality = Properties->HasField(TEXT("Quality"))
                ? (int32)Properties->GetNumberField(TEXT("Quality"))
                : (int32)Properties->GetNumberField(TEXT("quality"));
        }
        if (Properties->HasField(TEXT("Levels")) || Properties->HasField(TEXT("levels")))
        {
            NoiseExpr->Levels = Properties->HasField(TEXT("Levels"))
                ? (int32)Properties->GetNumberField(TEXT("Levels"))
                : (int32)Properties->GetNumberField(TEXT("levels"));
        }
        if (Properties->HasField(TEXT("OutputMin")) || Properties->HasField(TEXT("output_min")))
        {
            NoiseExpr->OutputMin = Properties->HasField(TEXT("OutputMin"))
                ? Properties->GetNumberField(TEXT("OutputMin"))
                : Properties->GetNumberField(TEXT("output_min"));
        }
        if (Properties->HasField(TEXT("OutputMax")) || Properties->HasField(TEXT("output_max")))
        {
            NoiseExpr->OutputMax = Properties->HasField(TEXT("OutputMax"))
                ? Properties->GetNumberField(TEXT("OutputMax"))
                : Properties->GetNumberField(TEXT("output_max"));
        }
        if (Properties->HasField(TEXT("LevelScale")) || Properties->HasField(TEXT("level_scale")))
        {
            NoiseExpr->LevelScale = Properties->HasField(TEXT("LevelScale"))
                ? Properties->GetNumberField(TEXT("LevelScale"))
                : Properties->GetNumberField(TEXT("level_scale"));
        }
        if (Properties->HasField(TEXT("Turbulence")) || Properties->HasField(TEXT("turbulence")))
        {
            NoiseExpr->bTurbulence = Properties->HasField(TEXT("Turbulence"))
                ? Properties->GetBoolField(TEXT("Turbulence"))
                : Properties->GetBoolField(TEXT("turbulence"));
        }
        if (Properties->HasField(TEXT("Tiling")) || Properties->HasField(TEXT("tiling")))
        {
            NoiseExpr->bTiling = Properties->HasField(TEXT("Tiling"))
                ? Properties->GetBoolField(TEXT("Tiling"))
                : Properties->GetBoolField(TEXT("tiling"));
        }
        if (Properties->HasField(TEXT("RepeatSize")) || Properties->HasField(TEXT("repeat_size")))
        {
            NoiseExpr->RepeatSize = Properties->HasField(TEXT("RepeatSize"))
                ? (uint32)Properties->GetNumberField(TEXT("RepeatSize"))
                : (uint32)Properties->GetNumberField(TEXT("repeat_size"));
        }
        // NoiseFunction enum: 0=SimplexTex, 1=GradientTex, 2=GradientTex3D, 3=GradientALU, 4=ValueALU, 5=Voronoi
        if (Properties->HasField(TEXT("NoiseFunction")) || Properties->HasField(TEXT("noise_function")))
        {
            int32 FuncValue = Properties->HasField(TEXT("NoiseFunction"))
                ? (int32)Properties->GetNumberField(TEXT("NoiseFunction"))
                : (int32)Properties->GetNumberField(TEXT("noise_function"));
            NoiseExpr->NoiseFunction = (ENoiseFunction)FuncValue;
        }
    }
    // Handle MaterialFunctionCall - load function by path and set it
    else if (UMaterialExpressionMaterialFunctionCall* FunctionCallExpr = Cast<UMaterialExpressionMaterialFunctionCall>(Expression))
    {
        // Accept multiple property names for the function path
        if (Properties->HasField(TEXT("function")) || Properties->HasField(TEXT("Function")) || Properties->HasField(TEXT("FunctionPath")))
        {
            FString FunctionPath;
            if (Properties->HasField(TEXT("function")))
                FunctionPath = Properties->GetStringField(TEXT("function"));
            else if (Properties->HasField(TEXT("Function")))
                FunctionPath = Properties->GetStringField(TEXT("Function"));
            else
                FunctionPath = Properties->GetStringField(TEXT("FunctionPath"));

            // Load the material function asset
            UMaterialFunctionInterface* MaterialFunction = LoadObject<UMaterialFunctionInterface>(nullptr, *FunctionPath);
            if (MaterialFunction)
            {
                // CRITICAL: SetMaterialFunction calls UpdateFromFunctionResource() which requires
                // the Expression->Material pointer to be set, otherwise it silently returns
                // without populating FunctionInputs/FunctionOutputs (and thus GetOutputs() returns empty)
                if (!FunctionCallExpr->Material)
                {
                    // Get Material from outer - this is set when creating the expression with NewObject<>(Material, Class)
                    UMaterial* OuterMaterial = Cast<UMaterial>(FunctionCallExpr->GetOuter());
                    if (OuterMaterial)
                    {
                        FunctionCallExpr->Material = OuterMaterial;
                        UE_LOG(LogTemp, Log, TEXT("Set MaterialFunctionCall->Material from outer: %s"), *OuterMaterial->GetName());
                    }
                }

                // Use SetMaterialFunction which properly updates inputs/outputs
                FunctionCallExpr->SetMaterialFunction(MaterialFunction);
                UE_LOG(LogTemp, Log, TEXT("Set MaterialFunction to: %s (Outputs: %d)"), *FunctionPath, FunctionCallExpr->GetOutputs().Num());
            }
            else
            {
                OutError = FString::Printf(TEXT("Failed to load MaterialFunction at path: %s"), *FunctionPath);
                return false;
            }
        }
        else
        {
            // MaterialFunctionCall requires a function path - return helpful error with valid property names
            TArray<FString> ProvidedKeys;
            Properties->Values.GetKeys(ProvidedKeys);
            OutError = FString::Printf(TEXT("MaterialFunctionCall requires 'Function' or 'FunctionPath' property to specify the material function path. "
                "Got properties: [%s]. Example: {\"Function\": \"/Engine/Functions/Engine_MaterialFunctions01/Gradient/RadialGradientExponential.RadialGradientExponential\"}"),
                *FString::Join(ProvidedKeys, TEXT(", ")));
            return false;
        }
    }

    return true;
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
                if (!ApplyExpressionProperties(NewExpression, Params.Properties, OutError))
                {
                    // Property validation failed - clean up and return
                    Material->GetEditorOnlyData()->ExpressionCollection.RemoveExpression(NewExpression);
                    NewExpression->MarkAsGarbage();
                    return nullptr;
                }
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
            if (!ApplyExpressionProperties(NewExpression, Params.Properties, OutError))
            {
                // Property validation failed - clean up and return
                NewExpression->MarkAsGarbage();
                return nullptr;
            }
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
