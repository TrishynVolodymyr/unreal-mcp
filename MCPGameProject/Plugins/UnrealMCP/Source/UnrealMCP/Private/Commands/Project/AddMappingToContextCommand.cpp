#include "Commands/Project/AddMappingToContextCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "EnhancedInput/Public/InputAction.h"
#include "EnhancedInput/Public/InputTriggers.h"
#include "EditorAssetLibrary.h"
#include "Engine/Engine.h"
#include "AssetToolsModule.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "InputCoreTypes.h"

namespace
{
    // Find-or-create a Digital modifier Input Action (e.g. IA_Mod_Ctrl) in the same package
    // path as the IMC, and ensure its key is mapped in the context so the chord gets driven.
    // Used to back the ctrl/shift/alt/cmd chord flags on add_mapping_to_context.
    UInputAction* FindOrCreateModifierAction(UInputMappingContext* Context,
                                             const FKey& ModKey, const FString& AssetName)
    {
        const FString PkgPath = FPackageName::GetLongPackagePath(Context->GetOutermost()->GetName());
        const FString PackageName = PkgPath / AssetName;

        UInputAction* ModAction = nullptr;
        if (UEditorAssetLibrary::DoesAssetExist(PackageName))
        {
            ModAction = Cast<UInputAction>(UEditorAssetLibrary::LoadAsset(PackageName));
        }
        if (!ModAction)
        {
            FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
            UObject* Created = AssetToolsModule.Get().CreateAsset(AssetName, PkgPath, UInputAction::StaticClass(), nullptr);
            ModAction = Cast<UInputAction>(Created);
            if (ModAction)
            {
                ModAction->ValueType = EInputActionValueType::Boolean;
                ModAction->MarkPackageDirty();
                FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
                ARM.Get().AssetCreated(ModAction);
            }
        }
        if (!ModAction) { return nullptr; }

        bool bMapped = false;
        for (const FEnhancedActionKeyMapping& M : Context->GetMappings())
        {
            if (M.Action == ModAction && M.Key == ModKey) { bMapped = true; break; }
        }
        if (!bMapped) { Context->MapKey(ModAction, ModKey); }
        return ModAction;
    }
}

FAddMappingToContextCommand::FAddMappingToContextCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

FString FAddMappingToContextCommand::GetCommandName() const
{
    return TEXT("add_mapping_to_context");
}

bool FAddMappingToContextCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString ContextPath, ActionPath, Key;
    if (!JsonObject->TryGetStringField(TEXT("context_path"), ContextPath) || ContextPath.IsEmpty())
    {
        return false;
    }
    if (!JsonObject->TryGetStringField(TEXT("action_path"), ActionPath) || ActionPath.IsEmpty())
    {
        return false;
    }
    if (!JsonObject->TryGetStringField(TEXT("key"), Key) || Key.IsEmpty())
    {
        return false;
    }

    return true;
}

