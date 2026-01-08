#include "Services/MaterialService.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/SavePackage.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "Dom/JsonValue.h"

// Singleton instance
TUniquePtr<FMaterialService> FMaterialService::Instance;

FMaterialService::FMaterialService()
{
    UE_LOG(LogTemp, Log, TEXT("FMaterialService initialized"));
}

FMaterialService& FMaterialService::Get()
{
    if (!Instance.IsValid())
    {
        Instance = MakeUnique<FMaterialService>();
    }
    return *Instance;
}

UMaterial* FMaterialService::CreateMaterial(const FMaterialCreationParams& Params, FString& OutMaterialPath, FString& OutError)
{
    // Validate parameters
    if (!Params.IsValid(OutError))
    {
        return nullptr;
    }

    // Build full path
    FString PackagePath = Params.Path / Params.Name;
    FString PackageName = FPackageName::ObjectPathToPackageName(PackagePath);

    UE_LOG(LogTemp, Log, TEXT("Creating material at path: %s"), *PackagePath);

    // Create the package
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        OutError = FString::Printf(TEXT("Failed to create package for material: %s"), *PackageName);
        return nullptr;
    }

    // Create material factory
    UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
    if (!MaterialFactory)
    {
        OutError = TEXT("Failed to create material factory");
        return nullptr;
    }

    // Create the material
    UMaterial* NewMaterial = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
        UMaterial::StaticClass(),
        Package,
        *Params.Name,
        RF_Public | RF_Standalone,
        nullptr,
        GWarn
    ));

    if (!NewMaterial)
    {
        OutError = TEXT("Failed to create material asset");
        return nullptr;
    }

    // Set blend mode
    EBlendMode BlendMode = GetBlendModeFromString(Params.BlendMode);
    NewMaterial->BlendMode = BlendMode;

    // Set shading model
    EMaterialShadingModel ShadingModel = GetShadingModelFromString(Params.ShadingModel);
    NewMaterial->SetShadingModel(ShadingModel);

    // Mark package dirty and register
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewMaterial);

    // Build output path
    OutMaterialPath = PackagePath;

    UE_LOG(LogTemp, Log, TEXT("Successfully created material: %s"), *OutMaterialPath);

    return NewMaterial;
}

UMaterialInterface* FMaterialService::CreateMaterialInstance(const FMaterialInstanceCreationParams& Params, FString& OutInstancePath, FString& OutError)
{
    // Validate parameters
    if (!Params.IsValid(OutError))
    {
        return nullptr;
    }

    // Find parent material
    UMaterialInterface* ParentMaterial = FindMaterial(Params.ParentMaterialPath);
    if (!ParentMaterial)
    {
        OutError = FString::Printf(TEXT("Parent material not found: %s"), *Params.ParentMaterialPath);
        return nullptr;
    }

    if (Params.bIsDynamic)
    {
        // Create dynamic material instance (runtime modifiable)
        UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ParentMaterial, GetTransientPackage(), *Params.Name);
        if (!MID)
        {
            OutError = TEXT("Failed to create dynamic material instance");
            return nullptr;
        }

        OutInstancePath = FString::Printf(TEXT("Transient/%s"), *Params.Name);
        UE_LOG(LogTemp, Log, TEXT("Successfully created dynamic material instance: %s"), *OutInstancePath);
        return MID;
    }
    else
    {
        // Create static material instance (editor-time)
        FString PackagePath = Params.Path / Params.Name;
        FString PackageName = FPackageName::ObjectPathToPackageName(PackagePath);

        UPackage* Package = CreatePackage(*PackageName);
        if (!Package)
        {
            OutError = FString::Printf(TEXT("Failed to create package for material instance: %s"), *PackageName);
            return nullptr;
        }

        // Create factory
        UMaterialInstanceConstantFactoryNew* MICFactory = NewObject<UMaterialInstanceConstantFactoryNew>();
        if (!MICFactory)
        {
            OutError = TEXT("Failed to create material instance factory");
            return nullptr;
        }

        // Set parent material
        MICFactory->InitialParent = ParentMaterial;

        // Create the material instance
        UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(MICFactory->FactoryCreateNew(
            UMaterialInstanceConstant::StaticClass(),
            Package,
            *Params.Name,
            RF_Public | RF_Standalone,
            nullptr,
            GWarn
        ));

        if (!MIC)
        {
            OutError = TEXT("Failed to create material instance constant");
            return nullptr;
        }

        Package->MarkPackageDirty();
        FAssetRegistryModule::AssetCreated(MIC);

        OutInstancePath = PackagePath;
        UE_LOG(LogTemp, Log, TEXT("Successfully created material instance constant: %s"), *OutInstancePath);
        return MIC;
    }
}

