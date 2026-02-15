# Portrait Scene Capture — Implementation Plan

## Goal
SceneCapture2D captures character's head/face → renders to RT_Portrait → displays in HUD portrait circle.
Must work during movement, jumping, animations.

## Current State (2026-02-15)
- ✅ SceneCaptureComponent2D "PortraitCapture" added to BP_ThirdPersonCharacter
- ✅ RT_Portrait (256×256) TextureRenderTarget2D created at /Game/Rendering/RT_Portrait
- ✅ M_Portrait material (UI domain, TextureSample→EmissiveColor) at /Game/Materials/M_Portrait
- ✅ M_Portrait applied to Image_PortraitFace Brush in WBP_HUD_PortraitGroup
- ✅ Static capture works when character stands still
- ❌ Portrait disappears during movement/jumping (camera attached to Capsule, mesh moves relative to it)
- ⚠️ Debug BeginPlay nodes exist (GetSocketTransform → PrintString) — REMOVE when done

## Camera Position (current, on Capsule)
- Location: X=50 (forward), Y=0, Z=75 (head height)
- Rotation: Pitch=0, Yaw=180, Roll=0 (looking back at character)
- FOV: 40°

## Head Bone Info (from PrintString debug)
- World transform at rest: Translation X=401 Y=0.2 Z=161, Rotation P=83.2 Y=-93.3 R=85.6
- Bone-local axes are heavily rotated — static bone-local offsets are impractical

## Three Approaches (from research)

### Approach 1: FindLookAtRotation on Tick ⭐ RECOMMENDED — simplest
- Keep camera on Capsule (predictable position)
- Every Tick: get head socket world location → FindLookAtRotation → set camera rotation
- Camera always points at head regardless of animation state
- **Pros:** Minimal code, no coordinate headaches, works with any animation
- **Cons:** Slight Tick cost (negligible for one component), camera position doesn't follow head lean
- **Implementation:** ~5 Blueprint nodes in EventGraph

### Approach 2: Runtime AttachToComponent in BeginPlay
- In BeginPlay: call AttachToComponent(Mesh, "head", SnapToTarget)
- Set relative location/rotation after attach (runtime attach resolves bone transform correctly)
- Different from editor static attach — runtime attach applies bone rotation to relative offsets
- **Pros:** Camera physically follows head, lean/tilt reflected
- **Cons:** Need to figure out correct relative offset (still bone-local, but runtime may be more predictable)
- **Implementation:** ~4 Blueprint nodes in BeginPlay + trial-and-error offset tuning

### Approach 3: Separate Portrait Scene (AAA approach)
- Create separate sub-level or hidden area with copy of character head mesh
- Dedicated lighting setup for portrait
- SceneCapture only renders that isolated scene
- **Pros:** Full lighting control, no background bleed, professional look
- **Cons:** Duplicate mesh overhead, more complex setup, overkill for current stage
- **Implementation:** Separate actor + mesh + lights + scene capture actor

## Next Step
Try Approach 1 (FindLookAtRotation on Tick).
