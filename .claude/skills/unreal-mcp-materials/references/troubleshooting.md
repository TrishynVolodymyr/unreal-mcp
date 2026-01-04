# Material Troubleshooting

Common issues and debugging steps when working with materialMCP tools.

---

## Connection Issues

### "Connection Failed"

**Check order:** Connections must go `SourceExpression.OutputPin → TargetExpression.InputPin`

**Common pin name mistakes:**
| Wrong | Correct |
|-------|---------|
| `Color` | `RGB` or `RGBA` |
| `Output` | (default output - leave blank) |
| `Value` | (specific to expression type) |

**Verify expression exists:** Use `get_material_graph_metadata` before connecting.

### "Pin Not Found"

Use `get_material_graph_metadata` with full detail to see available pins:
- Texture nodes: `RGB`, `R`, `G`, `B`, `A`
- Math nodes: Usually unnamed default output
- Parameters: Check specific output names

---

## Material Not Appearing

### On Actor

1. Verify material compiled: Check for errors in metadata
2. Confirm slot index: `apply_material_to_actor` uses 0-indexed slots
3. Check actor type: Static mesh vs skeletal mesh

### In Editor

1. Material may need recompile
2. Shading model mismatch with preview
3. Check blend mode vs preview background

---

## Visual Issues

### Material is Black

- **Missing connections:** BaseColor not connected
- **Normal map issue:** Wrong sampler type (must be `Normal`)
- **Texture missing:** Parameter texture not assigned

### Material is Pink/Magenta

- Shader compilation error
- Use `get_material_metadata` to check error state

### Texture Stretched/Wrong Scale

- Check `TextureCoordinate` tiling
- World-aligned materials: Verify divide constant matches expected scale

### Normal Map Looks Flat

Verify normal texture sampler settings:
```
set_material_expression_property(
  expression_name="NormalTex",
  property_name="SamplerType",
  property_value="Normal"
)
```

### Transparency Not Working

1. Check Blend Mode: Must be `Translucent` or `Masked`
2. For Masked: Verify `OpacityMask` connected (not `Opacity`)
3. For Translucent: `Opacity` should be < 1.0

---

## Parameter Issues

### Parameter Not Showing in Instance

- Verify parameter is exposed (group name set)
- Check parameter was created correctly: `ScalarParameter`, `VectorParameter`
- Static parameters only appear in advanced section

### Parameter Has No Effect

- Ensure parameter is connected to something affecting output
- Check for broken connections after parameter
- Multiply nodes need both inputs connected

---

## Performance Issues

### High Instruction Count

Use `get_material_metadata` to check instruction count.

**Reduction strategies:**
1. Pack textures (ORM instead of separate)
2. Replace `If` with `StaticSwitch`
3. Reduce texture samples
4. Simplify math chains

### Slow Material Compilation

- Too many expressions
- Complex custom nodes
- Many texture samples

---

## MCP Tool-Specific Issues

### Expression Not Added

- Check exact expression class name
- Some expressions require properties set immediately after creation

### Graph Looks Messy

Expressions are placed at default position. Use editor to organize after creation.

### Can't Find Expression

Expression names are auto-generated. Use `get_material_graph_metadata` to find actual names assigned by the engine.

---

## Debug Workflow

1. **Get metadata first:** `get_material_metadata` → check for errors
2. **Get graph detail:** `get_material_graph_metadata` → see all nodes/connections
3. **Verify one connection:** Add simple `Constant` → `BaseColor` to confirm material works
4. **Build incrementally:** One expression and connection at a time
