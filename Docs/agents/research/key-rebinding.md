# Research: Key Rebinding in UE 5.7 Enhanced Input

## Summary

UE 5.7 has a **complete, built-in key rebinding system** via `UEnhancedInputUserSettings`. The old `UPlayerMappableInputConfig` is deprecated since UE 5.3. The modern system provides: mapping/unmapping keys, multiple key profiles, built-in SaveGame persistence, conflict detection via `QueryMapKeyInActiveContextSet`, and per-action mappability configuration via `UPlayerMappableKeySettings`. This is the standard approach and should be used directly -- no custom rebinding logic needed.

## Architecture Overview

```
UEnhancedInputDeveloperSettings (Project Settings)
  ├── bEnableUserSettings = true           ← MUST enable this
  ├── UserSettingsClass                    ← UEnhancedInputUserSettings or subclass
  ├── DefaultPlayerMappableKeyProfileClass ← UEnhancedPlayerMappableKeyProfile or subclass
  └── InputSettingsSaveSlotName            ← "EnhancedInputUserSettings" default

UEnhancedInputLocalPlayerSubsystem
  ├── GetUserSettings() → UEnhancedInputUserSettings
  │     ├── MapPlayerKey(Args, FailureReason)
  │     ├── UnMapPlayerKey(Args, FailureReason)
  │     ├── SaveSettings() / AsyncSaveSettings()
  │     ├── GetActiveKeyProfile() → UEnhancedPlayerMappableKeyProfile
  │     │     ├── GetPlayerMappingRows() → TMap<FName, FKeyMappingRow>
  │     │     ├── QueryPlayerMappedKeys(Options, OutKeys)
  │     │     ├── GetMappedKeysInRow(MappingName, OutKeys)
  │     │     ├── GetMappingNamesForKey(Key, OutMappingNames) ← reverse lookup
  │     │     └── ResetToDefault()
  │     ├── RegisterInputMappingContext(IMC)
  │     ├── FindMappingsInRow(MappingName) → TSet<FPlayerKeyMapping>
  │     └── OnSettingsChanged delegate
  └── QueryMapKeyInActiveContextSet(...)   ← CONFLICT DETECTION
```

## Step-by-Step: How to Implement Key Rebinding

### 1. Enable User Settings (Project Settings)

In Enhanced Input Developer Settings (`Edit > Project Settings > Enhanced Input`):
- **Enable User Settings** = true (`bEnableUserSettings`)
- **User Settings Class** = `UEnhancedInputUserSettings` (or custom subclass)
- **Default Player Mappable Key Profile Class** = `UEnhancedPlayerMappableKeyProfile`

### 2. Mark Input Actions as Player-Mappable

On each `UInputAction` asset that should be rebindable:
- Add a `UPlayerMappableKeySettings` object in the "User Settings" category
- Set `Name` (FName) -- this is the unique mapping identifier used throughout the system
- Set `DisplayName` (FText) -- localized name shown in UI (e.g., "Jump", "Sprint")
- Set `DisplayCategory` (FText) -- for grouping in settings UI (e.g., "Movement", "Combat")

Alternatively, on individual `FEnhancedActionKeyMapping` entries in an IMC:
- Set `SettingBehavior` to `OverrideSettings` to override per-mapping
- Or leave as `InheritSettingsFromAction` (default) to use the InputAction's settings

### 3. Register IMCs with User Settings

When adding mapping contexts, use `bNotifyUserSettings = true`:
```cpp
FModifyContextOptions Options;
Options.bNotifyUserSettings = true;
Subsystem->AddMappingContext(IMC, Priority, Options);
```

Or manually register:
```cpp
UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings();
Settings->RegisterInputMappingContext(IMC);
```

### 4. Query Current Key Bindings (for UI display)

```cpp
UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings();
UEnhancedPlayerMappableKeyProfile* Profile = Settings->GetActiveKeyProfile();

// Get all mappings organized by mapping name
const TMap<FName, FKeyMappingRow>& AllMappings = Profile->GetPlayerMappingRows();

// For a specific mapping, get current keys
TArray<FKey> Keys;
Profile->GetMappedKeysInRow(MappingName, Keys);

// Or get a specific slot's mapping
const FPlayerKeyMapping* Mapping = Settings->FindCurrentMappingForSlot(MappingName, EPlayerMappableKeySlot::First);
FKey CurrentKey = Mapping->GetCurrentKey();
FKey DefaultKey = Mapping->GetDefaultKey();
bool bIsCustomized = Mapping->IsCustomized();
```

