# MCP Issue Report Template

Use this format when creating issue files. Save to project root as:
`MCP_ISSUE_{YYYYMMDD_HHMMSS}_{brief-description}.md`

---

## Issue Summary

[One sentence describing the problem]

## Context

**Feature being implemented:** [e.g., Dialogue System]
**Current step:** [e.g., Step 3 - Create placeholder widget]
**Plan file:** [path to plan file]

## What Was Attempted

**MCP Tool(s) used:**
- `[tool_name]`

**Payload sent:**
```json
{
  // exact payload
}
```

**Expected result:**
[What should have happened]

**Actual result:**
[What actually happened, include any error messages]

## Analysis

[Your understanding of why this failed. Was it:]
- Tool limitation?
- Incorrect payload format?
- Missing prerequisite?
- Unexpected tool behavior?
- Something else?

## Suggested Fix

[If you have ideas for how the MCP tool should be improved]

## Workaround Used

[If you found a workaround, describe it. If blocked, state "Blocked - no workaround found"]
