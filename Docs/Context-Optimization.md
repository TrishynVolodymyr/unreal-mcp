# Context Optimization Guide

Starting a fresh chat with all MCP tools enabled consumes ~76% of available context (153k/200k tokens). This guide helps reduce that.

## Context Breakdown

| Category | Tokens | % of Limit |
|----------|--------|------------|
| MCP tools | 82.3k | 41.1% |
| System tools | 15.6k | 7.8% |
| Memory files (CLAUDE.md) | 6.1k | 3.0% |
| Skills | ~6k | 3.0% |

## Disable Unused MCP Servers

Each MCP server consumes significant context. Disable servers you won't need.

**Location:** `%USERPROFILE%\.config\claude-desktop\mcp.json`

Servers and their token costs:
- `blueprintMCP` - Core Blueprint operations
- `editorMCP` - Actor spawning/transforms
- `umgMCP` - Widget/UI creation
- `nodeMCP` - Node connections
- `datatableMCP` - DataTable CRUD
- `projectMCP` - Folders, structs, enums
- `blueprintActionMCP` - Node discovery (~3.9k for `create_node_by_action_name` alone)
- `materialMCP` - Material graph editing
- `niagaraMCP` - Particle effects

**To disable:** Remove the server entry from `mcp.json` or comment it out.

## Disable Unused Skills

Skills are loaded from `.claude/skills/`. Each skill adds to context.

| Skill | Tokens | When Needed | Default |
|-------|--------|-------------|---------|
| `font-generator` | 3.1k | Creating custom game fonts | Disabled |
| `unreal-mcp-architect` | 1.6k | Feature implementation planning | Enabled |
| `umg-widget-designer` | 938 | UI widget creation workflows | Enabled |
| `unreal-mcp-debugger` | 874 | MCP tool debugging | Enabled |

**To disable:** Move skill folder from `.claude/skills/` to `.claude/additional-skills/`
**To enable:** Move skill folder from `.claude/additional-skills/` to `.claude/skills/`

See `.claude/additional-skills/README.md` for details.

## Recommended Configurations

### Minimal (Blueprint Logic Only)
Enable: `blueprintMCP`, `nodeMCP`, `blueprintActionMCP`
Disable: `umgMCP`, `datatableMCP`, `materialMCP`, `niagaraMCP`

### UI Development
Enable: `umgMCP`, `blueprintMCP`, `nodeMCP`
Disable: `datatableMCP`, `materialMCP`, `niagaraMCP`

### VFX/Materials
Enable: `materialMCP`, `niagaraMCP`, `editorMCP`
Disable: `umgMCP`, `datatableMCP`

## Quick Check

Run `/context` in Claude Code to see current usage breakdown.
