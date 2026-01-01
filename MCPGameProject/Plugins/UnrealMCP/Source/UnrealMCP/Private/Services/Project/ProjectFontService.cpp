#include "Services/Project/ProjectFontService.h"
#include "EditorAssetLibrary.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FProjectFontService& FProjectFontService::Get()
{
    static FProjectFontService Instance;
    return Instance;
}

bool FProjectFontService::CreateFontFace(const FString& FontName, const FString& Path, const FString& SourceTexturePath, bool bUseSDF, int32 DistanceFieldSpread, const TSharedPtr<FJsonObject>& FontMetrics, FString& OutAssetPath, FString& OutError)
{
    // Ensure the path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(Path))
    {
        if (!UEditorAssetLibrary::MakeDirectory(Path))
        {
            OutError = FString::Printf(TEXT("Failed to create directory: %s"), *Path);
            return false;
        }
    }

    // Build the asset path
    FString AssetName = FontName;
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + AssetName;
    OutAssetPath = PackageName;

    // Check if the font face already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        OutError = FString::Printf(TEXT("Font face already exists: %s"), *PackageName);
        return false;
    }

    // Load the source texture if provided
    UTexture2D* SourceTexture = nullptr;
    if (!SourceTexturePath.IsEmpty())
    {
        FString NormalizedTexturePath = SourceTexturePath;
        if (!NormalizedTexturePath.Contains(TEXT(".")))
        {
            FString TextureAssetName = FPaths::GetBaseFilename(SourceTexturePath);
            NormalizedTexturePath = SourceTexturePath + TEXT(".") + TextureAssetName;
        }

        UObject* LoadedAsset = StaticLoadObject(UTexture2D::StaticClass(), nullptr, *NormalizedTexturePath);
        SourceTexture = Cast<UTexture2D>(LoadedAsset);
        if (!SourceTexture)
        {
            OutError = FString::Printf(TEXT("Failed to load source texture: %s"), *SourceTexturePath);
            return false;
        }
    }

    // Create the font face asset
    FString PackagePathForCreate = PackagePath.LeftChop(1); // Remove trailing slash
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        OutError = FString::Printf(TEXT("Failed to create package for font face: %s"), *PackageName);
        return false;
    }

    // Create the FontFace object
    UFontFace* NewFontFace = NewObject<UFontFace>(Package, FName(*AssetName), RF_Public | RF_Standalone);
    if (!NewFontFace)
    {
        OutError = TEXT("Failed to create font face object");
        return false;
    }

    // Configure SDF settings
    if (bUseSDF)
    {
        NewFontFace->Hinting = EFontHinting::None; // SDF fonts don't use hinting
        NewFontFace->LoadingPolicy = EFontLoadingPolicy::LazyLoad;
    }

    // Apply font metrics if provided
    if (FontMetrics.IsValid())
    {
        double Ascender = 0;
        if (FontMetrics->TryGetNumberField(TEXT("ascender"), Ascender))
        {
            NewFontFace->bIsAscendOverridden = true;
            NewFontFace->AscendOverriddenValue = static_cast<int32>(Ascender);
        }

        double Descender = 0;
        if (FontMetrics->TryGetNumberField(TEXT("descender"), Descender))
        {
            NewFontFace->bIsDescendOverridden = true;
            NewFontFace->DescendOverriddenValue = static_cast<int32>(Descender);
        }
    }

    // Mark the asset as modified
    NewFontFace->MarkPackageDirty();
    Package->MarkPackageDirty();

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewFontFace);

    // Save the asset
    UEditorAssetLibrary::SaveAsset(PackageName, false);

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully created font face '%s' at '%s'"), *FontName, *OutAssetPath);

    return true;
}

