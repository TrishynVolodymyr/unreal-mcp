---
name: crash-investigator
description: >
  Investigate Unreal Engine crashes by parsing crash logs and correlating with recent
  MCP tool calls to identify the culprit. Handles WinError 10054, access violations,
  ensure failures, and other crash types. ALWAYS trigger on: crash, crashed, UE crash,
  editor crashed, PIE crashed, WinError, access violation, fatal error, unhandled exception,
  what caused the crash, why did it crash, investigate crash, crash log, crash dump,
  or any mention of Unreal Engine crashing during MCP operations.
---

# Crash Investigator

Investigate UE crashes and correlate with MCP tool calls.

## Key Locations

```
MCPGameProject/Saved/Crashes/          # Crash dump folders
MCPGameProject/Saved/Logs/MCPGameProject.log  # Runtime log (pre-crash context)
```

## Crash Types & MCP Correlation

| Error Pattern | Likely MCP Cause |
|---------------|------------------|
| `WinError 10054` | Connection reset - MCP server disconnected mid-operation |
| `Access violation reading 0x0` | Null object from failed MCP create/get operation |
| `Ensure failed` | MCP returned invalid data, Blueprint consumed it |
| `Pure virtual function call` | Object deleted while MCP was referencing it |
| `OutOfMemory` | MCP loop creating unlimited assets |

## Workflow

1. **Locate crash data** - Find latest crash folder in `Saved/Crashes/`
2. **Run investigator** - Execute `scripts/investigate_crash.py`
3. **Review findings** - Identify MCP call chain leading to crash

## Script Usage

```bash
python3 scripts/investigate_crash.py <project_path> [--crash-id CRASHID]
```

Options:
- `project_path`: Root of MCPGameProject
- `--crash-id`: Specific crash folder name (default: most recent)

## Output Sections

1. **Crash Summary** - Error type, timestamp, call stack highlight
2. **MCP Timeline** - Last 20 MCP operations before crash
3. **Correlation** - Which MCP call likely triggered it
4. **Recovery Hints** - Suggested fixes based on crash type

## Manual Investigation

If script unavailable, check these manually:

```
# In crash folder, find the .log file:
Saved/Crashes/UECC-Windows-*/MCPGameProject.log

# Search for:
grep -i "error\|exception\|fatal" *.log
grep -i "LogMCP" *.log | tail -20
```

## Common Fixes by Crash Type

- **WinError 10054**: Restart MCP server, check for long-running operations
- **Null access after MCP call**: Add IsValid checks before using returned objects
- **Ensure in MCP code**: Check tool parameters, validate inputs before calls
