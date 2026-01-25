# Reconnaissance Protocol

Structured communication between PM and skills during planning phase.

## Reconnaissance Request Format

When PM needs assessment from a skill:

```
RECONNAISSANCE REQUEST
Skill: [skill-name]
User Goal: [summary of what user wants]

User Context:
- [Relevant background]
- [Any references provided]
- [Constraints mentioned]

Questions:
1. FEASIBILITY: Is this achievable with your capabilities?
2. APPROACH: Do you have a better approach than the obvious one?
3. USER NEEDS: What information do you need from the user before proceeding?
4. RESOURCES: What's the expected resource cost (memory/CPU/GPU)?
5. DEPENDENCIES: Do you need another skill to complete work first?

Please respond with a SHORT VERDICT (not full implementation).
```

## Expected Response Format

Skills should respond concisely:

```
RECONNAISSANCE RESPONSE
Skill: [skill-name]

FEASIBILITY: [Yes / No / Partial]
- [One sentence explanation]

APPROACH: 
- Recommended: [technique/method]
- Alternative: [if applicable]
- NOT viable: [what won't work, if relevant]

USER NEEDS:
- [Question 1 for user]
- [Question 2 for user]
- Or: "None, can proceed with current info"

RESOURCES:
- Memory: [estimate or "minimal"]
- CPU: [estimate or "minimal"]  
- GPU: [estimate or "minimal"]
- Particles/Actors/etc: [if applicable]

DEPENDENCIES:
- Needs first: [skill-name] for [reason]
- Or: "None, can start immediately"
- Can parallel with: [skill-names]

CONCERNS:
- [Any red flags or limitations]
- Or: "None"
```

## Response Handling

### Feasibility = Yes
Skill is confident. Add to execution plan.

### Feasibility = Partial
Skill can do some but not all. PM should:
1. Clarify which parts are possible
2. Identify gaps
3. Find other skills or manual solutions for gaps

### Feasibility = No
Skill cannot help. PM should:
1. Ask skill for alternatives
2. Check if different skill could help
3. Report to user with options

## Alternative Request

When skill says "No" or has concerns:

```
ALTERNATIVE REQUEST
Skill: [skill-name]

Original goal was not feasible because: [reason from skill]

Can you suggest:
1. A modified approach that IS feasible?
2. A different technique that achieves similar result?
3. What the user should do instead?
```

## Dependency Chain Building

From skill responses, build execution order:

```
Example responses:
- vfx-technical-director: "Dependencies: None"
- niagara-vfx-architect: "Needs first: vfx-technical-director"
- asset-state-extractor: "Dependencies: None, can parallel"
- metasound-sound-designer: "Dependencies: None"

Resulting plan:
Phase 1 (parallel):
  - vfx-technical-director
  - asset-state-extractor  
  - metasound-sound-designer

Phase 2 (sequential):
  - niagara-vfx-architect (after vfx-technical-director)
```

## User Questions Aggregation

Collect all "USER NEEDS" from skills:

```
Before proceeding, please clarify:

From vfx-technical-director:
- What viewing angles will this effect be seen from?
- Is this for gameplay (many instances) or hero moment (one-off)?

From metasound-sound-designer:
- Should the sound be positional (3D) or 2D?
- What's the priority: realism or stylization?
```

## Resource Aggregation

Combine estimates for total project scope:

```
Combined Resource Estimate:

VFX (niagara-vfx-architect):
- Particles: ~500 max
- GPU: Low-Medium

Audio (metasound-sound-designer):
- Voices: 8 concurrent
- CPU: Minimal

Materials (unreal-mcp-materials):
- IPP: ~150 per material
- Textures: 2x 512px

Total Project Impact:
- Should run well on mid-range hardware
- No performance concerns flagged
```

## Red Flag Handling

If any skill raises concerns:

```
⚠️ CONCERNS FLAGGED

vfx-technical-director:
"Billboard sprites won't work for this projectile - 
will look flat from side angles. Must use mesh-based approach."

This affects the plan:
- Cannot use FluidNinja flipbook directly on projectile
- Need mesh core + sprite accents approach
- May need to adjust user expectations

Recommendation: Discuss with user before proceeding.
```