bool FProjectFontService::ImportTTFFont(const FString& FontName, const FString& Path, const FString& TTFFilePath, const TSharedPtr<FJsonObject>& FontMetrics, FString& OutAssetPath, FString& OutError)
{
    // Validate the TTF file exists
    if (!FPaths::FileExists(TTFFilePath))
    {
        OutError = FString::Printf(TEXT("TTF file not found: %s"), *TTFFilePath);
        return false;
    }

    // Ensure the output path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(Path))
    {
        if (!UEditorAssetLibrary::MakeDirectory(Path))
        {
            OutError = FString::Printf(TEXT("Failed to create directory: %s"), *Path);
            return false;
        }
    }

    // Build the asset paths
    FString AssetName = FontName;
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString FontPackageName = PackagePath + AssetName;
    FString FontFacePackageName = PackagePath + AssetName + TEXT("_Face");
    OutAssetPath = FontPackageName;

    // Check if the font already exists
    if (UEditorAssetLibrary::DoesAssetExist(FontPackageName))
    {
        OutError = FString::Printf(TEXT("Font already exists: %s"), *FontPackageName);
        return false;
    }

    // Read the TTF file into memory
    TArray<uint8> FontData;
    if (!FFileHelper::LoadFileToArray(FontData, *TTFFilePath))
    {
        OutError = FString::Printf(TEXT("Failed to read TTF file: %s"), *TTFFilePath);
        return false;
    }

    // Step 1: Create the FontFace asset (holds the raw TTF data)
    UPackage* FontFacePackage = CreatePackage(*FontFacePackageName);
    if (!FontFacePackage)
    {
        OutError = FString::Printf(TEXT("Failed to create package for font face: %s"), *FontFacePackageName);
        return false;
    }

    FString FontFaceAssetName = AssetName + TEXT("_Face");
    UFontFace* NewFontFace = NewObject<UFontFace>(FontFacePackage, FName(*FontFaceAssetName), RF_Public | RF_Standalone);
    if (!NewFontFace)
    {
        OutError = TEXT("Failed to create font face object");
        return false;
    }

    // Set the source file path
    NewFontFace->SourceFilename = TTFFilePath;

    // Load the font data into the FontFace
    NewFontFace->FontFaceData = MakeShared<FFontFaceData>();
    NewFontFace->FontFaceData->SetData(MoveTemp(FontData));

    // Set properties for TTF fonts - use Inline loading since we embedded the data
    NewFontFace->Hinting = EFontHinting::Default;
    NewFontFace->LoadingPolicy = EFontLoadingPolicy::Inline;

    // Apply font metrics if provided
    if (FontMetrics.IsValid())
    {
        double Ascender = 0;
        if (FontMetrics->TryGetNumberField(TEXT("ascender"), Ascender))
        {
            NewFontFace->bIsAscendOverridden = true;
            NewFontFace->AscendOverriddenValue = static_cast<int32>(Ascender);
        }

        double Descender = 0;
        if (FontMetrics->TryGetNumberField(TEXT("descender"), Descender))
        {
            NewFontFace->bIsDescendOverridden = true;
            NewFontFace->DescendOverriddenValue = static_cast<int32>(Descender);
        }
    }

    // Save the FontFace
    NewFontFace->MarkPackageDirty();
    FontFacePackage->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewFontFace);
    UEditorAssetLibrary::SaveAsset(FontFacePackageName, false);

    // Step 2: Create the UFont (composite font) that UMG can use
    UPackage* FontPackage = CreatePackage(*FontPackageName);
    if (!FontPackage)
    {
        OutError = FString::Printf(TEXT("Failed to create package for font: %s"), *FontPackageName);
        return false;
    }

    UFont* NewFont = NewObject<UFont>(FontPackage, FName(*AssetName), RF_Public | RF_Standalone);
    if (!NewFont)
    {
        OutError = TEXT("Failed to create font object");
        return false;
    }

    // Configure as a Runtime font (renders TTF on-demand)
    NewFont->FontCacheType = EFontCacheType::Runtime;

    // Set up the CompositeFont to reference our FontFace
    FCompositeFont& CompositeFont = NewFont->GetMutableInternalCompositeFont();

    // Clear and set up the default typeface
    CompositeFont.DefaultTypeface.Fonts.Empty();

    // Add our font as the "Regular" style in the default typeface
    FTypefaceEntry& TypefaceEntry = CompositeFont.DefaultTypeface.Fonts.AddDefaulted_GetRef();
    TypefaceEntry.Name = TEXT("Regular");
    TypefaceEntry.Font = FFontData(NewFontFace);

    // Save the Font
    NewFont->MarkPackageDirty();
    FontPackage->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewFont);
    UEditorAssetLibrary::SaveAsset(FontPackageName, false);

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully imported TTF font '%s' from '%s' (FontFace: %s, Font: %s)"),
        *FontName, *TTFFilePath, *FontFacePackageName, *OutAssetPath);

    return true;
}

