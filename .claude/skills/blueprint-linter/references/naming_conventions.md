# Blueprint Naming Conventions

## Asset Names

| Type | Prefix | Example |
|------|--------|---------|
| Blueprint | `BP_` | `BP_PlayerCharacter` |
| Widget Blueprint | `WBP_` | `WBP_InventoryPanel` |
| Animation Blueprint | `ABP_` | `ABP_PlayerAnimations` |
| Actor Component | `AC_` | `AC_HealthComponent` |
| Interface | `BPI_` | `BPI_Interactable` |
| Enum | `E` | `EWeaponType` |
| Struct | `F` or `S` | `FInventorySlot` |

## Variables

### General Rules
- PascalCase: `CurrentHealth`, `MaxSpeed`
- Descriptive: avoid `x`, `temp`, `val`
- No type prefixes (Hungarian notation) except booleans

### Boolean Variables
- Prefix with `b`: `bIsAlive`, `bCanJump`
- Question form: Should read as yes/no question
- Bad: `Dead`, `Jumping`, `Active`
- Good: `bIsDead`, `bIsJumping`, `bIsActive`

### Arrays/Collections
- Plural form: `Enemies`, `InventorySlots`
- Or suffix: `EnemyList`, `ItemArray`

### References
- Suffix with type for clarity when ambiguous:
  - `TargetActor`, `OwningWidget`, `ParentComponent`

## Functions

### General Rules
- Verb + Noun: `GetHealth`, `SetPosition`, `CalculateDamage`
- No prefixes (unlike C++)

### Common Verbs
| Action | Verb | Example |
|--------|------|---------|
| Retrieve | Get | `GetCurrentHealth` |
| Assign | Set | `SetTargetLocation` |
| Boolean check | Is/Has/Can | `IsAlive`, `HasWeapon`, `CanJump` |
| Creation | Create/Spawn | `CreateInventorySlot`, `SpawnProjectile` |
| Destruction | Destroy/Remove | `DestroyWidget`, `RemoveItem` |
| Calculation | Calculate/Compute | `CalculateDamage` |
| Validation | Validate/Check | `ValidateInput` |
| Application | Apply | `ApplyDamage`, `ApplyBuff` |

### Event Functions
- Prefix with `On`: `OnDamageReceived`, `OnItemPickedUp`
- Past tense for completed actions: `OnDied`, `OnLanded`

### Pure Functions
- No side effects implied: `GetHealth` not `FetchHealth`
- Boolean returns: `IsValid`, `HasAuthority`

## Event Dispatchers

- Prefix with `On`: `OnHealthChanged`, `OnInventoryUpdated`
- Describe what happened, not what to do

## Local Variables

- Same rules as class variables
- Can be shorter if scope is obvious
- `Index`, `TempValue` acceptable in tight loops

## Parameters

- Descriptive of purpose: `DamageAmount`, `TargetActor`
- Input vs Output clarity:
  - `In` prefix for input refs: `InOutHealth`
  - `Out` prefix for output: `OutResult`

## Anti-Patterns

| Bad | Why | Good |
|-----|-----|------|
| `MyFunction` | Non-descriptive | `CalculateJumpHeight` |
| `DoStuff` | Vague | `ProcessInventoryUpdate` |
| `HandleThing` | Generic | `HandleDamageEvent` |
| `Temp` | Meaningless | `CachedDamageValue` |
| `Data` | Ambiguous | `PlayerStatistics` |
| `Manager` | Overused, vague | `InventoryController` |
| `flag` | Non-descriptive | `bShouldRespawn` |