UMaterialInterface* FMaterialService::FindMaterial(const FString& MaterialPath)
{
    if (MaterialPath.IsEmpty())
    {
        return nullptr;
    }

    // Try loading the material directly
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    if (Material)
    {
        return Material;
    }

    // Try with various suffixes
    TArray<FString> Suffixes = {TEXT(""), TEXT(".Material"), TEXT(".MaterialInstanceConstant")};
    for (const FString& Suffix : Suffixes)
    {
        FString FullPath = MaterialPath + Suffix;
        Material = LoadObject<UMaterialInterface>(nullptr, *FullPath);
        if (Material)
        {
            return Material;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Material not found: %s"), *MaterialPath);
    return nullptr;
}

bool FMaterialService::GetMaterialMetadata(const FString& MaterialPath, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata)
{
    UMaterialInterface* Material = FindMaterial(MaterialPath);
    if (!Material)
    {
        return false;
    }

    OutMetadata = MakeShared<FJsonObject>();
    OutMetadata->SetBoolField(TEXT("success"), true);
    OutMetadata->SetStringField(TEXT("name"), Material->GetName());
    OutMetadata->SetStringField(TEXT("path"), Material->GetPathName());
    OutMetadata->SetStringField(TEXT("type"), GetMaterialTypeString(Material));

    // Check if it's an instance and get parent
    if (UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material))
    {
        if (MaterialInstance->Parent)
        {
            OutMetadata->SetStringField(TEXT("parent_material"), MaterialInstance->Parent->GetPathName());
        }
    }

    // Get base material for properties
    UMaterial* BaseMaterial = Material->GetMaterial();
    if (BaseMaterial)
    {
        OutMetadata->SetStringField(TEXT("blend_mode"), GetBlendModeString(BaseMaterial->BlendMode));
        OutMetadata->SetStringField(TEXT("shading_model"), GetShadingModelString(BaseMaterial->GetShadingModels().GetFirstShadingModel()));
        OutMetadata->SetBoolField(TEXT("is_two_sided"), BaseMaterial->IsTwoSided());
        OutMetadata->SetBoolField(TEXT("is_masked"), BaseMaterial->IsMasked());
    }

    // Add parameter information
    AddParameterInfoToMetadata(Material, OutMetadata);

    return true;
}

void FMaterialService::AddParameterInfoToMetadata(UMaterialInterface* Material, TSharedPtr<FJsonObject>& OutMetadata) const
{
    if (!Material) return;

    // Get scalar parameters
    TArray<FMaterialParameterInfo> ScalarParamInfos;
    TArray<FGuid> ScalarParamIds;
    Material->GetAllScalarParameterInfo(ScalarParamInfos, ScalarParamIds);

    TArray<TSharedPtr<FJsonValue>> ScalarParams;
    for (int32 i = 0; i < ScalarParamInfos.Num(); ++i)
    {
        TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
        ParamObj->SetStringField(TEXT("name"), ScalarParamInfos[i].Name.ToString());

        float Value = 0.0f;
        if (Material->GetScalarParameterValue(ScalarParamInfos[i], Value))
        {
            ParamObj->SetNumberField(TEXT("value"), Value);
        }
        ScalarParams.Add(MakeShared<FJsonValueObject>(ParamObj));
    }
    OutMetadata->SetArrayField(TEXT("scalar_parameters"), ScalarParams);

    // Get vector parameters
    TArray<FMaterialParameterInfo> VectorParamInfos;
    TArray<FGuid> VectorParamIds;
    Material->GetAllVectorParameterInfo(VectorParamInfos, VectorParamIds);

    TArray<TSharedPtr<FJsonValue>> VectorParams;
    for (int32 i = 0; i < VectorParamInfos.Num(); ++i)
    {
        TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
        ParamObj->SetStringField(TEXT("name"), VectorParamInfos[i].Name.ToString());

        FLinearColor Value;
        if (Material->GetVectorParameterValue(VectorParamInfos[i], Value))
        {
            TArray<TSharedPtr<FJsonValue>> ColorArray;
            ColorArray.Add(MakeShared<FJsonValueNumber>(Value.R));
            ColorArray.Add(MakeShared<FJsonValueNumber>(Value.G));
            ColorArray.Add(MakeShared<FJsonValueNumber>(Value.B));
            ColorArray.Add(MakeShared<FJsonValueNumber>(Value.A));
            ParamObj->SetArrayField(TEXT("value"), ColorArray);
        }
        VectorParams.Add(MakeShared<FJsonValueObject>(ParamObj));
    }
    OutMetadata->SetArrayField(TEXT("vector_parameters"), VectorParams);

    // Get texture parameters
    TArray<FMaterialParameterInfo> TextureParamInfos;
    TArray<FGuid> TextureParamIds;
    Material->GetAllTextureParameterInfo(TextureParamInfos, TextureParamIds);

    TArray<TSharedPtr<FJsonValue>> TextureParams;
    for (int32 i = 0; i < TextureParamInfos.Num(); ++i)
    {
        TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
        ParamObj->SetStringField(TEXT("name"), TextureParamInfos[i].Name.ToString());

        UTexture* Texture = nullptr;
        if (Material->GetTextureParameterValue(TextureParamInfos[i], Texture) && Texture)
        {
            ParamObj->SetStringField(TEXT("value"), Texture->GetPathName());
        }
        else
        {
            ParamObj->SetStringField(TEXT("value"), TEXT(""));
        }
        TextureParams.Add(MakeShared<FJsonValueObject>(ParamObj));
    }
    OutMetadata->SetArrayField(TEXT("texture_parameters"), TextureParams);
}

bool FMaterialService::SetScalarParameter(const FString& MaterialPath, const FString& ParameterName, float Value, FString& OutError)
{
    UMaterialInterface* Material = FindMaterial(MaterialPath);
    if (!Material)
    {
        OutError = FString::Printf(TEXT("Material not found: %s"), *MaterialPath);
        return false;
    }

    // Try dynamic instance first
    if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material))
    {
        MID->SetScalarParameterValue(FName(*ParameterName), Value);
        UE_LOG(LogTemp, Log, TEXT("Set scalar parameter '%s' to %f on dynamic instance"), *ParameterName, Value);
        return true;
    }

    // Try static instance
    if (UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(Material))
    {
        MIC->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(*ParameterName), Value);
        MIC->MarkPackageDirty();
        UE_LOG(LogTemp, Log, TEXT("Set scalar parameter '%s' to %f on static instance"), *ParameterName, Value);
        return true;
    }

    OutError = TEXT("Cannot set parameters on base Material. Use a Material Instance instead.");
    return false;
}