bool FProjectFontService::SetFontFaceProperties(const FString& FontPath, const TSharedPtr<FJsonObject>& Properties, TArray<FString>& OutSuccessProperties, TArray<FString>& OutFailedProperties, FString& OutError)
{
    OutSuccessProperties.Empty();
    OutFailedProperties.Empty();

    // Normalize the path
    FString NormalizedPath = FontPath;
    if (!NormalizedPath.Contains(TEXT(".")))
    {
        FString AssetName = FPaths::GetBaseFilename(FontPath);
        NormalizedPath = FontPath + TEXT(".") + AssetName;
    }

    // Load the font face
    UObject* LoadedAsset = StaticLoadObject(UFontFace::StaticClass(), nullptr, *NormalizedPath);
    UFontFace* FontFace = Cast<UFontFace>(LoadedAsset);
    if (!FontFace)
    {
        OutError = FString::Printf(TEXT("Failed to load font face: %s"), *FontPath);
        return false;
    }

    // Apply properties
    if (!Properties.IsValid())
    {
        OutError = TEXT("No properties provided");
        return false;
    }

    // Handle Hinting property
    FString HintingStr;
    if (Properties->TryGetStringField(TEXT("Hinting"), HintingStr))
    {
        if (HintingStr == TEXT("None"))
        {
            FontFace->Hinting = EFontHinting::None;
            OutSuccessProperties.Add(TEXT("Hinting"));
        }
        else if (HintingStr == TEXT("Auto"))
        {
            FontFace->Hinting = EFontHinting::Auto;
            OutSuccessProperties.Add(TEXT("Hinting"));
        }
        else if (HintingStr == TEXT("AutoLight"))
        {
            FontFace->Hinting = EFontHinting::AutoLight;
            OutSuccessProperties.Add(TEXT("Hinting"));
        }
        else
        {
            OutFailedProperties.Add(FString::Printf(TEXT("Hinting_InvalidValue_%s"), *HintingStr));
        }
    }

    // Handle LoadingPolicy property
    FString LoadingPolicyStr;
    if (Properties->TryGetStringField(TEXT("LoadingPolicy"), LoadingPolicyStr))
    {
        if (LoadingPolicyStr == TEXT("LazyLoad"))
        {
            FontFace->LoadingPolicy = EFontLoadingPolicy::LazyLoad;
            OutSuccessProperties.Add(TEXT("LoadingPolicy"));
        }
        else if (LoadingPolicyStr == TEXT("Stream"))
        {
            FontFace->LoadingPolicy = EFontLoadingPolicy::Stream;
            OutSuccessProperties.Add(TEXT("LoadingPolicy"));
        }
        else
        {
            OutFailedProperties.Add(FString::Printf(TEXT("LoadingPolicy_InvalidValue_%s"), *LoadingPolicyStr));
        }
    }

    // Handle Ascender override
    double Ascender = 0;
    if (Properties->TryGetNumberField(TEXT("Ascender"), Ascender))
    {
        FontFace->bIsAscendOverridden = true;
        FontFace->AscendOverriddenValue = static_cast<int32>(Ascender);
        OutSuccessProperties.Add(TEXT("Ascender"));
    }

    // Handle Descender override
    double Descender = 0;
    if (Properties->TryGetNumberField(TEXT("Descender"), Descender))
    {
        FontFace->bIsDescendOverridden = true;
        FontFace->DescendOverriddenValue = static_cast<int32>(Descender);
        OutSuccessProperties.Add(TEXT("Descender"));
    }

    // Handle StrikeBrushHeightPercentage
    double StrikeHeight = 0;
    if (Properties->TryGetNumberField(TEXT("StrikeBrushHeightPercentage"), StrikeHeight))
    {
        FontFace->StrikeBrushHeightPercentage = static_cast<int32>(StrikeHeight);
        OutSuccessProperties.Add(TEXT("StrikeBrushHeightPercentage"));
    }

    // Mark as dirty and save
    FontFace->Modify();
    FontFace->MarkPackageDirty();

    return OutSuccessProperties.Num() > 0 || OutFailedProperties.Num() == 0;
}

