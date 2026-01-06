#include "Services/MaterialExpressionService.h"
#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

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
