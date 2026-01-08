#include "Services/MaterialExpressionService.h"
#include "Dom/JsonValue.h"

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