TSharedPtr<FJsonObject> FProjectFontService::GetFontFaceMetadata(const FString& FontPath, FString& OutError)
{
    // Normalize the path
    FString NormalizedPath = FontPath;
    if (!NormalizedPath.Contains(TEXT(".")))
    {
        FString AssetName = FPaths::GetBaseFilename(FontPath);
        NormalizedPath = FontPath + TEXT(".") + AssetName;
    }

    // Load the font face
    UObject* LoadedAsset = StaticLoadObject(UFontFace::StaticClass(), nullptr, *NormalizedPath);
    UFontFace* FontFace = Cast<UFontFace>(LoadedAsset);
    if (!FontFace)
    {
        OutError = FString::Printf(TEXT("Failed to load font face: %s"), *FontPath);
        return nullptr;
    }

    TSharedPtr<FJsonObject> Metadata = MakeShared<FJsonObject>();
    Metadata->SetBoolField(TEXT("success"), true);
    Metadata->SetStringField(TEXT("font_path"), FontPath);
    Metadata->SetStringField(TEXT("font_name"), FontFace->GetName());

    // Hinting
    FString HintingStr;
    switch (FontFace->Hinting)
    {
        case EFontHinting::None: HintingStr = TEXT("None"); break;
        case EFontHinting::Auto: HintingStr = TEXT("Auto"); break;
        case EFontHinting::AutoLight: HintingStr = TEXT("AutoLight"); break;
        default: HintingStr = TEXT("Default"); break;
    }
    Metadata->SetStringField(TEXT("hinting"), HintingStr);

    // Loading Policy
    FString LoadingPolicyStr;
    switch (FontFace->LoadingPolicy)
    {
        case EFontLoadingPolicy::LazyLoad: LoadingPolicyStr = TEXT("LazyLoad"); break;
        case EFontLoadingPolicy::Stream: LoadingPolicyStr = TEXT("Stream"); break;
        default: LoadingPolicyStr = TEXT("LazyLoad"); break;
    }
    Metadata->SetStringField(TEXT("loading_policy"), LoadingPolicyStr);

    // Source filename
    Metadata->SetStringField(TEXT("source_filename"), FontFace->SourceFilename);

    // Metrics
    Metadata->SetBoolField(TEXT("ascender_override_set"), FontFace->bIsAscendOverridden);
    Metadata->SetNumberField(TEXT("ascender"), FontFace->AscendOverriddenValue);
    Metadata->SetBoolField(TEXT("descender_override_set"), FontFace->bIsDescendOverridden);
    Metadata->SetNumberField(TEXT("descender"), FontFace->DescendOverriddenValue);
    Metadata->SetNumberField(TEXT("strike_brush_height_percentage"), FontFace->StrikeBrushHeightPercentage);

    return Metadata;
}

