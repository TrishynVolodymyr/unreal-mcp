# Research: Dynamic Input Device Detection in UE 5.7

## Summary

There are two viable approaches. **Approach A (CommonInput plugin)** is the industry standard -- it provides automatic keyboard/gamepad/touch detection with built-in thrashing protection and a Blueprint-assignable delegate. It does NOT require replacing your widgets with CommonUI widgets; `CommonInput` is a separate module from `CommonUI`. **Approach B (UInputDeviceSubsystem)** is a lighter engine-level alternative that tracks hardware device changes, but requires more manual work. Approach A is recommended.

## Approach A: CommonInput Plugin (RECOMMENDED)

### What It Is

The `CommonUI` plugin ships with TWO independent modules:
- **`CommonInput`** -- Input detection only. LocalPlayer subsystem that tracks keyboard vs gamepad vs touch. This is what we need.
- **`CommonUI`** -- Full UI framework (CommonButton, CommonActivatableWidget, etc.). We do NOT need this.

You can use `CommonInput` without `CommonUI`. The `.uplugin` bundles them together, but once enabled, you only need to add `CommonInput` to your Build.cs dependencies.

### Key Classes

**`UCommonInputSubsystem`** (LocalPlayerSubsystem)
- File: `Engine/Plugins/Runtime/CommonUI/Source/CommonInput/Public/CommonInputSubsystem.h`
- Per-player subsystem that tracks current input type
- Blueprint-accessible via `UCommonInputSubsystem::Get(LocalPlayer)`

**`ECommonInputType`** (enum)
```cpp
enum class ECommonInputType : uint8
{
    MouseAndKeyboard,
    Gamepad,
    Touch,
    Count
};
```

### API Surface (All BlueprintCallable)

```cpp
// Get current input type
ECommonInputType GetCurrentInputType() const;

// Check if specific method is active
bool IsInputMethodActive(ECommonInputType InputMethod) const;

// Get default for platform
ECommonInputType GetDefaultInputType() const;

// Get gamepad name (e.g., "XInputController", "DualSense")
const FName GetCurrentGamepadName() const;

// Manually override (for testing)
void SetCurrentInputType(ECommonInputType NewInputType);
```

### The Delegate (Blueprint-Assignable)

```cpp
// C++ native delegate (non-dynamic)
FInputMethodChangedEvent OnInputMethodChangedNative;

// Blueprint-assignable delegate
UPROPERTY(BlueprintAssignable)
FInputMethodChangedDelegate OnInputMethodChanged;
// Signature: void(ECommonInputType bNewInputType)
```

In Blueprint, you bind to `OnInputMethodChanged` on the subsystem. It fires whenever the player switches between keyboard and gamepad.

### How Detection Works Internally

`FCommonInputPreprocessor` (an `IInputProcessor` registered with Slate) intercepts ALL input before widgets see it:
- `HandleKeyDownEvent` -- checks `FKey::IsGamepadKey()` to determine input type
- `HandleMouseMoveEvent` / `HandleMouseButtonDownEvent` -- sets to MouseAndKeyboard
- `HandleAnalogInputEvent` -- gamepad sticks/triggers

It has **built-in thrashing protection**: if the user rapidly alternates (e.g., joystick drift while typing), it suppresses switching for a configurable cooldown window.

### Setup Required

1. **Enable CommonUI plugin** in `.uproject`:
```json
{
    "Name": "CommonUI",
    "Enabled": true
}
```

2. **Add CommonInput to Build.cs** (NOT CommonUI -- just the input module):
```csharp
PublicDependencyModuleNames.Add("CommonInput");
```

3. **Configure in Project Settings > Common Input**:
   - `bEnableEnhancedInputSupport = true` (for Enhanced Input compatibility)
   - `bSupportsMouseAndKeyboard = true`
   - `bSupportsGamepad = true`
   - `DefaultInputType = MouseAndKeyboard` (for PC)
   - Thrashing settings are fine at defaults (30 changes in 3s window = thrashing, 1s cooldown)

