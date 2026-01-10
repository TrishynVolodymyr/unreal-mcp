#include "Commands/Material/SearchMaterialPaletteCommand.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialFunction.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/UObjectIterator.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSearchMaterialPaletteCommand::FSearchMaterialPaletteCommand()
{
}

FString FSearchMaterialPaletteCommand::GetCommandName() const
{
    return TEXT("search_material_palette");
}

bool FSearchMaterialPaletteCommand::ValidateParams(const FString& Parameters) const
{
    // All parameters are optional, so always valid
    return true;
}

FString FSearchMaterialPaletteCommand::Execute(const FString& Parameters)
{
    // Parse parameters
    TSharedPtr<FJsonObject> JsonParams;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonParams) || !JsonParams.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString SearchQuery = JsonParams->HasField(TEXT("search_query"))
        ? JsonParams->GetStringField(TEXT("search_query")) : TEXT("");
    FString CategoryFilter = JsonParams->HasField(TEXT("category_filter"))
        ? JsonParams->GetStringField(TEXT("category_filter")) : TEXT("");
    FString TypeFilter = JsonParams->HasField(TEXT("type_filter"))
        ? JsonParams->GetStringField(TEXT("type_filter")) : TEXT("All");
    int32 MaxResults = JsonParams->HasField(TEXT("max_results"))
        ? (int32)JsonParams->GetNumberField(TEXT("max_results")) : 50;

    TArray<TSharedPtr<FJsonValue>> Results;
    TSet<FString> Categories;
    int32 TotalExpressionCount = 0;
    int32 TotalFunctionCount = 0;

    // Search expressions if type filter allows
    if (TypeFilter.Equals(TEXT("All"), ESearchCase::IgnoreCase) ||
        TypeFilter.Equals(TEXT("Expression"), ESearchCase::IgnoreCase))
    {
        // Iterate all UMaterialExpression-derived classes
        for (TObjectIterator<UClass> It; It; ++It)
        {
            UClass* Class = *It;

            // Skip abstract, deprecated, private classes
            if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
                continue;
            if (Class->HasMetaData(TEXT("Private")))
                continue;
            if (!Class->IsChildOf(UMaterialExpression::StaticClass()))
                continue;

            // Get display name (remove "MaterialExpression" prefix)
            FString DisplayName;
            if (Class->HasMetaData("DisplayName"))
            {
                DisplayName = Class->GetDisplayNameText().ToString();
            }
            else
            {
                DisplayName = Class->GetName();
                static const FString Prefix = TEXT("MaterialExpression");
                if (DisplayName.StartsWith(Prefix))
                {
                    DisplayName = DisplayName.Mid(Prefix.Len());
                }
            }

            // Get categories from default object
            TArray<FString> ItemCategories;
            UMaterialExpression* DefaultObj = Cast<UMaterialExpression>(Class->GetDefaultObject());
            if (DefaultObj)
            {
                for (const FText& Category : DefaultObj->MenuCategories)
                {
                    FString CategoryStr = Category.ToString();
                    ItemCategories.Add(CategoryStr);
                    Categories.Add(CategoryStr);
                }
            }

            // Apply search filter - split query into tokens and match ALL
            if (!SearchQuery.IsEmpty())
            {
                // Split search query by spaces into tokens
                TArray<FString> SearchTokens;
                SearchQuery.ParseIntoArray(SearchTokens, TEXT(" "), true);

                // Check if ALL tokens match somewhere in name, class name, or description
                bool bMatchesAllTokens = true;
                for (const FString& Token : SearchTokens)
                {
                    if (Token.IsEmpty())
                        continue;

                    bool bTokenFound = DisplayName.Contains(Token, ESearchCase::IgnoreCase);
                    if (!bTokenFound)
                    {
                        bTokenFound = Class->GetName().Contains(Token, ESearchCase::IgnoreCase);
                    }
                    if (!bTokenFound && DefaultObj)
                    {
                        FString Description = DefaultObj->GetCreationDescription().ToString();
                        bTokenFound = Description.Contains(Token, ESearchCase::IgnoreCase);
                    }

                    if (!bTokenFound)
                    {
                        bMatchesAllTokens = false;
                        break;
                    }
                }

                if (!bMatchesAllTokens)
                    continue;
            }

            // Apply category filter
            if (!CategoryFilter.IsEmpty())
            {
                bool bMatchesCategory = false;
                for (const FString& Cat : ItemCategories)
                {
                    if (Cat.Contains(CategoryFilter, ESearchCase::IgnoreCase))
                    {
                        bMatchesCategory = true;
                        break;
                    }
                }
                if (!bMatchesCategory)
                    continue;
            }

            TotalExpressionCount++;

            // Skip if we've reached max results
            if (Results.Num() >= MaxResults)
                continue;

            // Build result object
            TSharedPtr<FJsonObject> ItemObj = MakeShared<FJsonObject>();
            ItemObj->SetStringField(TEXT("type"), TEXT("Expression"));
            ItemObj->SetStringField(TEXT("name"), DisplayName);
            ItemObj->SetStringField(TEXT("class_name"), Class->GetName());

            // Add categories
            if (ItemCategories.Num() == 1)
            {
                ItemObj->SetStringField(TEXT("category"), ItemCategories[0]);
            }
            else if (ItemCategories.Num() > 1)
            {
                TArray<TSharedPtr<FJsonValue>> CatArray;
                for (const FString& Cat : ItemCategories)
                {
                    CatArray.Add(MakeShared<FJsonValueString>(Cat));
                }
                ItemObj->SetArrayField(TEXT("category"), CatArray);
            }
            else
            {
                ItemObj->SetStringField(TEXT("category"), TEXT("Uncategorized"));
            }

            // Add description
            if (DefaultObj)
            {
                FString Description = DefaultObj->GetCreationDescription().ToString();
                if (!Description.IsEmpty())
                {
                    ItemObj->SetStringField(TEXT("description"), Description);
                }
            }

            Results.Add(MakeShared<FJsonValueObject>(ItemObj));
        }
    }

    // Search functions if type filter allows
    if (TypeFilter.Equals(TEXT("All"), ESearchCase::IgnoreCase) ||
        TypeFilter.Equals(TEXT("Function"), ESearchCase::IgnoreCase))
    {
        // Use Asset Registry to find MaterialFunction assets
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

        TArray<FAssetData> AssetDataList;
        AssetRegistryModule.Get().GetAssetsByClass(UMaterialFunction::StaticClass()->GetClassPathName(), AssetDataList);

        for (const FAssetData& AssetData : AssetDataList)
        {
            // Only show functions exposed to library
            const bool bExposeToLibrary = AssetData.GetTagValueRef<bool>("bExposeToLibrary");
            if (!bExposeToLibrary)
                continue;

            // Skip transient packages
            if (AssetData.IsAssetLoaded())
            {
                if (AssetData.GetAsset()->GetOutermost() == GetTransientPackage())
                    continue;
            }

            FString FunctionName = AssetData.AssetName.ToString();
            FString FunctionPath = AssetData.GetObjectPathString();

            // Get category from library categories tag
            FString LibraryCategories;
            AssetData.GetTagValue("LibraryCategories", LibraryCategories);

            // Apply search filter - split query into tokens and match ALL
            if (!SearchQuery.IsEmpty())
            {
                // Split search query by spaces into tokens
                TArray<FString> SearchTokens;
                SearchQuery.ParseIntoArray(SearchTokens, TEXT(" "), true);

                // Also get description for search
                FString Description;
                AssetData.GetTagValue("Description", Description);

                // Check if ALL tokens match somewhere in name, path, categories, or description
                bool bMatchesAllTokens = true;
                for (const FString& Token : SearchTokens)
                {
                    if (Token.IsEmpty())
                        continue;

                    bool bTokenFound = FunctionName.Contains(Token, ESearchCase::IgnoreCase);
                    if (!bTokenFound)
                    {
                        bTokenFound = FunctionPath.Contains(Token, ESearchCase::IgnoreCase);
                    }
                    if (!bTokenFound && !LibraryCategories.IsEmpty())
                    {
                        bTokenFound = LibraryCategories.Contains(Token, ESearchCase::IgnoreCase);
                    }
                    if (!bTokenFound && !Description.IsEmpty())
                    {
                        bTokenFound = Description.Contains(Token, ESearchCase::IgnoreCase);
                    }

                    if (!bTokenFound)
                    {
                        bMatchesAllTokens = false;
                        break;
                    }
                }

                if (!bMatchesAllTokens)
                    continue;
            }

            // Apply category filter
            if (!CategoryFilter.IsEmpty())
            {
                if (!LibraryCategories.Contains(CategoryFilter, ESearchCase::IgnoreCase))
                    continue;
            }

            TotalFunctionCount++;

            // Skip if we've reached max results
            if (Results.Num() >= MaxResults)
                continue;

            // Build result object
            TSharedPtr<FJsonObject> ItemObj = MakeShared<FJsonObject>();
            ItemObj->SetStringField(TEXT("type"), TEXT("Function"));
            ItemObj->SetStringField(TEXT("name"), FunctionName);
            ItemObj->SetStringField(TEXT("path"), FunctionPath);

            if (!LibraryCategories.IsEmpty())
            {
                ItemObj->SetStringField(TEXT("category"), LibraryCategories);
            }
            else
            {
                ItemObj->SetStringField(TEXT("category"), TEXT("Uncategorized"));
            }

            // Add description if available
            FString Description;
            if (AssetData.GetTagValue("Description", Description) && !Description.IsEmpty())
            {
                ItemObj->SetStringField(TEXT("description"), Description);
            }

            Results.Add(MakeShared<FJsonValueObject>(ItemObj));
        }
    }

    // Build category list
    TArray<TSharedPtr<FJsonValue>> CategoryArray;
    for (const FString& Category : Categories)
    {
        CategoryArray.Add(MakeShared<FJsonValueString>(Category));
    }

    // Build result
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetArrayField(TEXT("results"), Results);
    ResultObj->SetNumberField(TEXT("total_count"), TotalExpressionCount + TotalFunctionCount);
    ResultObj->SetNumberField(TEXT("returned_count"), Results.Num());
    ResultObj->SetNumberField(TEXT("expression_count"), TotalExpressionCount);
    ResultObj->SetNumberField(TEXT("function_count"), TotalFunctionCount);
    ResultObj->SetArrayField(TEXT("categories"), CategoryArray);
    ResultObj->SetStringField(TEXT("message"),
        FString::Printf(TEXT("Found %d expressions and %d functions (showing %d)"),
            TotalExpressionCount, TotalFunctionCount, Results.Num()));

    return CreateSuccessResponse(ResultObj);
}

FString FSearchMaterialPaletteCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSearchMaterialPaletteCommand::CreateSuccessResponse(TSharedPtr<FJsonObject> ResultObj) const
{
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);

    return OutputString;
}
