# MCP Tools — Backlog (Fixes & Improvements)

## Bugs / Fixes

### `add_component_to_blueprint` — no attach_to / socket support
- **Problem:** Cannot specify parent component or socket when adding a component
- **Current:** Component always lands under root (Capsule), requires manual drag in editor
- **Expected:** `attach_to` param (parent component name) + `socket_name` param
- **Workaround:** Manual drag in Components panel, or set AttachSocketName via modify_blueprint_component_properties (but AttachParent doesn't work as property)
- **Priority:** High — needed for Scene Capture, widget components, attached meshes
- **Found:** 2026-02-15 (Portrait Capture setup)

### `create_node_by_action_name` — cannot reference Blueprint components as targets
- **Problem:** No way to create a component reference node (e.g., drag CharacterMesh0 onto graph) via MCP
- **Current:** GetSocketTransform needs Target pin connected to mesh component, but no MCP tool creates component getter nodes
- **Expected:** Either `target` kwarg on create_node resolves component names, or a dedicated tool to create component reference nodes
- **Workaround:** Manual drag from Components panel onto graph
- **Priority:** Medium — needed for any component method call
- **Found:** 2026-02-15 (Portrait Capture setup)

## Improvements / New Tools

### `create_render_target` ✅ (2026-02-15)
- Created and working in editorMCP
- Creates TextureRenderTarget2D assets in Content Browser

---

*Add new items at the top of each section. Date everything.*
