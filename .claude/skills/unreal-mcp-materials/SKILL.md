---
name: unreal-mcp-materials
description: >
  Expert material creation and optimization for Unreal Engine using materialMCP tools.
  ALWAYS trigger on: creating materials, editing materials, material graphs, shaders, 
  material instances, material parameters, PBR setup, texture sampling, material optimization,
  material expressions, connecting nodes, base color, roughness, metallic, normal maps,
  emissive materials, translucent materials, masked materials, material functions,
  or phrases like "make a material", "shader", "surface", "appearance", "texture mapping".
  Uses create_material, add_material_expression, connect_material_expressions, and related tools.
---

# Unreal MCP Materials

Expert-level material creation using materialMCP tools. Focus: optimized, production-ready materials.

## Core Workflow

### Step 1: Define Material Requirements

Before creating, clarify:
- **Surface type**: Opaque, Masked, Translucent, Additive?
- **Shading model**: Default Lit, Unlit, Subsurface, Clear Coat?
- **Key outputs**: Base Color, Roughness, Metallic, Normal, Emissive?
- **Parameterization**: What needs runtime variation via instances?

### Step 2: Create Material

```
create_material(
  material_name="M_MaterialName",
  asset_path="/Game/Materials/"
)
```

**Naming conventions:**
- `M_` — Master/parent materials
- `MI_` — Material instances
- `MF_` — Material functions

### Step 3: Build Node Graph

Follow this order for clean graphs:

1. **Texture Samplers** — Add all texture samples first
2. **Math Operations** — Lerps, multiplies, adds
3. **Parameters** — Scalar/vector params for instances
4. **Connections** — Connect to material outputs last

### Step 4: Create Instances

Use `create_material_instance` for variations. Expose only necessary parameters.

---

## MCP Tool Reference

### Material Creation

| Tool | Purpose |
|------|---------|
| `create_material` | Create new material asset |
| `create_material_instance` | Create instance from parent |
| `get_material_metadata` | Inspect existing material |
| `get_material_graph_metadata` | Detailed node graph info |

### Node Graph Building

| Tool | Purpose |
|------|---------|
| `add_material_expression` | Add node (sampler, math, param) |
| `connect_material_expressions` | Connect node outputs to inputs |
| `connect_expression_to_material_output` | Connect to final outputs (BaseColor, etc.) |
| `set_material_expression_property` | Configure node properties |
| `delete_material_expression` | Remove nodes |

### Parameter Control

| Tool | Purpose |
|------|---------|
| `set_material_parameter` | Set param value on material/instance |
| `get_material_parameter` | Read current param value |

### Application

| Tool | Purpose |
|------|---------|
| `apply_material_to_actor` | Assign material to actor in level |

---

## Expression Types Quick Reference

### Texture Sampling
- `TextureSample` — Standard texture sampler
- `TextureSampleParameter2D` — Parameterized texture (instanceable)

### Parameters (for instances)
- `ScalarParameter` — Single float (roughness multiplier, etc.)
- `VectorParameter` — RGBA/color values
- `StaticSwitchParameter` — Compile-time branching

### Math
- `Multiply`, `Add`, `Subtract`, `Divide`
- `Lerp` — Blend between values
- `OneMinus` — Invert (1 - x)
- `Clamp`, `Saturate` — Value limiting
- `Power` — Exponential operations

### Utility
- `TextureCoordinate` — UV input
- `VertexColor` — Mesh vertex colors
- `Time` — Animation driver
- `Fresnel` — Edge-based effects

---

## Optimization Rules

### Instruction Count Targets

| Complexity | Base Pass | Target |
|------------|-----------|--------|
| Simple | < 50 | Props, floors |
| Medium | 50-100 | Characters, weapons |
| Complex | 100-150 | Hero assets only |
| Avoid | > 150 | Refactor required |

### Optimization Techniques

1. **Texture channel packing** — RGBA = 4 values, 1 sample
   - R: Metallic, G: Roughness, B: AO, A: Height
2. **Avoid dynamic branching** — Use `StaticSwitch` instead of `If`
3. **Pre-compute in textures** — Bake complex math offline
4. **Limit texture samples** — Each sample = significant cost
5. **Use shared samplers** — `Shared: Wrap` for same-tiled textures

### Anti-Patterns

- Separate textures for R/M/AO (pack them!)
- Runtime math that could be baked
- Unnecessary Fresnel on non-reflective surfaces
- Translucency when Masked works

---

## Common Patterns

See `references/material-patterns.md` for:
- Standard PBR setup
- Masked vegetation
- Emissive with pulse animation
- World-aligned textures
- Distance-based blend
- Vertex color masking

## Troubleshooting

See `references/troubleshooting.md` for:
- Connection failures and pin names
- Material not appearing
- Visual issues (black, pink, stretched)
- Parameter not working
- Performance debugging

---

## Building Connections

### Output Pin Names

When connecting to material outputs via `connect_expression_to_material_output`:

| Output | Pin Name |
|--------|----------|
| Base Color | `BaseColor` |
| Metallic | `Metallic` |
| Roughness | `Roughness` |
| Normal | `Normal` |
| Emissive | `EmissiveColor` |
| Opacity | `Opacity` |
| Opacity Mask | `OpacityMask` |
| Ambient Occlusion | `AmbientOcclusion` |
| World Position Offset | `WorldPositionOffset` |

### Expression Output Pins

Common output pins from expressions:

| Expression | Output Pin |
|------------|-----------|
| `TextureSample` | `RGB`, `R`, `G`, `B`, `A` |
| `ScalarParameter` | (default output) |
| `VectorParameter` | `RGB`, `R`, `G`, `B`, `A` |
| `Multiply/Add/Lerp` | (default output) |
| `Fresnel` | (default output) |

---

## Incremental Workflow

Follow unreal-mcp-architect principles:

1. **Create material** — Verify it exists
2. **Add expressions one at a time** — Test each
3. **Connect incrementally** — Verify connections
4. **Create instance** — Test parameter exposure
5. **Apply to actor** — Visual verification

**After each step:** "Finished [step]. Ready for testing."

---

## Material Settings Checklist

Before finalizing, verify:

- [ ] Blend Mode correct (Opaque/Masked/Translucent)
- [ ] Shading Model appropriate
- [ ] Two-Sided if needed (foliage, cloth)
- [ ] Only necessary parameters exposed
- [ ] Texture compression settings correct
- [ ] Material complexity acceptable for target platform