### 5. Remap a Key (MapPlayerKey)

```cpp
FMapPlayerKeyArgs Args;
Args.MappingName = TEXT("Jump");                    // The mapping name from PlayerMappableKeySettings
Args.Slot = EPlayerMappableKeySlot::First;          // Which slot (supports multiple keys per action)
Args.NewKey = EKeys::SpaceBar;                      // The new key
Args.bCreateMatchingSlotIfNeeded = true;            // Create slot if doesn't exist

FGameplayTagContainer FailureReason;
Settings->MapPlayerKey(Args, FailureReason);

if (!FailureReason.IsEmpty())
{
    // Handle failure
}

// Save after remapping
Settings->SaveSettings();
```

### 6. Unmap a Key (UnMapPlayerKey)

```cpp
FMapPlayerKeyArgs Args;
Args.MappingName = TEXT("Jump");
Args.Slot = EPlayerMappableKeySlot::First;

FGameplayTagContainer FailureReason;
Settings->UnMapPlayerKey(Args, FailureReason);
```

### 7. Reset to Defaults

```cpp
// Reset a single mapping row
FMapPlayerKeyArgs Args;
Args.MappingName = TEXT("Jump");
FGameplayTagContainer FailureReason;
Settings->ResetAllPlayerKeysInRow(Args, FailureReason);

// Reset entire profile
Settings->ResetKeyProfileIdToDefault(ProfileId, FailureReason);

// Reset via profile directly
Profile->ResetToDefault();
```

### 8. Conflict Detection (QueryMapKeyInActiveContextSet)

This is on `IEnhancedInputSubsystemInterface` (the subsystem itself, NOT user settings):

```cpp
UEnhancedInputLocalPlayerSubsystem* Subsystem = ...;

TArray<FMappingQueryIssue> Issues;
EMappingQueryResult Result = Subsystem->QueryMapKeyInActiveContextSet(
    IMC,                                    // The IMC containing the action
    Action,                                 // The input action
    NewKey,                                 // The key to test
    Issues,                                 // Output: list of issues
    EMappingQueryIssue::CollisionWithMappingInSameContext  // What issues are fatal
);

if (Result == EMappingQueryResult::NotMappable)
{
    for (const FMappingQueryIssue& Issue : Issues)
    {
        // Issue.Issue -- the type of issue (ReservedByAction, HidesExistingMapping, etc.)
        // Issue.BlockingAction -- what action is conflicting
        // Issue.BlockingContext -- what IMC the conflict is in
    }
}
```

**Issue types** (from `EMappingQueryIssue`):
- `ReservedByAction` -- key reserved exclusively by another action
- `HidesExistingMapping` -- new mapping will hide an existing one
- `HiddenByExistingMapping` -- new mapping will be hidden by existing one
- `CollisionWithMappingInSameContext` -- collision within same IMC
- `ForcesTypePromotion` -- key type mismatch (e.g., button to axis)
- `ForcesTypeDemotion` -- key type mismatch (e.g., 2D axis to 1D)

**Reverse lookup** (what actions use a specific key):
```cpp
TArray<FName> MappingNames;
Profile->GetMappingNamesForKey(NewKey, MappingNames);
// If MappingNames is not empty, there's a conflict
```

### 9. Persistence (Built-in SaveGame)

`UEnhancedInputUserSettings` extends `USaveGame`. Persistence is built-in:

```cpp
// Save (synchronous)
Settings->SaveSettings();

// Save (asynchronous)
Settings->AsyncSaveSettings();

// Loading happens automatically via LoadOrCreateSettings() on subsystem init
```

Save file location: `<Project>/Saved/SaveGames/<InputSettingsSaveSlotName>.sav`
Default slot name: `"EnhancedInputUserSettings"`

The system only serializes **dirty** (customized) mappings, not all mappings.

