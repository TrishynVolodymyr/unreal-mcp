# Additional Skills

Skills in this folder are **disabled by default** to reduce AI context usage.

## How to Enable

Move the skill folder into `.claude/skills/`:

```powershell
# Enable font-generator
Move-Item .claude\additional-skills\font-generator .claude\skills\
```

## How to Disable

Move it back here:

```powershell
# Disable font-generator
Move-Item .claude\skills\font-generator .claude\additional-skills\
```

## Available Skills

| Skill | Tokens | Purpose |
|-------|--------|---------|
| `font-generator` | ~3.1k | Create custom game fonts/typography |
