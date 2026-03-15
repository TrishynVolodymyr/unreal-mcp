#include "Services/MaterialExpressionService.h"
#include "MaterialEditingLibrary.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "UObject/SavePackage.h"

UMaterialExpression* FMaterialExpressionService::AddExpressionToFunction(
    const FMaterialExpressionCreationParams& Params,
    TSharedPtr<FJsonObject>& OutExpressionInfo,
    FString& OutError)
{
    // Load the MaterialFunction asset
    UMaterialFunction* MatFunc = LoadObject<UMaterialFunction>(nullptr, *Params.MaterialFunctionPath);
    if (!MatFunc)
    {
        OutError = FString::Printf(TEXT("MaterialFunction not found: %s"), *Params.MaterialFunctionPath);
        return nullptr;
    }

    // Get expression class
    UClass* ExpressionClass = GetExpressionClassFromTypeName(Params.ExpressionType);
    if (!ExpressionClass)
    {
        OutError = FString::Printf(TEXT("Unknown expression type: %s"), *Params.ExpressionType);
        return nullptr;
    }

    // Create expression in the MaterialFunction using UE's API
    UMaterialExpression* NewExpression = UMaterialEditingLibrary::CreateMaterialExpressionInFunction(
        MatFunc,
        ExpressionClass,
        (int32)Params.Position.X,
        (int32)Params.Position.Y
    );

    if (!NewExpression)
    {
        OutError = FString::Printf(TEXT("Failed to create expression %s in MaterialFunction"), *Params.ExpressionType);
        return nullptr;
    }

    // Apply type-specific properties
    if (Params.Properties.IsValid())
    {
        NewExpression->Modify();
        if (!ApplyExpressionProperties(NewExpression, Params.Properties, OutError))
        {
            // Clean up on failure
            UMaterialEditingLibrary::DeleteMaterialExpressionInFunction(MatFunc, NewExpression);
            return nullptr;
        }
    }

    // Handle FunctionInput-specific properties
    if (UMaterialExpressionFunctionInput* InputExpr = Cast<UMaterialExpressionFunctionInput>(NewExpression))
    {
        if (Params.Properties.IsValid())
        {
            FString InputName;
            if (Params.Properties->TryGetStringField(TEXT("InputName"), InputName) ||
                Params.Properties->TryGetStringField(TEXT("input_name"), InputName))
            {
                InputExpr->InputName = FName(*InputName);
            }

            FString InputTypeStr;
            if (Params.Properties->TryGetStringField(TEXT("InputType"), InputTypeStr) ||
                Params.Properties->TryGetStringField(TEXT("input_type"), InputTypeStr))
            {
                if (InputTypeStr == TEXT("Scalar") || InputTypeStr == TEXT("Float"))
                    InputExpr->InputType = FunctionInput_Scalar;
                else if (InputTypeStr == TEXT("Vector2"))
                    InputExpr->InputType = FunctionInput_Vector2;
                else if (InputTypeStr == TEXT("Vector3"))
                    InputExpr->InputType = FunctionInput_Vector3;
                else if (InputTypeStr == TEXT("Vector4"))
                    InputExpr->InputType = FunctionInput_Vector4;
                else if (InputTypeStr == TEXT("Texture2D"))
                    InputExpr->InputType = FunctionInput_Texture2D;
                else if (InputTypeStr == TEXT("Bool") || InputTypeStr == TEXT("StaticBool"))
                    InputExpr->InputType = FunctionInput_StaticBool;
            }

            int32 SortPriority;
            if (Params.Properties->TryGetNumberField(TEXT("SortPriority"), SortPriority) ||
                Params.Properties->TryGetNumberField(TEXT("sort_priority"), SortPriority))
            {
                InputExpr->SortPriority = SortPriority;
            }
        }
        InputExpr->ConditionallyGenerateId(true);
    }

    // Handle FunctionOutput-specific properties
    if (UMaterialExpressionFunctionOutput* OutputExpr = Cast<UMaterialExpressionFunctionOutput>(NewExpression))
    {
        if (Params.Properties.IsValid())
        {
            FString OutputName;
            if (Params.Properties->TryGetStringField(TEXT("OutputName"), OutputName) ||
                Params.Properties->TryGetStringField(TEXT("output_name"), OutputName))
            {
                OutputExpr->OutputName = FName(*OutputName);
            }

            int32 SortPriority;
            if (Params.Properties->TryGetNumberField(TEXT("SortPriority"), SortPriority) ||
                Params.Properties->TryGetNumberField(TEXT("sort_priority"), SortPriority))
            {
                OutputExpr->SortPriority = SortPriority;
            }
        }
        OutputExpr->ConditionallyGenerateId(true);
    }

    // Update and save
    UMaterialEditingLibrary::UpdateMaterialFunction(MatFunc, nullptr);
    MatFunc->MarkPackageDirty();

    // Save to disk
    UPackage* Package = MatFunc->GetOutermost();
    FString PackageFilename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, MatFunc, *PackageFilename, SaveArgs);

    UE_LOG(LogTemp, Log, TEXT("Added %s to MaterialFunction %s"), *Params.ExpressionType, *Params.MaterialFunctionPath);

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
    OutExpressionInfo->SetStringField(TEXT("message"),
        FString::Printf(TEXT("Expression %s added to MaterialFunction %s"), *Params.ExpressionType, *Params.MaterialFunctionPath));

    return NewExpression;
}

UMaterialExpression* FMaterialExpressionService::FindExpressionInFunction(UMaterialFunction* Function, const FGuid& ExpressionId)
{
    if (!Function) return nullptr;

    const TArray<TObjectPtr<UMaterialExpression>>& Expressions = Function->GetExpressionCollection().Expressions;
    for (UMaterialExpression* Expr : Expressions)
    {
        if (Expr && Expr->MaterialExpressionGuid == ExpressionId)
        {
            return Expr;
        }
    }
    return nullptr;
}