### 10. Key Profiles (Multiple Binding Sets)

The system supports multiple key profiles (e.g., "Default", "Left-handed", "AZERTY"):

```cpp
// Create a new profile
FPlayerMappableKeyProfileCreationArgs CreateArgs;
CreateArgs.ProfileStringIdentifier = TEXT("LeftHanded");
CreateArgs.DisplayName = FText::FromString(TEXT("Left-Handed"));
CreateArgs.bSetAsCurrentProfile = true;
UEnhancedPlayerMappableKeyProfile* NewProfile = Settings->CreateNewKeyProfile(CreateArgs);

// Switch active profile
Settings->SetActiveKeyProfile(TEXT("LeftHanded"));

// Get current profile ID
const FString& ActiveId = Settings->GetActiveKeyProfileId();
```

### 11. Key Slots (Multiple Keys per Action)

Each mapping supports multiple slots (up to 7):
```
| Mapping Name | Slot 1 (Primary) | Slot 2 (Secondary) | Slot 3... |
| Jump         | Space             | Gamepad_FaceButton_Bottom | ... |
```

`EPlayerMappableKeySlot`: `First`, `Second`, `Third`, `Fourth`, `Fifth`, `Sixth`, `Seventh`, `Unspecified`

## Key Classes Reference

### UEnhancedInputUserSettings
- **Location**: `Engine/Plugins/EnhancedInput/Source/EnhancedInput/Public/UserSettings/EnhancedInputUserSettings.h`
- **Base**: `USaveGame`
- **Config**: `GameUserSettings`
- **Purpose**: Central hub for all player input settings. Owns key profiles, handles mapping/unmapping, save/load.
- **Key methods**: `MapPlayerKey`, `UnMapPlayerKey`, `SaveSettings`, `GetActiveKeyProfile`, `RegisterInputMappingContext`
- **Delegates**: `OnSettingsChanged`, `OnSettingsApplied`, `OnKeyProfileChanged`

### UEnhancedPlayerMappableKeyProfile
- **Location**: Same header as above
- **Purpose**: Stores one set of player key mappings. Multiple profiles can exist (Default, Custom, etc.)
- **Key methods**: `GetPlayerMappingRows`, `QueryPlayerMappedKeys`, `GetMappedKeysInRow`, `GetMappingNamesForKey`, `ResetToDefault`

### FPlayerKeyMapping
- **Location**: Same header
- **Purpose**: A single key mapping entry. Tracks default key, current key, slot, display info.
- **Key methods**: `GetCurrentKey`, `GetDefaultKey`, `IsCustomized`, `GetMappingName`, `GetDisplayName`, `GetDisplayCategory`, `ResetToDefault`

### FMapPlayerKeyArgs
- **Location**: Same header
- **Purpose**: Arguments struct for `MapPlayerKey`/`UnMapPlayerKey` calls.
- **Fields**: `MappingName`, `Slot`, `NewKey`, `HardwareDeviceId`, `ProfileIdString`, `bCreateMatchingSlotIfNeeded`, `bDeferOnSettingsChangedBroadcast`

### UPlayerMappableKeySettings
- **Location**: `Engine/Plugins/EnhancedInput/Source/EnhancedInput/Public/PlayerMappableKeySettings.h`
- **Purpose**: Metadata on InputAction or ActionKeyMapping that marks it as player-mappable.
- **Fields**: `Name`, `DisplayName`, `DisplayCategory`, `SupportedKeyProfileIds`, `Metadata`

### UPlayerMappableInputConfig (DEPRECATED since 5.3)
- **Location**: `Engine/Plugins/EnhancedInput/Source/EnhancedInput/Public/PlayerMappableInputConfig.h`
- **Status**: `UE_DEPRECATED(5.3, "Please use UEnhancedInputUserSettings instead.")`
- **Do NOT use this.**

### IEnhancedInputSubsystemInterface
- **Location**: `Engine/Plugins/EnhancedInput/Source/EnhancedInput/Public/EnhancedInputSubsystemInterface.h`
- **Key methods for rebinding**: `QueryMapKeyInActiveContextSet` (conflict detection), `QueryKeysMappedToAction`, `GetAllPlayerMappableActionKeyMappings`, `RequestRebuildControlMappings`