4. **In Blueprint (e.g., WBP_HUD_Main or BP_GameHUD)**:
```
BeginPlay:
  Get Local Player → Get Subsystem (CommonInputSubsystem)
  → Bind Event to OnInputMethodChanged
    → Custom Event "OnInputDeviceChanged(NewInputType: ECommonInputType)"
      → Switch on ECommonInputType
        → MouseAndKeyboard: Show KB section, hide gamepad prompts
        → Gamepad: Show gamepad section, show gamepad prompts
```

### No Widget Replacement Needed

`CommonInput` works at the Slate `IInputProcessor` level -- it intercepts raw input events globally. Your existing UMG widgets (TextBlock, Button, Image, etc.) continue working unchanged. The subsystem just tells you WHAT device is active; your UI reacts however you want.

### PIE Compatibility

Works in PIE. The preprocessor is registered with `FSlateApplication` which is active in editor. Gamepad input in PIE is detected normally as long as a gamepad is connected.

---

## Approach B: UInputDeviceSubsystem (Lighter, More Manual)

### What It Is

`UInputDeviceSubsystem` is an Engine subsystem (always available, no plugin needed) that tracks which hardware device each platform user is using.

- File: `Engine/Source/Runtime/Engine/Classes/GameFramework/InputDeviceSubsystem.h`

### Key API

```cpp
// Get most recently used device
FHardwareDeviceIdentifier GetMostRecentlyUsedHardwareDevice(FPlatformUserId InUserId) const;

// Get most recent device of a specific type
FInputDeviceId GetMostRecentlyUsedInputDeviceId(FPlatformUserId InUserId,
    EHardwareDevicePrimaryType OfType = EHardwareDevicePrimaryType::Unspecified) const;

// Delegate when device changes
UPROPERTY(BlueprintAssignable)
FHardwareInputDeviceChanged OnInputHardwareDeviceChanged;
// Signature: void(FPlatformUserId UserId, FInputDeviceId DeviceId)
```

**`EHardwareDevicePrimaryType`**:
```cpp
enum class EHardwareDevicePrimaryType : uint8
{
    Unspecified,
    KeyboardAndMouse,
    Gamepad,
    Touch,
    MotionTracking,
    RacingWheel,
    FlightStick,
    Camera,
    Instrument,
    // ...custom types
};
```

### Why It's Worse Than CommonInput

1. **No thrashing protection** -- you must implement your own debouncing
2. **Gives you device IDs, not input types** -- you need to query `GetInputDeviceHardwareIdentifier` then check the `PrimaryType` field
3. **Less widely documented** -- CommonInput is the standard approach used by Fortnite, Lyra, and most UE5 games
4. **No built-in gamepad name detection** (Xbox vs PS vs Switch)

### When To Use It

Only if you absolutely cannot enable the CommonUI plugin for some reason (e.g., plugin conflicts, minimal dependency requirements).

---

## Approach C: Pure Blueprint DIY (NOT Recommended)

You could bind `AnyKey` input event, check `IsGamepadKey()` on the key, and track state manually. This is what beginners do and it has problems:
- No thrashing protection
- Misses edge cases (analog sticks, touch)
- Reinvents what CommonInput already does perfectly
- More Blueprint spaghetti to maintain

**Do not do this.** CommonInput exists specifically to solve this problem.

---

## Menu Navigation (Gamepad D-pad/Stick)

### Built-In Slate Navigation

UE's `FNavigationConfig` (registered with `FSlateApplication`) already handles gamepad navigation by default:

```cpp
// Default mappings (already active):
D-Pad Left / Arrow Left  → EUINavigation::Left
D-Pad Right / Arrow Right → EUINavigation::Right
D-Pad Up / Arrow Up      → EUINavigation::Up
D-Pad Down / Arrow Down  → EUINavigation::Down
Left Stick X/Y            → Analog navigation (0.5 threshold)

// Actions:
Enter / Space / Gamepad_FaceButton_Bottom → Accept
Escape / Gamepad_FaceButton_Right         → Back
```

### What You Need For Gamepad Menu Navigation

1. **Set `bIsFocusable = true`** on interactive widgets (Buttons are focusable by default)
2. **Set Navigation rules** on widgets -- in UMG Designer, each widget has Navigation settings:
   - Left/Right/Up/Down: Automatic (default), Explicit (point to specific widget), Custom, Stop, Wrap