bool FMaterialService::SetVectorParameter(const FString& MaterialPath, const FString& ParameterName, const FLinearColor& Value, FString& OutError)
{
    UMaterialInterface* Material = FindMaterial(MaterialPath);
    if (!Material)
    {
        OutError = FString::Printf(TEXT("Material not found: %s"), *MaterialPath);
        return false;
    }

    // Try dynamic instance first
    if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material))
    {
        MID->SetVectorParameterValue(FName(*ParameterName), Value);
        UE_LOG(LogTemp, Log, TEXT("Set vector parameter '%s' on dynamic instance"), *ParameterName);
        return true;
    }

    // Try static instance
    if (UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(Material))
    {
        MIC->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(*ParameterName), Value);
        MIC->MarkPackageDirty();
        UE_LOG(LogTemp, Log, TEXT("Set vector parameter '%s' on static instance"), *ParameterName);
        return true;
    }

    OutError = TEXT("Cannot set parameters on base Material. Use a Material Instance instead.");
    return false;
}

bool FMaterialService::SetTextureParameter(const FString& MaterialPath, const FString& ParameterName, const FString& TexturePath, FString& OutError)
{
    UMaterialInterface* Material = FindMaterial(MaterialPath);
    if (!Material)
    {
        OutError = FString::Printf(TEXT("Material not found: %s"), *MaterialPath);
        return false;
    }

    // Load texture
    UTexture* Texture = LoadObject<UTexture>(nullptr, *TexturePath);
    if (!Texture)
    {
        OutError = FString::Printf(TEXT("Texture not found: %s"), *TexturePath);
        return false;
    }

    // Try dynamic instance first
    if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material))
    {
        MID->SetTextureParameterValue(FName(*ParameterName), Texture);
        UE_LOG(LogTemp, Log, TEXT("Set texture parameter '%s' to '%s' on dynamic instance"), *ParameterName, *TexturePath);
        return true;
    }

    // Try static instance
    if (UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(Material))
    {
        MIC->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(*ParameterName), Texture);
        MIC->MarkPackageDirty();
        UE_LOG(LogTemp, Log, TEXT("Set texture parameter '%s' to '%s' on static instance"), *ParameterName, *TexturePath);
        return true;
    }

    OutError = TEXT("Cannot set parameters on base Material. Use a Material Instance instead.");
    return false;
}