### EMappingQueryIssue / FMappingQueryIssue
- **Location**: `Engine/Plugins/EnhancedInput/Source/EnhancedInput/Public/InputMappingQuery.h`
- **Purpose**: Conflict detection results. Bitflag enum with predefined fatal issue sets.

## Blueprint Accessibility

All key methods are `BlueprintCallable`:
- `MapPlayerKey` -- yes
- `UnMapPlayerKey` -- yes
- `SaveSettings` / `AsyncSaveSettings` -- yes
- `GetActiveKeyProfile` -- yes
- `QueryMapKeyInActiveContextSet` -- yes
- `QueryKeysMappedToAction` -- yes
- `GetPlayerMappingRows` -- yes (returns const ref)
- `GetMappedKeysInRow` -- yes
- `ResetToDefault` -- yes
- All `FPlayerKeyMapping` fields are `BlueprintReadOnly`

This means the entire system can be driven from Blueprint, which is ideal for a settings UI widget.

## Implementation Approach for Settings UI

### UI Flow (typical "click to rebind" pattern):

1. **Display**: Iterate `GetPlayerMappingRows()` to build the key binding list. Group by `DisplayCategory`. Show `DisplayName` + `GetCurrentKey()` for each mapping.

2. **Listen for input**: When user clicks a binding button, enter "listening" mode. Capture the next key press via `APlayerController::InputKey` override or a dedicated input listener widget.

3. **Check conflicts**: Before applying, call `GetMappingNamesForKey(NewKey, ConflictingNames)` on the profile. If conflicts exist, prompt user to swap or cancel.

4. **Apply**: Call `Settings->MapPlayerKey(Args, FailureReason)`. The system automatically triggers `RequestRebuildControlMappings` so the new binding takes effect immediately.

5. **Save**: Call `Settings->SaveSettings()` after user confirms changes (or on settings screen close).

6. **Reset**: Per-row reset via `ResetAllPlayerKeysInRow` or full profile reset via `ResetKeyProfileIdToDefault`.

### What we need to build (game-side):
- A widget that displays the binding list (iterates profiles/mapping rows)
- A "key listener" modal that captures the next key press
- Conflict resolution UI (swap/cancel dialog)
- Calls to the built-in `MapPlayerKey`/`UnMapPlayerKey`/`SaveSettings` API

### What we do NOT need to build:
- Custom save/load system (built-in via `USaveGame`)
- Custom conflict detection (built-in via `QueryMapKeyInActiveContextSet` and `GetMappingNamesForKey`)
- Custom key profile management (built-in)
- Custom mapping storage (built-in `FPlayerKeyMapping`)

## UE 5.7 Deprecation Notes

- `FGameplayTag ProfileId` deprecated in 5.6, use `FString ProfileIdString` instead
- `UPlayerMappableInputConfig` deprecated since 5.3
- `GetCurrentKeyProfile` deprecated in 5.6, use `GetActiveKeyProfile`
- `GetKeyProfileWithIdentifier(FGameplayTag)` deprecated, use `GetKeyProfileWithId(FString)`
- `OnKeyMappingRegistered` deprecated in 5.7, use `OnKeyMappingRegisteredToProfile`

## Gap Analysis for Nekomata

| Need | Have in UE | Gap |
|------|------------|-----|
| Mark actions as rebindable | `UPlayerMappableKeySettings` on `UInputAction` | Need to add settings to each IA_ asset |
| Display current bindings | `GetPlayerMappingRows`, `GetCurrentKey` | Need to build widget |
| Remap keys | `MapPlayerKey` | API ready, need UI trigger |
| Detect conflicts | `GetMappingNamesForKey`, `QueryMapKeyInActiveContextSet` | API ready, need conflict UI |
| Save/Load | `SaveSettings`, auto-load on init | Built-in, zero gap |
| Reset to defaults | `ResetToDefault`, `ResetAllPlayerKeysInRow` | API ready |
| Multiple key slots | `EPlayerMappableKeySlot` (up to 7) | Built-in |
| Enable system | `bEnableUserSettings` in project settings | One checkbox |