3. **Set initial focus** when opening a menu -- call `SetFocus()` on the first button
4. **On input switch to Gamepad**: Set focus to first menu item, hide mouse cursor
5. **On input switch to Keyboard**: Clear focus (or leave it), show mouse cursor

### Key Consideration

When `CommonInputSubsystem` reports a switch to Gamepad:
- Call `SetUserFocus()` on the first interactive widget in your menu
- Optionally hide the mouse cursor via `APlayerController::bShowMouseCursor = false`

When it reports MouseAndKeyboard:
- Show mouse cursor
- Optionally clear focus so highlight disappears

---

## Recommendation: Minimal Implementation Plan

### Step 1: Enable CommonUI Plugin
Edit `MCPGameProject.uproject`:
```json
"Plugins": [
    ...,
    {
        "Name": "CommonUI",
        "Enabled": true
    }
]
```

### Step 2: Add CommonInput Module Dependency
In `MCPGameProject.Build.cs` (the game module, NOT the MCP plugin):
```csharp
PublicDependencyModuleNames.Add("CommonInput");
```

### Step 3: Configure CommonInput Settings
In `DefaultGame.ini` (or via Project Settings > Common Input):
```ini
[/Script/CommonInput.CommonInputPlatformSettings_Windows]
DefaultInputType=MouseAndKeyboard
bSupportsMouseAndKeyboard=True
bSupportsGamepad=True
bSupportsTouch=False
DefaultGamepadName=Generic
bCanChangeGamepadType=True
```

Also enable Enhanced Input support:
```ini
[/Script/CommonInput.CommonInputSettings]
bEnableEnhancedInputSupport=True
```

### Step 4: Blueprint Integration
In BP_GameHUD or WBP_HUD_Main BeginPlay:
1. Get Local Player
2. Get Subsystem: UCommonInputSubsystem
3. Bind to `OnInputMethodChanged`
4. On change: switch UI state (show/hide sections, update prompts, manage focus)

### Step 5: Focus Management for Gamepad
When switching to Gamepad mode:
- Set focus to first interactive widget
- Hide cursor
When switching to KBM mode:
- Show cursor
- Optionally clear widget focus

### No C++ Required (Beyond Build.cs)

Everything can be done in Blueprint:
- `Get Subsystem` node works for `UCommonInputSubsystem`
- `Bind Event` on `OnInputMethodChanged` is Blueprint-assignable
- `GetCurrentInputType` is BlueprintCallable
- Widget focus management is all Blueprint-accessible

### What Does NOT Need to Change

- Existing UMG widgets stay as-is
- No need for CommonUI widget base classes
- No need for CommonUI activatable widgets
- Enhanced Input mappings stay unchanged
- Standard Button/TextBlock/Image widgets all work

---

## Reference Files

### UE Source
- `Engine/Plugins/Runtime/CommonUI/Source/CommonInput/Public/CommonInputSubsystem.h` -- Main subsystem
- `Engine/Plugins/Runtime/CommonUI/Source/CommonInput/Public/CommonInputPreprocessor.h` -- Detection logic
- `Engine/Plugins/Runtime/CommonUI/Source/CommonInput/Public/CommonInputTypeEnum.h` -- ECommonInputType enum
- `Engine/Plugins/Runtime/CommonUI/Source/CommonInput/Public/CommonInputSettings.h` -- Settings/config
- `Engine/Plugins/Runtime/CommonUI/Source/CommonInput/Public/CommonInputBaseTypes.h` -- Base types, controller data
- `Engine/Plugins/Runtime/CommonUI/CommonUI.uplugin` -- Plugin manifest (contains both CommonInput and CommonUI modules)
- `Engine/Source/Runtime/Slate/Public/Framework/Application/NavigationConfig.h` -- Gamepad navigation config
- `Engine/Source/Runtime/Engine/Classes/GameFramework/InputDeviceSubsystem.h` -- Alternative approach

### Project Files
- `MCPGameProject/MCPGameProject.uproject` -- Needs CommonUI plugin enabled
- `MCPGameProject/Source/MCPGameProject/MCPGameProject.Build.cs` -- Needs CommonInput dependency
