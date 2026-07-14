# Unreal MCP Mesh Tools

## `get_static_mesh_metadata`

Returns LOD counts and per-LOD geometry statistics, bounds, material slots,
screen sizes, collision presence, and Nanite state for a static-mesh asset.

## `set_lod_count`

Changes the number of source-model LOD slots (1-8). Newly added slots contain
no authored geometry until populated by `import_lod` or mesh reduction.

## `import_lod`

Imports an FBX/OBJ file into an existing LOD slot. The target slot is replaced;
use `set_lod_count` first when the slot does not yet exist.

## `set_lod_screen_sizes`

Sets the ordered screen-size thresholds used for LOD transitions, one value per
LOD from LOD0 outward.

## `auto_generate_lods`

Builds lower LODs from LOD0 using UE mesh reduction and caller-provided
triangle retention percentages.

## `set_static_mesh_properties`

Edits asset-level static-mesh properties and saves the mesh package.

### Collision contract

`has_collision` changes real simple-collision geometry; it is not a metadata flag.

- `false` removes every aggregate-geometry element, including convex collision.
- `true` keeps existing simple collision or generates a box primitive when none exists.
- `true` creates a missing `UBodySetup` before fitting the primitive; a body-setup-less mesh is a supported input.
- The command returns an error when Unreal cannot apply the requested collision state.
- The command returns an error when the edited package cannot be saved; an in-memory edit is not reported as persisted success.

The implementation checks `UBodySetup::AggGeom.GetElementCount()`. Do not use
`UStaticMeshEditorSubsystem::GetSimpleCollisionCount()` for the absence gate: in
UE 5.7 it counts boxes, spheres, and capsules but omits convex elements.

Collision generation/removal uses the subsystem's non-applying variants and is
folded into the command's single final `PostEditChange`. This avoids a second
mesh rebuild when a request combines collision with material or Nanite edits.
Because those subsystem calls close an open Static Mesh Editor even in
non-applying mode, the command owns the editor lifecycle and reopens it after the
batched rebuild. Referenced materials and slot indices are validated before any
mutation, and collision is attempted before infallible property assignments, so
a rejected request does not leave an earlier material or Nanite edit behind.

```python
set_static_mesh_properties(
    mesh_path="/Game/Flora/SM_Grass",
    has_collision=False,
)
```

Other optional properties are `enable_nanite`, `material_path`, and the paired
`material_slot_index` / `material_slot_path` fields. Supplying only one member of
that pair is rejected before editor closure, mutation, or package save.
