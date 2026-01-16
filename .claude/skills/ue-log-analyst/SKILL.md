---
name: ue-log-analyst
description: >
  Parse Unreal Engine logs after PIE sessions to extract user-generated output only.
  Filters MCPGameProject/Saved/Logs/MCPGameProject.log for LogBlueprintUserMessages
  (Print String) and custom MCP plugin logs while ignoring default engine noise.
  ALWAYS trigger on: check logs, what happened in PIE, analyze runtime, parse logs,
  show print strings, debug output, PIE output, what printed, log analysis,
  execution trace, runtime logs, or any request to see what happened during PIE testing.
---

# UE Log Analyst

Parse Unreal Engine PIE session logs, extracting only user-generated content.

## Log Location

```
MCPGameProject/Saved/Logs/MCPGameProject.log
```

## Target Log Categories

Extract ONLY these categories (ignore everything else):

| Category | Source | Purpose |
|----------|--------|---------|
| `LogBlueprintUserMessages` | Print String nodes | Blueprint debug output |
| `LogMCP*` | MCP C++ plugin | Custom plugin diagnostics |
| `LogTemp` | UE_LOG(LogTemp,...) | Quick debug prints |

## Workflow

1. **Locate log file** - Use the standard path or ask user if project location differs
2. **Run parser script** - Execute `scripts/parse_ue_log.py` with the log path
3. **Present findings** - Summarize execution flow, errors, and anomalies

## Script Usage

```bash
python3 scripts/parse_ue_log.py <log_file_path> [--last-session] [--category CATEGORY]
```

Options:
- `--last-session`: Only show logs from most recent PIE session (detects `PIE:` markers)
- `--category`: Filter to specific category (e.g., `LogBlueprintUserMessages`)

## Analysis Guidelines

When presenting results:

1. **Chronological flow** - Show execution order with timestamps
2. **Group by context** - Cluster related prints (same Blueprint, same function)
3. **Highlight issues** - Flag warnings, errors, unexpected values
4. **Identify gaps** - Note if expected prints are missing (suggests early exit or crash)

## Common Patterns to Watch

- **Rapid repeats**: Same message many times → infinite loop or tick-based spam
- **Missing continuation**: Print A exists, expected Print B absent → early return/crash
- **Error cascade**: One error followed by many → root cause identification
- **Timing anomalies**: Large gaps between sequential prints → performance issue