bool FMaterialService::GetScalarParameter(const FString& MaterialPath, const FString& ParameterName, float& OutValue, FString& OutError)
{
    UMaterialInterface* Material = FindMaterial(MaterialPath);
    if (!Material)
    {
        OutError = FString::Printf(TEXT("Material not found: %s"), *MaterialPath);
        return false;
    }

    FMaterialParameterInfo ParamInfo(*ParameterName);
    if (Material->GetScalarParameterValue(ParamInfo, OutValue))
    {
        return true;
    }

    OutError = FString::Printf(TEXT("Scalar parameter '%s' not found"), *ParameterName);
    return false;
}

bool FMaterialService::GetVectorParameter(const FString& MaterialPath, const FString& ParameterName, FLinearColor& OutValue, FString& OutError)
{
    UMaterialInterface* Material = FindMaterial(MaterialPath);
    if (!Material)
    {
        OutError = FString::Printf(TEXT("Material not found: %s"), *MaterialPath);
        return false;
    }

    FMaterialParameterInfo ParamInfo(*ParameterName);
    if (Material->GetVectorParameterValue(ParamInfo, OutValue))
    {
        return true;
    }

    OutError = FString::Printf(TEXT("Vector parameter '%s' not found"), *ParameterName);
    return false;
}

bool FMaterialService::GetTextureParameter(const FString& MaterialPath, const FString& ParameterName, FString& OutTexturePath, FString& OutError)
{
    UMaterialInterface* Material = FindMaterial(MaterialPath);
    if (!Material)
    {
        OutError = FString::Printf(TEXT("Material not found: %s"), *MaterialPath);
        return false;
    }

    FMaterialParameterInfo ParamInfo(*ParameterName);
    UTexture* Texture = nullptr;
    if (Material->GetTextureParameterValue(ParamInfo, Texture))
    {
        OutTexturePath = Texture ? Texture->GetPathName() : TEXT("");
        return true;
    }

    OutError = FString::Printf(TEXT("Texture parameter '%s' not found"), *ParameterName);
    return false;
}

bool FMaterialService::ApplyMaterialToActor(const FString& ActorName, const FString& MaterialPath, int32 SlotIndex, const FString& ComponentName, FString& OutError)
{
    // Find the actor
    AActor* Actor = FindActorByName(ActorName);
    if (!Actor)
    {
        OutError = FString::Printf(TEXT("Actor not found: %s"), *ActorName);
        return false;
    }

    // Find the material
    UMaterialInterface* Material = FindMaterial(MaterialPath);
    if (!Material)
    {
        OutError = FString::Printf(TEXT("Material not found: %s"), *MaterialPath);
        return false;
    }

    // Get mesh component
    UMeshComponent* MeshComp = GetMeshComponent(Actor, ComponentName);
    if (!MeshComp)
    {
        OutError = FString::Printf(TEXT("No mesh component found on actor: %s"), *ActorName);
        return false;
    }

    // Apply material
    MeshComp->SetMaterial(SlotIndex, Material);
    UE_LOG(LogTemp, Log, TEXT("Applied material '%s' to actor '%s' slot %d"), *MaterialPath, *ActorName, SlotIndex);

    return true;
}

