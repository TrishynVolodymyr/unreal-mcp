# StateTree Architecture Patterns — Detailed Recipes

This reference contains architecture recipes for common StateTree use cases. Each pattern describes the state hierarchy, evaluators needed, key transitions, and property binding strategy.

Remember: **always discover available task/condition/evaluator types** using search tools before building. Struct paths listed here are illustrative — discover the real ones from your MCP tools.

---

## Table of Contents

1. [Priority-Based NPC AI](#1-priority-based-npc-ai)
2. [Boss Fight Phases](#2-boss-fight-phases)
3. [Quest Progression System](#3-quest-progression-system)
4. [Utility AI (Scoring-Based)](#4-utility-ai-scoring-based)
5. [Ambient Behavior (Random)](#5-ambient-behavior-random)
6. [Hierarchical Combat](#6-hierarchical-combat)
7. [Environmental Gameplay Logic](#7-environmental-gameplay-logic)
8. [Companion/Follower AI](#8-companionnpc-follower-ai)

---

## 1. Priority-Based NPC AI

The bread-and-butter pattern. An NPC that patrols, detects the player, fights, and returns to patrol when the threat is gone.

### Hierarchy

```
Root (Group, TrySelectChildrenInOrder)
├── Combat (Group, enter: enemy detected)
│   ├── Chase (State, enter: too far to attack)
│   │   Task: MoveTo target
│   │   Transition OnTick [in range] → Attack
│   ├── Attack (State, enter: in attack range)
│   │   Task: face target, play attack
│   │   Transition OnStateCompleted → Chase (re-evaluate)
│   │   Transition OnStateFailed → Flee
│   └── Flee (State, enter: health < 20%)
│       Task: MoveTo flee point
│       Transition OnStateCompleted → Root (re-evaluate everything)
├── Alert (State, enter: heard noise)
│   Task: look around, investigate
│   Transition OnStateCompleted → Patrol
│   Transition OnTick [enemy visible] → (handled by Combat enter condition)
├── Patrol (State, enter: has waypoints)
│   Task: move to next waypoint
│   Transition OnStateCompleted → Patrol (next waypoint)
└── Idle (State, default fallback)
    Task: delay 2-4 seconds
    Transition OnStateCompleted → Patrol
```

### Why This Order Works

With TrySelectChildrenInOrder, the tree checks Combat's enter condition first every tick. If an enemy is detected mid-patrol, Combat wins automatically — no explicit "break out of patrol" transition needed. When the enemy escapes, Combat's condition fails and the tree falls through to Alert or Patrol.

### Evaluators Needed

- **Perception Evaluator** — exposes whether an enemy is detected, distance to nearest enemy, enemy actor reference
- **Health Evaluator** — exposes current health percentage
- **Navigation Evaluator** — exposes current waypoint, path validity

### Key Bindings

- Combat → Chase: bind MoveTo target to perception evaluator's enemy actor reference
- Combat → Attack: bind attack direction to perception evaluator's enemy location
- Combat → Flee: bind flee destination to a computed safe point
- Patrol: bind MoveTo destination to navigation evaluator's current waypoint

### Transition Details

- Combat → Flee uses transition priority **High** — once health is low, fleeing should override attack decisions
- Patrol → Patrol (next waypoint) uses OnStateCompleted — the MoveTo task completes when reaching the waypoint

---

## 2. Boss Fight Phases

Health-threshold phases where the boss changes behavior as health drops.

### Hierarchy

```
Root (Group, TrySelectChildrenInOrder)
├── Phase3_Enrage (Group, enter: health < 33%)
│   ├── AoEAttack (State, weight: 3)
│   ├── ChargeAttack (State, weight: 2)
│   └── Summon (State, weight: 1)
├── Phase2_Aggressive (Group, enter: health < 66%)
│   ├── ComboAttack (State, weight: 2)
│   ├── RangedAttack (State, weight: 2)
│   └── Teleport (State, weight: 1)
└── Phase1_Normal (Group, default)
    ├── BasicAttack (State, weight: 3)
    ├── BlockAndCounter (State, weight: 2)
    └── Taunt (State, weight: 1)
```

### Design Notes

- Phases use **enter conditions** based on health ranges. Phase3 at top = highest priority
- Attack patterns within each phase use **TrySelectChildrenAtRandom** with weights for variety
- Each attack state has a task (play animation, deal damage) and transitions OnStateCompleted back to the parent group for re-selection
- Phase transitions are automatic — when health drops below 66%, Phase2's condition passes and it takes over

### Evaluators Needed

- **Health Evaluator** — current HP and percentage
- **Target Evaluator** — player reference, distance, facing direction
- **Phase Evaluator** (optional) — tracks current phase for UI/audio/VFX triggers

### Phase Transition Effects

When transitioning between phases, you may want special behavior (roar animation, invulnerability window). Handle this by:
- Adding a "PhaseTransition" state at the top of each phase group
- This state plays the transition animation, then completes
- A transition OnStateCompleted → first attack pattern starts combat

---

## 3. Quest Progression System

Sequential quest steps with branching and event-driven progression.

### Hierarchy

```
Root (Group, TrySelectChildrenInOrder)
├── Step3_Finale (Group, enter: tag Quest.Step3.Active)
│   ├── BossFight (LinkedAsset → ST_BossEncounter)
│   └── Victory (State)
│       Task: grant rewards, set tag Quest.Complete
├── Step2_Dungeon (Group, enter: tag Quest.Step2.Active)
│   ├── ExploreRooms (State)
│   │   Task: track exploration progress
│   │   Transition OnEvent "Quest.Step2.AllRoomsCleared" → FindKey
│   ├── FindKey (State)
│   │   Task: check key pickup
│   │   Transition OnEvent "Quest.Step2.KeyFound" → OpenDoor
│   └── OpenDoor (State)
│       Task: animate door, set tag Quest.Step3.Active
│       Transition OnStateCompleted → Succeeded
├── Step1_Village (Group, enter: tag Quest.Step1.Active)
│   ├── TalkToElder (State)
│   │   Task: trigger dialogue
│   │   Transition OnEvent "Dialogue.Elder.Complete" → GatherSupplies
│   ├── GatherSupplies (State)
│   │   Task: track item collection
│   │   Transition OnEvent "Quest.Step1.SuppliesGathered" → ReturnToElder
│   └── ReturnToElder (State)
│       Task: trigger dialogue, set tag Quest.Step2.Active
│       Transition OnStateCompleted → Succeeded
└── Inactive (State, enter: no active quest tag)
    Task: idle, wait for quest start
    Transition OnEvent "Quest.Step1.Accepted" → (sets tag, re-evaluates)
```

### Design Notes

- **Gameplay tags** drive progression: each step sets the next step's activation tag on completion
- **OnEvent transitions** listen for specific gameplay tag events fired by game systems (dialogue, pickups, etc.)
- **LinkedAsset** for complex sub-behaviors (the boss fight has its own StateTree rather than cluttering this one)
- Steps are evaluated top-down: Step3 at top means if somehow tags get out of sync, the furthest-progressed step takes priority

### Persistence Considerations

- Quest state is stored in gameplay tags — these should be saved/loaded with your save system
- The StateTree itself is stateless — it re-evaluates from tags on load
- Tasks that modify world state (door opens, items granted) need idempotency for save/load scenarios

---

## 4. Utility AI (Scoring-Based)

Dynamic decision-making where states are scored by multiple factors rather than fixed priority.

### Hierarchy

```
Root (Group, SelectionBehavior depends on implementation)
├── AttackMelee (State, consideration: proximity × aggression × weapon_ready)
├── AttackRanged (State, consideration: distance × ammo × line_of_sight)
├── TakeCover (State, consideration: threat × low_health × cover_nearby)
├── HealSelf (State, consideration: low_health × has_potion × safety)
├── FlankTarget (State, consideration: allies_engaging × can_flank)
└── Retreat (State, consideration: outnumbered × very_low_health)
```

### Design Notes

Utility AI in StateTree is implemented through evaluators that score each potential behavior:

- **Scoring Evaluator** — calculates a utility score for each state based on multiple factors (curves, not linear)
- Enter conditions on each state check: "is my score the highest?"
- Or use a custom selection behavior if your tools support it

Each "consideration" is a factor mapped through a response curve:
- Distance: close = high melee score, far = high ranged score
- Health: low = high heal/retreat score
- Threat: high = high cover score

### Implementation Approach

1. Add evaluators that expose raw data (distance, health, ammo count, etc.)
2. Add a scoring evaluator that computes utility per state
3. Enter conditions compare scores: "my score > all other scores"
4. The state with the highest utility wins selection

This is more complex to set up than priority-based selection but produces more dynamic, believable behavior.

---

## 5. Ambient Behavior (Random)

NPCs doing varied, non-combat ambient actions — townspeople, wildlife, background characters.

### Hierarchy

```
Root (Group, TrySelectChildrenAtRandom)
├── Wander (State, weight: 4)
│   Task: move to random point within radius
│   Transition OnStateCompleted → Root (re-select)
├── SitOnBench (State, weight: 2, enter: bench nearby)
│   Task: move to bench, play sit animation, delay 10-30s
│   Transition OnStateCompleted → Root
├── TalkToNPC (State, weight: 2, enter: other NPC nearby)
│   Task: face NPC, play talk animation, delay 5-15s
│   Transition OnStateCompleted → Root
├── LookAround (State, weight: 3)
│   Task: play look animation, delay 3-5s
│   Transition OnStateCompleted → Root
└── UseObject (State, weight: 1, enter: interactable nearby)
    Task: move to object, play use animation
    Transition OnStateCompleted → Root
```

### Design Notes

- **TrySelectChildrenAtRandom** with weights creates natural variety — NPCs won't repeat the same action predictably
- Higher weight = more likely to be selected. Wander at weight 4 is the most common action
- Enter conditions prevent impossible actions (can't sit if no bench nearby)
- Each state transitions back to Root on completion, triggering a new random selection
- Add a small delay task at Root level to prevent rapid state switching

### Evaluators Needed

- **Surroundings Evaluator** — detects nearby benches, NPCs, interactables within a radius
- **Random Evaluator** (optional) — exposes a random seed that changes periodically for behavior variation

---

## 6. Hierarchical Combat

Deep combat tree with melee/ranged sub-trees, each containing specific attack patterns.

### Hierarchy

```
Root (Group, TrySelectChildrenInOrder)
├── Combat (Group, enter: enemy detected)
│   ├── Melee (Group, enter: distance < 300, TrySelectChildrenAtRandom)
│   │   ├── LightAttack (State, weight: 3)
│   │   ├── HeavyAttack (State, weight: 1)
│   │   └── ComboAttack (State, weight: 2, enter: stamina > 50%)
│   ├── Ranged (Group, enter: distance > 300 AND has ammo)
│   │   ├── SingleShot (State, weight: 3)
│   │   ├── BurstFire (State, weight: 2, enter: ammo > 10)
│   │   └── ThrowGrenade (State, weight: 1, enter: has grenade)
│   └── Defensive (Group, enter: recently hit OR low health)
│       ├── Dodge (State, weight: 2)
│       ├── Block (State, weight: 3)
│       └── CounterAttack (State, weight: 1, enter: just blocked)
├── Patrol (State)
└── Idle (State)
```

### Design Notes

- The outer group (Root) uses priority: Combat > Patrol > Idle
- Within Combat, sub-groups use conditions to pick the right combat style (melee vs ranged vs defensive)
- Within each combat style, TrySelectChildrenAtRandom with weights picks specific attacks for variety
- Defensive group has a higher effective priority because of its enter conditions — it activates when the NPC is threatened

### Transition Flow

After any attack completes, it transitions back to its parent group. The parent re-evaluates conditions and picks the next attack. If distance changed during the attack, the tree might switch from Melee to Ranged automatically.

---

## 7. Environmental Gameplay Logic

Non-AI use case: doors, traps, elevators, puzzle mechanisms.

### Hierarchy (Door Example)

```
Root (Group, TrySelectChildrenInOrder)
├── Locked (State, enter: tag Door.Locked)
│   Task: play locked visual
│   Transition OnEvent "Door.Unlock" → (removes tag, re-evaluates)
├── Opening (State, enter: tag Door.Opening)
│   Task: play open animation (2s)
│   Transition OnStateCompleted → Open
├── Open (State, enter: tag Door.Open)
│   Task: hold open, delay 5s
│   Transition OnStateCompleted → Closing
│   Transition OnEvent "Door.ForceClose" → Closing
├── Closing (State, enter: tag Door.Closing)
│   Task: play close animation (2s)
│   Transition OnStateCompleted → Closed
└── Closed (State, default)
    Task: idle
    Transition OnEvent "Door.Interact" → Opening
```

### Design Notes

- Uses **StateTreeComponentSchema** (not AI) — attached to the door actor
- Gameplay tag events drive transitions: player interaction, key usage, timer
- States correspond to physical states of the object
- No evaluators needed for simple cases — events are sufficient
- For complex puzzles, add evaluators that track puzzle state (switches flipped, pressure plates active, etc.)

---

## 8. Companion/NPC Follower AI

An NPC that follows the player, helps in combat, and has personality-driven idle behavior.

### Hierarchy

```
Root (Group, TrySelectChildrenInOrder)
├── Combat (Group, enter: enemies nearby AND player in combat)
│   ├── SupportPlayer (State, enter: player health < 50%)
│   │   Task: heal player, buff player
│   ├── AttackEnemy (State)
│   │   Task: pick target, attack
│   └── Regroup (State, enter: too far from player)
│       Task: move toward player
├── Follow (State, enter: distance to player > follow threshold)
│   Task: move to follow offset behind player
│   Transition OnTick [close enough] → Idle
├── Interact (State, enter: event "Companion.Interact")
│   Task: face player, play dialogue
│   Transition OnStateCompleted → Follow
└── Idle (Group, TrySelectChildrenAtRandom)
    ├── IdleAnimation (State, weight: 3)
    ├── LookAround (State, weight: 2)
    └── Comment (State, weight: 1)
        Task: play contextual voice line
```

### Evaluators Needed

- **Player Evaluator** — player reference, distance to player, player health, player combat state
- **Threat Evaluator** — nearby enemies, threat level
- **Context Evaluator** — current location type (town, dungeon, wilderness) for contextual comments

### Design Notes

- The companion prioritizes: Combat support > Following > Interaction > Idle
- Follow state uses an OnTick transition with a distance condition — it continuously adjusts until close enough
- Idle behavior uses random selection for personality-driven variety
- The Interact state responds to a gameplay tag event fired when the player presses an interact key near the companion

---

## General Architecture Tips

**Start with 3-4 states.** Don't over-engineer from the start. Get the basic loop working (Idle → Patrol → Combat → Idle), then add sub-states and refinements.

**One evaluator per data domain.** Don't create a single "uber-evaluator" that exposes everything. Separate perception, health, navigation, etc. This keeps bindings organized and makes it easy to swap evaluator implementations.

**Use Group states for visual clarity.** Even if a group only has one child, it names a section of behavior. A tree with "Combat", "Exploration", "Social" groups is immediately readable.

**Test with debug visualization.** Enable StateTree debugging during PIE to see active states, transition evaluations, and binding values in real-time. This catches issues faster than any amount of structural inspection.

**Document your tree.** Complex StateTrees benefit from a plain-text description of the intended behavior alongside the asset. When coming back to modify the tree weeks later, this documentation is invaluable.