bool FProjectFontService::CreateOfflineFont(const FString& FontName, const FString& Path, const FString& TexturePath, const FString& MetricsFilePath, FString& OutAssetPath, FString& OutError)
{
    // Load metrics JSON from file
    if (MetricsFilePath.IsEmpty())
    {
        OutError = TEXT("Metrics file path is required for offline font creation");
        return false;
    }

    // Check if file exists
    if (!FPaths::FileExists(MetricsFilePath))
    {
        OutError = FString::Printf(TEXT("Metrics file not found: %s"), *MetricsFilePath);
        return false;
    }

    // Read the file content
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *MetricsFilePath))
    {
        OutError = FString::Printf(TEXT("Failed to read metrics file: %s"), *MetricsFilePath);
        return false;
    }

    // Parse the JSON
    TSharedPtr<FJsonObject> MetricsJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    if (!FJsonSerializer::Deserialize(Reader, MetricsJson) || !MetricsJson.IsValid())
    {
        OutError = FString::Printf(TEXT("Failed to parse metrics JSON from file: %s"), *MetricsFilePath);
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Loaded metrics from file: %s"), *MetricsFilePath);

    // Ensure the path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(Path))
    {
        if (!UEditorAssetLibrary::MakeDirectory(Path))
        {
            OutError = FString::Printf(TEXT("Failed to create directory: %s"), *Path);
            return false;
        }
    }

    // Build the asset path
    FString AssetName = FontName;
    FString PackagePath = Path;
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }
    FString PackageName = PackagePath + AssetName;
    OutAssetPath = PackageName;

    // Check if the font already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        OutError = FString::Printf(TEXT("Font already exists: %s"), *PackageName);
        return false;
    }

    // Load the texture
    FString NormalizedTexturePath = TexturePath;
    if (!NormalizedTexturePath.Contains(TEXT(".")))
    {
        FString TextureAssetName = FPaths::GetBaseFilename(TexturePath);
        NormalizedTexturePath = TexturePath + TEXT(".") + TextureAssetName;
    }

    UObject* LoadedTexture = StaticLoadObject(UTexture2D::StaticClass(), nullptr, *NormalizedTexturePath);
    UTexture2D* FontTexture = Cast<UTexture2D>(LoadedTexture);
    if (!FontTexture)
    {
        OutError = FString::Printf(TEXT("Failed to load texture: %s"), *TexturePath);
        return false;
    }

    // Create a package for the new asset
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        OutError = FString::Printf(TEXT("Failed to create package for font: %s"), *PackageName);
        return false;
    }

    // Create the UFont object
    UFont* NewFont = NewObject<UFont>(Package, FName(*AssetName), RF_Public | RF_Standalone);
    if (!NewFont)
    {
        OutError = TEXT("Failed to create font object");
        return false;
    }

    // Set to offline caching mode
    NewFont->FontCacheType = EFontCacheType::Offline;

    // Add the texture to the font
    NewFont->Textures.Add(FontTexture);

    // Extract metrics from JSON
    int32 AtlasWidth = MetricsJson->GetIntegerField(TEXT("atlasWidth"));
    int32 AtlasHeight = MetricsJson->GetIntegerField(TEXT("atlasHeight"));
    int32 LineHeight = MetricsJson->GetIntegerField(TEXT("lineHeight"));
    int32 Baseline = MetricsJson->GetIntegerField(TEXT("baseline"));

    // Set font metrics
    NewFont->EmScale = 1.0f;
    NewFont->Ascent = static_cast<float>(Baseline);
    NewFont->Descent = static_cast<float>(LineHeight - Baseline);
    NewFont->Leading = 0.0f;
    NewFont->Kerning = 0;
    NewFont->ScalingFactor = 1.0f;
    NewFont->LegacyFontSize = LineHeight;

    // Process characters from JSON
    const TSharedPtr<FJsonObject>* CharactersObj;
    if (MetricsJson->TryGetObjectField(TEXT("characters"), CharactersObj))
    {
        TMap<FString, TSharedPtr<FJsonValue>> CharMap = (*CharactersObj)->Values;

        // Initialize character remap
        NewFont->IsRemapped = 1;

        for (const auto& CharPair : CharMap)
        {
            int32 CharCode = FCString::Atoi(*CharPair.Key);
            const TSharedPtr<FJsonObject>& CharData = CharPair.Value->AsObject();

            if (!CharData.IsValid())
            {
                continue;
            }

            // Get UV coordinates (normalized 0-1)
            double U = CharData->GetNumberField(TEXT("u"));
            double V = CharData->GetNumberField(TEXT("v"));
            int32 Width = CharData->GetIntegerField(TEXT("width"));
            int32 Height = CharData->GetIntegerField(TEXT("height"));
            int32 YOffset = CharData->GetIntegerField(TEXT("yOffset"));

            // Convert normalized UVs to pixel coordinates
            int32 StartU = FMath::RoundToInt(U * AtlasWidth);
            int32 StartV = FMath::RoundToInt(V * AtlasHeight);

            // Create the font character
            FFontCharacter FontChar;
            FontChar.StartU = StartU;
            FontChar.StartV = StartV;
            FontChar.USize = Width;
            FontChar.VSize = Height;
            FontChar.TextureIndex = 0; // First (and only) texture
            FontChar.VerticalOffset = YOffset;

            // Add to characters array
            int32 CharIndex = NewFont->Characters.Add(FontChar);

            // Add to remap table
            NewFont->CharRemap.Add(static_cast<uint16>(CharCode), static_cast<uint16>(CharIndex));
        }
    }

    // Cache character count and max height
    NewFont->CacheCharacterCountAndMaxCharHeight();

    // Mark the asset as modified
    NewFont->MarkPackageDirty();
    Package->MarkPackageDirty();

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewFont);

    // Save the asset
    UEditorAssetLibrary::SaveAsset(PackageName, false);

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully created offline font '%s' at '%s' with %d characters"),
        *FontName, *OutAssetPath, NewFont->Characters.Num());

    return true;
}