AActor* FMaterialService::FindActorByName(const FString& ActorName) const
{
    if (!GEditor || !GEditor->GetEditorWorldContext().World())
    {
        return nullptr;
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->GetName() == ActorName)
        {
            return Actor;
        }
    }

    return nullptr;
}

UMeshComponent* FMaterialService::GetMeshComponent(AActor* Actor, const FString& ComponentName) const
{
    if (!Actor) return nullptr;

    if (!ComponentName.IsEmpty())
    {
        // Find specific component by name
        TArray<UMeshComponent*> MeshComponents;
        Actor->GetComponents<UMeshComponent>(MeshComponents);
        for (UMeshComponent* Comp : MeshComponents)
        {
            if (Comp->GetName() == ComponentName)
            {
                return Comp;
            }
        }
        return nullptr;
    }

    // Return first mesh component found
    if (UStaticMeshComponent* SMC = Actor->FindComponentByClass<UStaticMeshComponent>())
    {
        return SMC;
    }
    if (USkeletalMeshComponent* SKC = Actor->FindComponentByClass<USkeletalMeshComponent>())
    {
        return SKC;
    }

    return nullptr;
}

EBlendMode FMaterialService::GetBlendModeFromString(const FString& BlendModeString) const
{
    if (BlendModeString.Equals(TEXT("Masked"), ESearchCase::IgnoreCase))
        return BLEND_Masked;
    if (BlendModeString.Equals(TEXT("Translucent"), ESearchCase::IgnoreCase))
        return BLEND_Translucent;
    if (BlendModeString.Equals(TEXT("Additive"), ESearchCase::IgnoreCase))
        return BLEND_Additive;
    if (BlendModeString.Equals(TEXT("Modulate"), ESearchCase::IgnoreCase))
        return BLEND_Modulate;
    if (BlendModeString.Equals(TEXT("AlphaComposite"), ESearchCase::IgnoreCase))
        return BLEND_AlphaComposite;
    if (BlendModeString.Equals(TEXT("AlphaHoldout"), ESearchCase::IgnoreCase))
        return BLEND_AlphaHoldout;

    return BLEND_Opaque; // Default
}

EMaterialShadingModel FMaterialService::GetShadingModelFromString(const FString& ShadingModelString) const
{
    if (ShadingModelString.Equals(TEXT("Unlit"), ESearchCase::IgnoreCase))
        return MSM_Unlit;
    if (ShadingModelString.Equals(TEXT("Subsurface"), ESearchCase::IgnoreCase))
        return MSM_Subsurface;
    if (ShadingModelString.Equals(TEXT("PreintegratedSkin"), ESearchCase::IgnoreCase))
        return MSM_PreintegratedSkin;
    if (ShadingModelString.Equals(TEXT("ClearCoat"), ESearchCase::IgnoreCase))
        return MSM_ClearCoat;
    if (ShadingModelString.Equals(TEXT("SubsurfaceProfile"), ESearchCase::IgnoreCase))
        return MSM_SubsurfaceProfile;
    if (ShadingModelString.Equals(TEXT("TwoSidedFoliage"), ESearchCase::IgnoreCase))
        return MSM_TwoSidedFoliage;
    if (ShadingModelString.Equals(TEXT("Hair"), ESearchCase::IgnoreCase))
        return MSM_Hair;
    if (ShadingModelString.Equals(TEXT("Cloth"), ESearchCase::IgnoreCase))
        return MSM_Cloth;
    if (ShadingModelString.Equals(TEXT("Eye"), ESearchCase::IgnoreCase))
        return MSM_Eye;
    if (ShadingModelString.Equals(TEXT("SingleLayerWater"), ESearchCase::IgnoreCase))
        return MSM_SingleLayerWater;
    if (ShadingModelString.Equals(TEXT("ThinTranslucent"), ESearchCase::IgnoreCase))
        return MSM_ThinTranslucent;

    return MSM_DefaultLit; // Default
}