FString FAddMappingToContextCommand::Execute(const FString& Parameters)
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid JSON parameters"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Validate parameters
    if (!ValidateParams(Parameters))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString ContextPath = JsonObject->GetStringField(TEXT("context_path"));
    FString ActionPath = JsonObject->GetStringField(TEXT("action_path"));
    FString Key = JsonObject->GetStringField(TEXT("key"));

    // Optional modifiers
    bool bShift = false;
    bool bCtrl = false;
    bool bAlt = false;
    bool bCmd = false;
    JsonObject->TryGetBoolField(TEXT("shift"), bShift);
    JsonObject->TryGetBoolField(TEXT("ctrl"), bCtrl);
    JsonObject->TryGetBoolField(TEXT("alt"), bAlt);
    JsonObject->TryGetBoolField(TEXT("cmd"), bCmd);

    // Load the Input Mapping Context
    if (!UEditorAssetLibrary::DoesAssetExist(ContextPath))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Input Mapping Context does not exist: %s"), *ContextPath));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    UObject* ContextAsset = UEditorAssetLibrary::LoadAsset(ContextPath);
    UInputMappingContext* Context = Cast<UInputMappingContext>(ContextAsset);
    if (!Context)
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to load Input Mapping Context"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Load the Input Action
    if (!UEditorAssetLibrary::DoesAssetExist(ActionPath))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Input Action does not exist: %s"), *ActionPath));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    UObject* ActionAsset = UEditorAssetLibrary::LoadAsset(ActionPath);
    UInputAction* Action = Cast<UInputAction>(ActionAsset);
    if (!Action)
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to load Input Action"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Create the key mapping
    FKey InputKey(*Key);
    if (!InputKey.IsValid())
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid key: %s"), *Key));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Optional: chord-blocker mode adds a ChordBlocker to an EXISTING (action,key) mapping
    // instead of creating a new mapping — prevents a bare-key action firing while the chord
    // (e.g. Ctrl) is held.
    bool bChordBlocker = false;
    JsonObject->TryGetBoolField(TEXT("chord_blocker"), bChordBlocker);

    // Resolve modifier IAs FIRST. FindOrCreateModifierAction may call Context->MapKey (which
    // mutates the Mappings array), so do this BEFORE taking any FEnhancedActionKeyMapping&
    // reference below — otherwise that reference could dangle on reallocation.
    struct FModSpec { bool bWanted; FKey Key; const TCHAR* Name; };
    const FModSpec ModSpecs[] = {
        { bCtrl,  EKeys::LeftControl, TEXT("IA_Mod_Ctrl")  },
        { bShift, EKeys::LeftShift,   TEXT("IA_Mod_Shift") },
        { bAlt,   EKeys::LeftAlt,     TEXT("IA_Mod_Alt")   },
        { bCmd,   EKeys::LeftCommand, TEXT("IA_Mod_Cmd")   },
    };
    TArray<UInputAction*> ModActions;
    for (const FModSpec& M : ModSpecs)
    {
        if (M.bWanted)
        {
            if (UInputAction* Mod = FindOrCreateModifierAction(Context, M.Key, M.Name))
            {
                ModActions.Add(Mod);
            }
        }
    }

    if (bChordBlocker)
    {
        // Add ChordBlocker trigger(s) to the EXISTING (Action, Key) mapping, in place.
        TArray<FEnhancedActionKeyMapping>& Mappings =
            const_cast<TArray<FEnhancedActionKeyMapping>&>(Context->GetMappings());
        FEnhancedActionKeyMapping* Existing = Mappings.FindByPredicate(
            [&](const FEnhancedActionKeyMapping& M){ return M.Action == Action && M.Key == InputKey; });
        if (!Existing) { Existing = &Context->MapKey(Action, InputKey); }
        for (UInputAction* Mod : ModActions)
        {
            // Outer MUST be the Context: Triggers is an Instanced array owned by the IMC.
            // Using the Action as Outer puts the trigger in the IA's package and makes the
            // IMC's cross-package reference "private" → IMC fails to save.
            UInputTriggerChordBlocker* Blocker = NewObject<UInputTriggerChordBlocker>(Context);
            Blocker->ChordAction = Mod;
            Existing->Triggers.Add(Blocker);
        }
    }
    else
    {
        // Create the mapping + attach a ChordAction trigger per requested modifier. A ChordAction
        // is an implicit trigger (Enhanced Input auto-orders chord evaluation via
        // FDependentChordTracker), so the key fires only while the modifier IA is also held.
        FEnhancedActionKeyMapping& NewMapping = Context->MapKey(Action, InputKey);
        for (UInputAction* Mod : ModActions)
        {
            // Outer MUST be the Context (Instanced Triggers array is owned by the IMC) — see
            // the blocker branch above: Action-as-Outer breaks IMC save (private cross-pkg ref).
            UInputTriggerChordAction* Chord = NewObject<UInputTriggerChordAction>(Context);
            Chord->ChordAction = Mod;
            NewMapping.Triggers.Add(Chord);
        }
    }

    // Mark the context dirty + PERSIST to disk. Previously this was MarkPackageDirty-only, so the
    // new mapping was lost on the next editor restart (the IMC reverted) — fatal when the caller
    // adds a binding then rebuilds. SaveLoadedAsset writes the .uasset now.
    Context->MarkPackageDirty();
    const bool bSaved = UEditorAssetLibrary::SaveLoadedAsset(Context, /*bOnlyIfIsDirty*/ false);

    // Create success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetBoolField(TEXT("saved"), bSaved);
    ResponseObj->SetStringField(TEXT("context_path"), ContextPath);
    ResponseObj->SetStringField(TEXT("action_path"), ActionPath);
    ResponseObj->SetStringField(TEXT("key"), Key);
    ResponseObj->SetBoolField(TEXT("shift"), bShift);
    ResponseObj->SetBoolField(TEXT("ctrl"), bCtrl);
    ResponseObj->SetBoolField(TEXT("alt"), bAlt);
    ResponseObj->SetBoolField(TEXT("cmd"), bCmd);

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