TSharedPtr<FJsonObject> FProjectFontService::GetFontMetadata(const FString& FontPath, FString& OutError)
{
    // Normalize the path
    FString NormalizedPath = FontPath;
    if (!NormalizedPath.Contains(TEXT(".")))
    {
        FString AssetName = FPaths::GetBaseFilename(FontPath);
        NormalizedPath = FontPath + TEXT(".") + AssetName;
    }

    // Load the font
    UObject* LoadedAsset = StaticLoadObject(UFont::StaticClass(), nullptr, *NormalizedPath);
    UFont* Font = Cast<UFont>(LoadedAsset);
    if (!Font)
    {
        OutError = FString::Printf(TEXT("Failed to load font: %s"), *FontPath);
        return nullptr;
    }

    TSharedPtr<FJsonObject> Metadata = MakeShared<FJsonObject>();
    Metadata->SetBoolField(TEXT("success"), true);
    Metadata->SetStringField(TEXT("font_path"), FontPath);
    Metadata->SetStringField(TEXT("font_name"), Font->GetName());

    // Font cache type
    FString CacheTypeStr = (Font->FontCacheType == EFontCacheType::Offline) ? TEXT("Offline") : TEXT("Runtime");
    Metadata->SetStringField(TEXT("cache_type"), CacheTypeStr);

    // Metrics
    Metadata->SetNumberField(TEXT("em_scale"), Font->EmScale);
    Metadata->SetNumberField(TEXT("ascent"), Font->Ascent);
    Metadata->SetNumberField(TEXT("descent"), Font->Descent);
    Metadata->SetNumberField(TEXT("leading"), Font->Leading);
    Metadata->SetNumberField(TEXT("kerning"), Font->Kerning);
    Metadata->SetNumberField(TEXT("scaling_factor"), Font->ScalingFactor);
    Metadata->SetNumberField(TEXT("legacy_font_size"), Font->LegacyFontSize);

    // Character count
    Metadata->SetNumberField(TEXT("character_count"), Font->Characters.Num());
    Metadata->SetNumberField(TEXT("texture_count"), Font->Textures.Num());
    Metadata->SetBoolField(TEXT("is_remapped"), Font->IsRemapped != 0);

    return Metadata;
}