FString FMaterialService::GetBlendModeString(EBlendMode BlendMode) const
{
    switch (BlendMode)
    {
        case BLEND_Opaque: return TEXT("Opaque");
        case BLEND_Masked: return TEXT("Masked");
        case BLEND_Translucent: return TEXT("Translucent");
        case BLEND_Additive: return TEXT("Additive");
        case BLEND_Modulate: return TEXT("Modulate");
        case BLEND_AlphaComposite: return TEXT("AlphaComposite");
        case BLEND_AlphaHoldout: return TEXT("AlphaHoldout");
        default: return TEXT("Unknown");
    }
}

FString FMaterialService::GetShadingModelString(EMaterialShadingModel ShadingModel) const
{
    switch (ShadingModel)
    {
        case MSM_Unlit: return TEXT("Unlit");
        case MSM_DefaultLit: return TEXT("DefaultLit");
        case MSM_Subsurface: return TEXT("Subsurface");
        case MSM_PreintegratedSkin: return TEXT("PreintegratedSkin");
        case MSM_ClearCoat: return TEXT("ClearCoat");
        case MSM_SubsurfaceProfile: return TEXT("SubsurfaceProfile");
        case MSM_TwoSidedFoliage: return TEXT("TwoSidedFoliage");
        case MSM_Hair: return TEXT("Hair");
        case MSM_Cloth: return TEXT("Cloth");
        case MSM_Eye: return TEXT("Eye");
        case MSM_SingleLayerWater: return TEXT("SingleLayerWater");
        case MSM_ThinTranslucent: return TEXT("ThinTranslucent");
        default: return TEXT("Unknown");
    }
}

FString FMaterialService::GetMaterialTypeString(UMaterialInterface* Material) const
{
    if (!Material) return TEXT("Unknown");

    if (Cast<UMaterialInstanceDynamic>(Material))
        return TEXT("MaterialInstanceDynamic");
    if (Cast<UMaterialInstanceConstant>(Material))
        return TEXT("MaterialInstanceConstant");
    if (Cast<UMaterial>(Material))
        return TEXT("Material");

    return TEXT("MaterialInterface");
}

bool FMaterialService::DuplicateMaterialInstance(const FString& SourcePath, const FString& NewName, const FString& FolderPath, FString& OutAssetPath, FString& OutParentMaterial, FString& OutError)
{
    // Find source material instance
    UMaterialInterface* SourceMaterial = FindMaterial(SourcePath);
    if (!SourceMaterial)
    {
        OutError = FString::Printf(TEXT("Source material not found: %s"), *SourcePath);
        return false;
    }

    UMaterialInstanceConstant* SourceMIC = Cast<UMaterialInstanceConstant>(SourceMaterial);
    if (!SourceMIC)
    {
        OutError = FString::Printf(TEXT("Source is not a material instance constant: %s"), *SourcePath);
        return false;
    }

    // Determine destination folder - use source folder if not specified
    FString DestFolder = FolderPath;
    if (DestFolder.IsEmpty())
    {
        DestFolder = FPackageName::GetLongPackagePath(SourceMIC->GetOutermost()->GetName());
    }

    // Build full package path
    FString PackagePath = DestFolder / NewName;
    FString PackageName = FPackageName::ObjectPathToPackageName(PackagePath);

    UE_LOG(LogTemp, Log, TEXT("Duplicating material instance to: %s"), *PackagePath);

    // Create the package
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        OutError = FString::Printf(TEXT("Failed to create package: %s"), *PackageName);
        return false;
    }

    // Duplicate the material instance
    UMaterialInstanceConstant* NewMIC = DuplicateObject<UMaterialInstanceConstant>(SourceMIC, Package, *NewName);
    if (!NewMIC)
    {
        OutError = TEXT("Failed to duplicate material instance");
        return false;
    }

    // Set proper flags
    NewMIC->SetFlags(RF_Public | RF_Standalone);
    NewMIC->ClearFlags(RF_Transient);

    // Mark package dirty and register
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewMIC);

    // Set output values
    OutAssetPath = PackagePath;
    if (NewMIC->Parent)
    {
        OutParentMaterial = NewMIC->Parent->GetPathName();
    }

    UE_LOG(LogTemp, Log, TEXT("Successfully duplicated material instance: %s"), *OutAssetPath);

    return true;
}
