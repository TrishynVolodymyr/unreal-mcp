#include "Commands/Material/SetMaterialPropertiesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Materials/Material.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"

FString FSetMaterialPropertiesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    // Required parameter: material_path
    FString MaterialPath;
    if (!JsonObject->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return CreateErrorResponse(TEXT("Missing required 'material_path' parameter"));
    }

    // Load the material
    UMaterialInterface* MaterialInterface = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    if (!MaterialInterface)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    // Get the base material (we need to modify UMaterial, not UMaterialInstance)
    UMaterial* Material = Cast<UMaterial>(MaterialInterface);
    if (!Material)
    {
        // If it's a material instance, try to get its parent
        Material = MaterialInterface->GetMaterial();
        if (!Material)
        {
            return CreateErrorResponse(TEXT("Cannot modify a Material Instance. Please provide the path to the base Material."));
        }
        // Update the path to the actual material we're modifying
        MaterialPath = Material->GetPathName();
        UE_LOG(LogTemp, Warning, TEXT("Modifying base material %s instead of instance"), *MaterialPath);
    }

    TArray<FString> ChangedProperties;

    // Optional: blend_mode
    FString BlendModeStr;
    if (JsonObject->TryGetStringField(TEXT("blend_mode"), BlendModeStr))
    {
        EBlendMode NewBlendMode = BLEND_Opaque;
        if (BlendModeStr.Equals(TEXT("Opaque"), ESearchCase::IgnoreCase))
            NewBlendMode = BLEND_Opaque;
        else if (BlendModeStr.Equals(TEXT("Masked"), ESearchCase::IgnoreCase))
            NewBlendMode = BLEND_Masked;
        else if (BlendModeStr.Equals(TEXT("Translucent"), ESearchCase::IgnoreCase))
            NewBlendMode = BLEND_Translucent;
        else if (BlendModeStr.Equals(TEXT("Additive"), ESearchCase::IgnoreCase))
            NewBlendMode = BLEND_Additive;
        else if (BlendModeStr.Equals(TEXT("Modulate"), ESearchCase::IgnoreCase))
            NewBlendMode = BLEND_Modulate;
        else if (BlendModeStr.Equals(TEXT("AlphaComposite"), ESearchCase::IgnoreCase))
            NewBlendMode = BLEND_AlphaComposite;
        else if (BlendModeStr.Equals(TEXT("AlphaHoldout"), ESearchCase::IgnoreCase))
            NewBlendMode = BLEND_AlphaHoldout;
        else
        {
            return CreateErrorResponse(FString::Printf(TEXT("Invalid blend_mode: %s. Valid options: Opaque, Masked, Translucent, Additive, Modulate, AlphaComposite, AlphaHoldout"), *BlendModeStr));
        }

        Material->BlendMode = NewBlendMode;
        ChangedProperties.Add(FString::Printf(TEXT("BlendMode=%s"), *BlendModeStr));
        UE_LOG(LogTemp, Log, TEXT("Set BlendMode to %s"), *BlendModeStr);
    }

    // Optional: shading_model
    FString ShadingModelStr;
    if (JsonObject->TryGetStringField(TEXT("shading_model"), ShadingModelStr))
    {
        EMaterialShadingModel NewShadingModel = MSM_DefaultLit;
        if (ShadingModelStr.Equals(TEXT("Unlit"), ESearchCase::IgnoreCase))
            NewShadingModel = MSM_Unlit;
        else if (ShadingModelStr.Equals(TEXT("DefaultLit"), ESearchCase::IgnoreCase))
            NewShadingModel = MSM_DefaultLit;
        else if (ShadingModelStr.Equals(TEXT("Subsurface"), ESearchCase::IgnoreCase))
            NewShadingModel = MSM_Subsurface;
        else if (ShadingModelStr.Equals(TEXT("PreintegratedSkin"), ESearchCase::IgnoreCase))
            NewShadingModel = MSM_PreintegratedSkin;
        else if (ShadingModelStr.Equals(TEXT("ClearCoat"), ESearchCase::IgnoreCase))
            NewShadingModel = MSM_ClearCoat;
        else if (ShadingModelStr.Equals(TEXT("SubsurfaceProfile"), ESearchCase::IgnoreCase))
            NewShadingModel = MSM_SubsurfaceProfile;
        else if (ShadingModelStr.Equals(TEXT("TwoSidedFoliage"), ESearchCase::IgnoreCase))
            NewShadingModel = MSM_TwoSidedFoliage;
        else if (ShadingModelStr.Equals(TEXT("Hair"), ESearchCase::IgnoreCase))
            NewShadingModel = MSM_Hair;
        else if (ShadingModelStr.Equals(TEXT("Cloth"), ESearchCase::IgnoreCase))
            NewShadingModel = MSM_Cloth;
        else if (ShadingModelStr.Equals(TEXT("Eye"), ESearchCase::IgnoreCase))
            NewShadingModel = MSM_Eye;
        else if (ShadingModelStr.Equals(TEXT("SingleLayerWater"), ESearchCase::IgnoreCase))
            NewShadingModel = MSM_SingleLayerWater;
        else if (ShadingModelStr.Equals(TEXT("ThinTranslucent"), ESearchCase::IgnoreCase))
            NewShadingModel = MSM_ThinTranslucent;
        else
        {
            return CreateErrorResponse(FString::Printf(TEXT("Invalid shading_model: %s. Valid options: Unlit, DefaultLit, Subsurface, ClearCoat, SubsurfaceProfile, TwoSidedFoliage, Hair, Cloth, Eye, SingleLayerWater, ThinTranslucent"), *ShadingModelStr));
        }

        Material->SetShadingModel(NewShadingModel);
        ChangedProperties.Add(FString::Printf(TEXT("ShadingModel=%s"), *ShadingModelStr));
        UE_LOG(LogTemp, Log, TEXT("Set ShadingModel to %s"), *ShadingModelStr);
    }

    // Optional: two_sided
    bool bTwoSided;
    if (JsonObject->TryGetBoolField(TEXT("two_sided"), bTwoSided))
    {
        Material->TwoSided = bTwoSided;
        ChangedProperties.Add(FString::Printf(TEXT("TwoSided=%s"), bTwoSided ? TEXT("true") : TEXT("false")));
        UE_LOG(LogTemp, Log, TEXT("Set TwoSided to %s"), bTwoSided ? TEXT("true") : TEXT("false"));
    }

    // Optional: material_domain
    FString MaterialDomainStr;
    if (JsonObject->TryGetStringField(TEXT("material_domain"), MaterialDomainStr))
    {
        EMaterialDomain NewDomain = MD_Surface;
        if (MaterialDomainStr.Equals(TEXT("Surface"), ESearchCase::IgnoreCase))
            NewDomain = MD_Surface;
        else if (MaterialDomainStr.Equals(TEXT("DeferredDecal"), ESearchCase::IgnoreCase) || MaterialDomainStr.Equals(TEXT("Decal"), ESearchCase::IgnoreCase))
            NewDomain = MD_DeferredDecal;
        else if (MaterialDomainStr.Equals(TEXT("LightFunction"), ESearchCase::IgnoreCase))
            NewDomain = MD_LightFunction;
        else if (MaterialDomainStr.Equals(TEXT("Volume"), ESearchCase::IgnoreCase))
            NewDomain = MD_Volume;
        else if (MaterialDomainStr.Equals(TEXT("PostProcess"), ESearchCase::IgnoreCase))
            NewDomain = MD_PostProcess;
        else if (MaterialDomainStr.Equals(TEXT("UserInterface"), ESearchCase::IgnoreCase) || MaterialDomainStr.Equals(TEXT("UI"), ESearchCase::IgnoreCase))
            NewDomain = MD_UI;
        else
        {
            return CreateErrorResponse(FString::Printf(TEXT("Invalid material_domain: %s. Valid options: Surface, DeferredDecal, LightFunction, Volume, PostProcess, UserInterface (or UI)"), *MaterialDomainStr));
        }

        Material->MaterialDomain = NewDomain;
        ChangedProperties.Add(FString::Printf(TEXT("MaterialDomain=%s"), *MaterialDomainStr));
        UE_LOG(LogTemp, Log, TEXT("Set MaterialDomain to %s"), *MaterialDomainStr);
    }

    // Optional: Usage flags
    bool bValue;
    if (JsonObject->TryGetBoolField(TEXT("used_with_niagara_sprites"), bValue))
    {
        Material->bUsedWithNiagaraSprites = bValue;
        ChangedProperties.Add(FString::Printf(TEXT("bUsedWithNiagaraSprites=%s"), bValue ? TEXT("true") : TEXT("false")));
    }
    if (JsonObject->TryGetBoolField(TEXT("used_with_niagara_ribbons"), bValue))
    {
        Material->bUsedWithNiagaraRibbons = bValue;
        ChangedProperties.Add(FString::Printf(TEXT("bUsedWithNiagaraRibbons=%s"), bValue ? TEXT("true") : TEXT("false")));
    }
    if (JsonObject->TryGetBoolField(TEXT("used_with_niagara_mesh_particles"), bValue))
    {
        Material->bUsedWithNiagaraMeshParticles = bValue;
        ChangedProperties.Add(FString::Printf(TEXT("bUsedWithNiagaraMeshParticles=%s"), bValue ? TEXT("true") : TEXT("false")));
    }
    if (JsonObject->TryGetBoolField(TEXT("used_with_particle_sprites"), bValue))
    {
        Material->bUsedWithParticleSprites = bValue;
        ChangedProperties.Add(FString::Printf(TEXT("bUsedWithParticleSprites=%s"), bValue ? TEXT("true") : TEXT("false")));
    }
    if (JsonObject->TryGetBoolField(TEXT("used_with_mesh_particles"), bValue))
    {
        Material->bUsedWithMeshParticles = bValue;
        ChangedProperties.Add(FString::Printf(TEXT("bUsedWithMeshParticles=%s"), bValue ? TEXT("true") : TEXT("false")));
    }
    if (JsonObject->TryGetBoolField(TEXT("used_with_skeletal_mesh"), bValue))
    {
        Material->bUsedWithSkeletalMesh = bValue;
        ChangedProperties.Add(FString::Printf(TEXT("bUsedWithSkeletalMesh=%s"), bValue ? TEXT("true") : TEXT("false")));
    }
    if (JsonObject->TryGetBoolField(TEXT("used_with_static_lighting"), bValue))
    {
        Material->bUsedWithStaticLighting = bValue;
        ChangedProperties.Add(FString::Printf(TEXT("bUsedWithStaticLighting=%s"), bValue ? TEXT("true") : TEXT("false")));
    }

    // Check if any properties were changed
    if (ChangedProperties.Num() == 0)
    {
        return CreateErrorResponse(TEXT("No valid properties provided to change. Supported: material_domain, blend_mode, shading_model, two_sided, used_with_niagara_sprites, used_with_niagara_ribbons, used_with_niagara_mesh_particles, used_with_particle_sprites, used_with_mesh_particles, used_with_skeletal_mesh, used_with_static_lighting"));
    }

    // Mark package dirty
    UPackage* Package = Material->GetOutermost();
    if (Package)
    {
        Package->MarkPackageDirty();
    }

    // Trigger material recompilation
    Material->PreEditChange(nullptr);
    Material->PostEditChange();

    // Save the package
    if (Package)
    {
        FString PackageName = Package->GetName();
        FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        bool bSaved = UPackage::SavePackage(Package, Material, *PackageFileName, SaveArgs);

        if (bSaved)
        {
            UE_LOG(LogTemp, Log, TEXT("Saved material package: %s"), *PackageFileName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to save material package: %s"), *PackageFileName);
        }
    }

    return CreateSuccessResponse(MaterialPath, ChangedProperties);
}

FString FSetMaterialPropertiesCommand::GetCommandName() const
{
    return TEXT("set_material_properties");
}

bool FSetMaterialPropertiesCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString MaterialPath;
    return JsonObject->TryGetStringField(TEXT("material_path"), MaterialPath);
}

FString FSetMaterialPropertiesCommand::CreateSuccessResponse(const FString& MaterialPath, const TArray<FString>& ChangedProperties) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("material_path"), MaterialPath);

    TArray<TSharedPtr<FJsonValue>> PropsArray;
    for (const FString& Prop : ChangedProperties)
    {
        PropsArray.Add(MakeShared<FJsonValueString>(Prop));
    }
    ResponseObj->SetArrayField(TEXT("changed_properties"), PropsArray);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Updated %d properties on material %s. Material will recompile shaders."), ChangedProperties.Num(), *MaterialPath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetMaterialPropertiesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
