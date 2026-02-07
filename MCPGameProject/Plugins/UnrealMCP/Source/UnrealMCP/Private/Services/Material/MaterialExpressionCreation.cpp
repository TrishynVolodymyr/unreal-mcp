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
// Particle SubUV for flipbook animations
#include "Materials/MaterialExpressionParticleSubUV.h"
// Custom HLSL expression support
#include "Materials/MaterialExpressionCustom.h"
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
        bool bValueChanged = false;
        if (Properties->HasField(TEXT("value")))
        {
            ConstExpr->R = Properties->GetNumberField(TEXT("value"));
            bValueChanged = true;
        }
        if (Properties->HasField(TEXT("R")))
        {
            ConstExpr->R = Properties->GetNumberField(TEXT("R"));
            bValueChanged = true;
        }
        // Trigger PostEditChangeProperty to update UI
        if (bValueChanged)
        {
            FProperty* RProp = ConstExpr->GetClass()->FindPropertyByName(TEXT("R"));
            if (RProp)
            {
                FPropertyChangedEvent PropertyChangedEvent(RProp, EPropertyChangeType::ValueSet);
                ConstExpr->PostEditChangeProperty(PropertyChangedEvent);
            }
        }
    }
    // Handle Constant2Vector
    else if (UMaterialExpressionConstant2Vector* Const2Expr = Cast<UMaterialExpressionConstant2Vector>(Expression))
    {
        bool bValueChanged = false;
        if (Properties->HasField(TEXT("R")))
        {
            Const2Expr->R = Properties->GetNumberField(TEXT("R"));
            bValueChanged = true;
        }
        if (Properties->HasField(TEXT("G")))
        {
            Const2Expr->G = Properties->GetNumberField(TEXT("G"));
            bValueChanged = true;
        }
        // Trigger PostEditChangeProperty to update UI
        if (bValueChanged)
        {
            FProperty* RProp = Const2Expr->GetClass()->FindPropertyByName(TEXT("R"));
            if (RProp)
            {
                FPropertyChangedEvent PropertyChangedEvent(RProp, EPropertyChangeType::ValueSet);
                Const2Expr->PostEditChangeProperty(PropertyChangedEvent);
            }
        }
    }
    // Handle Constant3Vector (color) - support both cases and string/array formats
    else if (UMaterialExpressionConstant3Vector* Const3Expr = Cast<UMaterialExpressionConstant3Vector>(Expression))
    {
        bool bValueChanged = false;
        FString FieldName;

        // Check for both uppercase and lowercase property names
        if (Properties->HasField(TEXT("Constant")))
            FieldName = TEXT("Constant");
        else if (Properties->HasField(TEXT("constant")))
            FieldName = TEXT("constant");

        if (!FieldName.IsEmpty())
        {
            TSharedPtr<FJsonValue> Value = Properties->TryGetField(FieldName);
            if (Value.IsValid())
            {
                // Try as array first: [R, G, B]
                const TArray<TSharedPtr<FJsonValue>>* ColorArray;
                if (Properties->TryGetArrayField(FieldName, ColorArray) && ColorArray->Num() >= 3)
                {
                    Const3Expr->Constant.R = (*ColorArray)[0]->AsNumber();
                    Const3Expr->Constant.G = (*ColorArray)[1]->AsNumber();
                    Const3Expr->Constant.B = (*ColorArray)[2]->AsNumber();
                    bValueChanged = true;
                }
                // Try as comma-separated string: "R,G,B"
                else if (Value->Type == EJson::String)
                {
                    FString ColorString = Value->AsString();
                    TArray<FString> Components;
                    ColorString.ParseIntoArray(Components, TEXT(","), true);
                    if (Components.Num() >= 3)
                    {
                        Const3Expr->Constant.R = FCString::Atof(*Components[0].TrimStartAndEnd());
                        Const3Expr->Constant.G = FCString::Atof(*Components[1].TrimStartAndEnd());
                        Const3Expr->Constant.B = FCString::Atof(*Components[2].TrimStartAndEnd());
                        bValueChanged = true;
                        UE_LOG(LogTemp, Log, TEXT("Parsed Constant3Vector from string: R=%f, G=%f, B=%f"),
                            Const3Expr->Constant.R, Const3Expr->Constant.G, Const3Expr->Constant.B);
                    }
                    else
                    {
                        OutError = FString::Printf(TEXT("Constant3Vector requires 3 comma-separated values (R,G,B), got: %s"), *ColorString);
                        return false;
                    }
                }
                else
                {
                    OutError = TEXT("Constant3Vector value must be an array [R,G,B] or string \"R,G,B\"");
                    return false;
                }
            }
        }

        // Trigger PostEditChangeProperty to update UI
        if (bValueChanged)
        {
            FProperty* ConstantProp = Const3Expr->GetClass()->FindPropertyByName(TEXT("Constant"));
            if (ConstantProp)
            {
                FPropertyChangedEvent PropertyChangedEvent(ConstantProp, EPropertyChangeType::ValueSet);
                Const3Expr->PostEditChangeProperty(PropertyChangedEvent);
            }
        }
    }
    // Handle Constant4Vector - support both cases and string/array formats
    else if (UMaterialExpressionConstant4Vector* Const4Expr = Cast<UMaterialExpressionConstant4Vector>(Expression))
    {
        bool bValueChanged = false;
        FString FieldName;

        // Check for both uppercase and lowercase property names
        if (Properties->HasField(TEXT("Constant")))
            FieldName = TEXT("Constant");
        else if (Properties->HasField(TEXT("constant")))
            FieldName = TEXT("constant");

        if (!FieldName.IsEmpty())
        {
            TSharedPtr<FJsonValue> Value = Properties->TryGetField(FieldName);
            if (Value.IsValid())
            {
                // Try as array first: [R, G, B, A]
                const TArray<TSharedPtr<FJsonValue>>* ColorArray;
                if (Properties->TryGetArrayField(FieldName, ColorArray) && ColorArray->Num() >= 4)
                {
                    Const4Expr->Constant.R = (*ColorArray)[0]->AsNumber();
                    Const4Expr->Constant.G = (*ColorArray)[1]->AsNumber();
                    Const4Expr->Constant.B = (*ColorArray)[2]->AsNumber();
                    Const4Expr->Constant.A = (*ColorArray)[3]->AsNumber();
                    bValueChanged = true;
                }
                // Try as comma-separated string: "R,G,B,A"
                else if (Value->Type == EJson::String)
                {
                    FString ColorString = Value->AsString();
                    TArray<FString> Components;
                    ColorString.ParseIntoArray(Components, TEXT(","), true);
                    if (Components.Num() >= 4)
                    {
                        Const4Expr->Constant.R = FCString::Atof(*Components[0].TrimStartAndEnd());
                        Const4Expr->Constant.G = FCString::Atof(*Components[1].TrimStartAndEnd());
                        Const4Expr->Constant.B = FCString::Atof(*Components[2].TrimStartAndEnd());
                        Const4Expr->Constant.A = FCString::Atof(*Components[3].TrimStartAndEnd());
                        bValueChanged = true;
                        UE_LOG(LogTemp, Log, TEXT("Parsed Constant4Vector from string: R=%f, G=%f, B=%f, A=%f"),
                            Const4Expr->Constant.R, Const4Expr->Constant.G, Const4Expr->Constant.B, Const4Expr->Constant.A);
                    }
                    else if (Components.Num() >= 3)
                    {
                        // Allow 3 components with default A=1.0
                        Const4Expr->Constant.R = FCString::Atof(*Components[0].TrimStartAndEnd());
                        Const4Expr->Constant.G = FCString::Atof(*Components[1].TrimStartAndEnd());
                        Const4Expr->Constant.B = FCString::Atof(*Components[2].TrimStartAndEnd());
                        Const4Expr->Constant.A = 1.0f;
                        bValueChanged = true;
                    }
                    else
                    {
                        OutError = FString::Printf(TEXT("Constant4Vector requires 3-4 comma-separated values (R,G,B[,A]), got: %s"), *ColorString);
                        return false;
                    }
                }
                else
                {
                    OutError = TEXT("Constant4Vector value must be an array [R,G,B,A] or string \"R,G,B,A\"");
                    return false;
                }
            }
        }

        // Trigger PostEditChangeProperty to update UI
        if (bValueChanged)
        {
            FProperty* ConstantProp = Const4Expr->GetClass()->FindPropertyByName(TEXT("Constant"));
            if (ConstantProp)
            {
                FPropertyChangedEvent PropertyChangedEvent(ConstantProp, EPropertyChangeType::ValueSet);
                Const4Expr->PostEditChangeProperty(PropertyChangedEvent);
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
    // Handle ParticleSubUV (flipbook texture sampler for particles)
    // NOTE: Must be checked BEFORE TextureSample since ParticleSubUV inherits from it
    else if (UMaterialExpressionParticleSubUV* ParticleSubUVExpr = Cast<UMaterialExpressionParticleSubUV>(Expression))
    {
        if (Properties->HasField(TEXT("texture")))
        {
            FString TexturePath = Properties->GetStringField(TEXT("texture"));
            UTexture* Texture = LoadObject<UTexture>(nullptr, *TexturePath);
            if (Texture)
            {
                ParticleSubUVExpr->Texture = Texture;
            }
        }
        // Handle bBlend property - whether to blend between SubUV frames
        if (Properties->HasField(TEXT("blend")) || Properties->HasField(TEXT("bBlend")))
        {
            ParticleSubUVExpr->bBlend = Properties->HasField(TEXT("blend"))
                ? Properties->GetBoolField(TEXT("blend"))
                : Properties->GetBoolField(TEXT("bBlend"));
        }
        // Handle SamplerType property (inherited from TextureSample)
        if (Properties->HasField(TEXT("SamplerType")) || Properties->HasField(TEXT("sampler_type")))
        {
            int32 SamplerTypeValue = Properties->HasField(TEXT("SamplerType"))
                ? (int32)Properties->GetNumberField(TEXT("SamplerType"))
                : (int32)Properties->GetNumberField(TEXT("sampler_type"));
            ParticleSubUVExpr->SamplerType = (EMaterialSamplerType)SamplerTypeValue;
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
    // Handle Custom HLSL expression
    else if (UMaterialExpressionCustom* CustomExpr = Cast<UMaterialExpressionCustom>(Expression))
    {
        if (Properties->HasField(TEXT("Code")) || Properties->HasField(TEXT("code")))
        {
            CustomExpr->Code = Properties->HasField(TEXT("Code"))
                ? Properties->GetStringField(TEXT("Code"))
                : Properties->GetStringField(TEXT("code"));
        }

        if (Properties->HasField(TEXT("OutputType")) || Properties->HasField(TEXT("output_type")))
        {
            int32 OutType = Properties->HasField(TEXT("OutputType"))
                ? (int32)Properties->GetNumberField(TEXT("OutputType"))
                : (int32)Properties->GetNumberField(TEXT("output_type"));
            CustomExpr->OutputType = (ECustomMaterialOutputType)OutType;
        }

        if (Properties->HasField(TEXT("Description")) || Properties->HasField(TEXT("description")))
        {
            CustomExpr->Description = Properties->HasField(TEXT("Description"))
                ? Properties->GetStringField(TEXT("Description"))
                : Properties->GetStringField(TEXT("description"));
        }

        // Handle named inputs — critical for HLSL code to reference connected values
        const TArray<TSharedPtr<FJsonValue>>* InputsArray;
        if (Properties->TryGetArrayField(TEXT("Inputs"), InputsArray) || Properties->TryGetArrayField(TEXT("inputs"), InputsArray))
        {
            CustomExpr->Inputs.Empty();
            for (const auto& InputVal : *InputsArray)
            {
                const TSharedPtr<FJsonObject>* InputObj;
                if (InputVal->TryGetObject(InputObj))
                {
                    FCustomInput NewInput;
                    FString InputName;
                    if ((*InputObj)->TryGetStringField(TEXT("InputName"), InputName) ||
                        (*InputObj)->TryGetStringField(TEXT("input_name"), InputName) ||
                        (*InputObj)->TryGetStringField(TEXT("name"), InputName))
                    {
                        NewInput.InputName = FName(*InputName);
                    }
                    CustomExpr->Inputs.Add(NewInput);
                }
            }
            UE_LOG(LogTemp, Log, TEXT("Custom expression: set %d named inputs"), CustomExpr->Inputs.Num());
        }

        // Handle additional outputs
        const TArray<TSharedPtr<FJsonValue>>* OutputsArray;
        if (Properties->TryGetArrayField(TEXT("AdditionalOutputs"), OutputsArray) || Properties->TryGetArrayField(TEXT("additional_outputs"), OutputsArray))
        {
            CustomExpr->AdditionalOutputs.Empty();
            for (const auto& OutputVal : *OutputsArray)
            {
                const TSharedPtr<FJsonObject>* OutputObj;
                if (OutputVal->TryGetObject(OutputObj))
                {
                    FCustomOutput NewOutput;
                    FString OutputName;
                    if ((*OutputObj)->TryGetStringField(TEXT("OutputName"), OutputName) ||
                        (*OutputObj)->TryGetStringField(TEXT("name"), OutputName))
                    {
                        NewOutput.OutputName = FName(*OutputName);
                    }
                    if ((*OutputObj)->HasField(TEXT("OutputType")))
                    {
                        NewOutput.OutputType = (ECustomMaterialOutputType)(int32)(*OutputObj)->GetNumberField(TEXT("OutputType"));
                    }
                    CustomExpr->AdditionalOutputs.Add(NewOutput);
                }
            }
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

    // Find the working material (editor's transient copy if editor is open)
    // Also get the MaterialEditor pointer if it's open
    TSharedPtr<IMaterialEditor> MaterialEditor;
    UMaterial* Material = FindWorkingMaterial(Params.MaterialPath, OutError, &MaterialEditor);
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

    // If Material Editor is open, use its API for proper UI refresh
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
                // Mark expression for modification before changing properties
                NewExpression->Modify();

                if (!ApplyExpressionProperties(NewExpression, Params.Properties, OutError))
                {
                    // Property validation failed - clean up and return
                    Material->GetEditorOnlyData()->ExpressionCollection.RemoveExpression(NewExpression);
                    NewExpression->MarkAsGarbage();
                    return nullptr;
                }

                // After setting properties, we need to update the graph node to reflect changes
                // Find the graph node for this expression and reconstruct it
                if (Material->MaterialGraph)
                {
                    for (UEdGraphNode* Node : Material->MaterialGraph->Nodes)
                    {
                        UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(Node);
                        if (MatNode && MatNode->MaterialExpression == NewExpression)
                        {
                            // Reconstruct the node to pick up property changes
                            MatNode->ReconstructNode();
                            break;
                        }
                    }
                }
            }

            // Notify graph of changes
            if (Material->MaterialGraph)
            {
                Material->MaterialGraph->NotifyGraphChanged();
            }

            // Mark dirty (let user save when ready)
            Material->MarkPackageDirty();
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
            // Mark expression for modification before changing properties
            NewExpression->Modify();

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

        // Ensure graph exists and rebuild to create visual nodes
        EnsureMaterialGraph(Material);
        Material->MaterialGraph->Modify();
        Material->MaterialGraph->RebuildGraph();
        Material->MaterialGraph->NotifyGraphChanged();

        // Mark dirty (let user save when ready)
        Material->MarkPackageDirty();
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
