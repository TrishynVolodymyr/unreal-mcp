#include "Services/MaterialExpressionService.h"
#include "MaterialEditingLibrary.h"
#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpressionSetMaterialAttributes.h"
#include "Materials/MaterialExpressionBreakMaterialAttributes.h"
#include "Materials/MaterialExpressionGetMaterialAttributes.h"
#include "Materials/MaterialAttributeDefinitionMap.h"
#include "MaterialShared.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace
{
    // Strict name -> EMaterialProperty map. Unlike the service's
    // GetMaterialPropertyFromString (which defaults unknown names to
    // MP_EmissiveColor), this returns MP_MAX for anything unrecognized so we
    // never silently create the wrong attribute pin.
    EMaterialProperty NameToMaterialPropertyStrict(const FString& Name)
    {
        static const TMap<FString, EMaterialProperty> Map = {
            { TEXT("BaseColor"),            MP_BaseColor },
            { TEXT("Metallic"),             MP_Metallic },
            { TEXT("Specular"),             MP_Specular },
            { TEXT("Roughness"),            MP_Roughness },
            { TEXT("Anisotropy"),           MP_Anisotropy },
            { TEXT("EmissiveColor"),        MP_EmissiveColor },
            { TEXT("Opacity"),              MP_Opacity },
            { TEXT("OpacityMask"),          MP_OpacityMask },
            { TEXT("Normal"),               MP_Normal },
            { TEXT("Tangent"),              MP_Tangent },
            { TEXT("WorldPositionOffset"),  MP_WorldPositionOffset },
            { TEXT("SubsurfaceColor"),      MP_SubsurfaceColor },
            { TEXT("AmbientOcclusion"),     MP_AmbientOcclusion },
            { TEXT("Refraction"),           MP_Refraction },
            { TEXT("PixelDepthOffset"),     MP_PixelDepthOffset },
            { TEXT("Displacement"),         MP_Displacement },
        };
        for (const TPair<FString, EMaterialProperty>& Pair : Map)
        {
            if (Name.Equals(Pair.Key, ESearchCase::IgnoreCase))
            {
                return Pair.Value;
            }
        }
        return MP_MAX;
    }

    // Resolve the target input pin by name. Handles the normal named-input case
    // and the special SetMaterialAttributes node, whose per-attribute input pins
    // (e.g. "OpacityMask") do not exist until the attribute's GUID is added to
    // AttributeSetTypes. When the requested name is a known material attribute we
    // add the pin on demand so callers can wire it. Returns nullptr (and fills
    // OutAvailable) if the input cannot be resolved.
    FExpressionInput* ResolveTargetInput(UMaterialExpression* TargetExpr, const FString& InputName, FString& OutAvailable)
    {
        const int32 NumInputs = TargetExpr->GetInputsView().Num();
        for (int32 i = 0; i < NumInputs; ++i)
        {
            if (TargetExpr->GetInputName(i).ToString().Equals(InputName, ESearchCase::IgnoreCase))
            {
                return TargetExpr->GetInput(i);
            }
        }

        // SetMaterialAttributes: expose the requested attribute pin via the engine's own
        // CreateOrGetInputAttribute(). The earlier approach manually appended to
        // AttributeSetTypes + Inputs; that compiled in-session but the override did NOT
        // survive serialization — on reload the emissive/basecolor pin reverted to default
        // (only OpacityMask, which masked-blend forces live, appeared to work). The engine
        // API does the node's full internal bookkeeping so the override persists + compiles.
        if (UMaterialExpressionSetMaterialAttributes* SetAttr = Cast<UMaterialExpressionSetMaterialAttributes>(TargetExpr))
        {
            const EMaterialProperty Prop = NameToMaterialPropertyStrict(InputName);
            if (Prop != MP_MAX)
            {
                SetAttr->Modify();
                const int32 InputIndex = SetAttr->CreateOrGetInputAttribute(Prop);
                if (FExpressionInput* AttrInput = SetAttr->GetInput(InputIndex))
                {
                    return AttrInput;
                }
            }
        }

        // Break/GetMaterialAttributes expose their single MaterialAttributes input as a
        // special FMaterialAttributesInput member that is NOT part of GetInputsView(), so
        // the name loop above can never find it. Resolve it directly (these nodes have
        // exactly one input, so any requested name — including "" — maps to it).
        if (UMaterialExpressionBreakMaterialAttributes* BreakAttr = Cast<UMaterialExpressionBreakMaterialAttributes>(TargetExpr))
        {
            return &BreakAttr->MaterialAttributes;
        }
        if (UMaterialExpressionGetMaterialAttributes* GetAttr = Cast<UMaterialExpressionGetMaterialAttributes>(TargetExpr))
        {
            return &GetAttr->MaterialAttributes;
        }

        TArray<FString> Available;
        for (int32 i = 0; i < NumInputs; ++i)
        {
            Available.Add(TargetExpr->GetInputName(i).ToString());
        }
        OutAvailable = FString::Join(Available, TEXT(", "));
        return nullptr;
    }
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

    // ---- MaterialFunction branch ----
    if (Params.IsForMaterialFunction())
    {
        UMaterialFunction* MatFunc = LoadObject<UMaterialFunction>(nullptr, *Params.MaterialFunctionPath);
        if (!MatFunc)
        {
            OutError = FString::Printf(TEXT("MaterialFunction not found: %s"), *Params.MaterialFunctionPath);
            return false;
        }

        UMaterialExpression* SourceExpr = FindExpressionInFunction(MatFunc, Params.SourceExpressionId);
        UMaterialExpression* TargetExpr = FindExpressionInFunction(MatFunc, Params.TargetExpressionId);

        if (!SourceExpr) { OutError = FString::Printf(TEXT("Source expression not found: %s"), *Params.SourceExpressionId.ToString()); return false; }
        if (!TargetExpr) { OutError = FString::Printf(TEXT("Target expression not found: %s"), *Params.TargetExpressionId.ToString()); return false; }

        if (Params.SourceOutputIndex < 0 || Params.SourceOutputIndex >= SourceExpr->GetOutputs().Num())
        {
            OutError = FString::Printf(TEXT("Invalid source output index: %d"), Params.SourceOutputIndex);
            return false;
        }

        // Resolve target input (handles SetMaterialAttributes dynamic attribute pins)
        FString AvailableInputs;
        FExpressionInput* TargetInput = ResolveTargetInput(TargetExpr, Params.TargetInputName, AvailableInputs);
        if (!TargetInput)
        {
            OutError = FString::Printf(TEXT("Input '%s' not found. Available: %s"),
                *Params.TargetInputName, *AvailableInputs);
            return false;
        }

        SourceExpr->Modify();
        TargetExpr->Modify();
        SourceExpr->ConnectExpression(TargetInput, Params.SourceOutputIndex);

        UMaterialEditingLibrary::UpdateMaterialFunction(MatFunc, nullptr);
        MatFunc->MarkPackageDirty();

        // Save
        UPackage* Package = MatFunc->GetOutermost();
        FString PackageFilename = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Package, MatFunc, *PackageFilename, SaveArgs);

        UE_LOG(LogTemp, Log, TEXT("Connected expressions in MaterialFunction %s"), *Params.MaterialFunctionPath);
        return true;
    }

    // Find the working material (editor's transient copy if editor is open)
    UMaterial* Material = FindWorkingMaterial(Params.MaterialPath, OutError);
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

    // Resolve target input (handles SetMaterialAttributes dynamic attribute pins)
    FString AvailableInputs;
    FExpressionInput* TargetInput = ResolveTargetInput(TargetExpr, Params.TargetInputName, AvailableInputs);
    if (!TargetInput)
    {
        OutError = FString::Printf(TEXT("Input '%s' not found on target expression. Available inputs: %s"),
            *Params.TargetInputName, *AvailableInputs);
        return false;
    }

    // Mark objects for modification (Undo/Redo support)
    SourceExpr->Modify();
    TargetExpr->Modify();
    Material->Modify();

    // Use UE5's built-in ConnectExpression() method - this correctly sets ALL fields
    // including the Mask fields (MaskR, MaskG, MaskB, MaskA) that direct assignment misses
    SourceExpr->ConnectExpression(TargetInput, Params.SourceOutputIndex);

    UE_LOG(LogTemp, Log, TEXT("Connected %s[%d] -> %s.%s using ConnectExpression()"),
        *SourceExpr->GetName(), Params.SourceOutputIndex,
        *TargetExpr->GetName(), *Params.TargetInputName);

    // Ensure MaterialGraph exists for visual sync
    EnsureMaterialGraph(Material);

    // For connections, we only need to update links - nodes already exist
    // LinkGraphNodesFromMaterial syncs the graph links (wires) from expression connections
    // Do NOT call RebuildGraph() here as it destroys/recreates all nodes, causing UI issues
    Material->MaterialGraph->Modify();
    Material->MaterialGraph->LinkGraphNodesFromMaterial();
    Material->MaterialGraph->NotifyGraphChanged();

    // Mark package dirty (let user save when ready)
    Material->MarkPackageDirty();

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

    // Find the working material once (editor's transient copy if editor is open)
    UMaterial* Material = FindWorkingMaterial(MaterialPath, OutError);
    if (!Material)
    {
        return false;
    }

    // Mark material for modification once
    Material->Modify();

    // Process each connection
    int32 SuccessCount = 0;
    for (const FMaterialExpressionConnectionParams& Conn : Connections)
    {
        // Validate connection
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

        // Resolve target input (handles SetMaterialAttributes dynamic attribute pins)
        FString AvailableInputs;
        FExpressionInput* TargetInput = ResolveTargetInput(TargetExpr, Conn.TargetInputName, AvailableInputs);
        if (!TargetInput)
        {
            OutResults.Add(FString::Printf(TEXT("FAILED: Input '%s' not found on target. Available: %s"), *Conn.TargetInputName, *AvailableInputs));
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

    // Ensure graph exists and update links once after all connections
    EnsureMaterialGraph(Material);
    Material->MaterialGraph->Modify();
    Material->MaterialGraph->LinkGraphNodesFromMaterial();
    Material->MaterialGraph->NotifyGraphChanged();

    // Mark dirty (let user save when ready)
    Material->MarkPackageDirty();

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
    // Find the working material (editor's transient copy if editor is open)
    UMaterial* Material = FindWorkingMaterial(MaterialPath, OutError);
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

    // Mark objects for modification (Undo/Redo support)
    Expression->Modify();
    Material->Modify();

    // Connect at material data level using UE5's built-in ConnectExpression()
    Expression->ConnectExpression(MaterialInput, OutputIndex);

    // Ensure MaterialGraph exists and update links to sync visual representation
    // Use LinkGraphNodesFromMaterial (not RebuildGraph) to preserve existing node references
    EnsureMaterialGraph(Material);
    Material->MaterialGraph->Modify();
    Material->MaterialGraph->LinkGraphNodesFromMaterial();
    Material->MaterialGraph->NotifyGraphChanged();

    // Mark package dirty (let user save when ready)
    Material->MarkPackageDirty();

    UE_LOG(LogTemp, Log, TEXT("Connected expression %s to %s in material %s"),
        *Expression->GetName(), *MaterialProperty, *MaterialPath);

    return true;
}
