# Blueprint Extension & Contracts Implementation Guide

## Existing Infrastructure

### FAssetDiscoveryService
Your plugin already has a robust **asset discovery service** at [AssetDiscoveryService.cpp](MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Services/AssetDiscoveryService.cpp) with:

- ✅ `FindBlueprints(BlueprintName, SearchPath)` - Find Blueprint assets by name
- ✅ `FindAssetByPath(AssetPath)` - Load assets by full path
- ✅ `FindAssetByName(AssetName, AssetType)` - Generic asset search
- ✅ `NormalizeAssetPath(AssetPath)` - Path normalization
- ✅ `ResolveObjectClass(ClassName)` - Native class resolution

**This service should be used for all Blueprint parent class resolution** - no hardcoded paths needed!

## Current State Analysis

### ✅ What Currently Works

1. **Blueprint Creation with Native C++ Parent Classes**
   - Location: [CreateBlueprintCommand.cpp](MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/Blueprint/CreateBlueprintCommand.cpp)
   - The system can create blueprints inheriting from native C++ classes like:
     - `AActor`, `APawn`, `ACharacter`, `APlayerController`, `AGameModeBase`
     - `UActorComponent`, `USceneComponent`
   - Resolution logic in [CreateBlueprintCommand.cpp:138-210](MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/Blueprint/CreateBlueprintCommand.cpp#L138-L210)

2. **Blueprint Interface Support (Contracts)**
   - **Interface Creation**: [BlueprintCreationService.cpp:127-204](MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Services/Blueprint/BlueprintCreationService.cpp#L127-L204)
   - **Adding Interfaces to Blueprints**: [AddInterfaceToBlueprintCommand.cpp](MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/Blueprint/AddInterfaceToBlueprintCommand.cpp)
   - Python MCP Tools:
     - `create_blueprint_interface()` - [blueprint_tools.py:461-480](Python/blueprint_tools/blueprint_tools.py#L461-L480)
     - `add_interface_to_blueprint()` - [blueprint_tools.py:436-458](Python/blueprint_tools/blueprint_tools.py#L436-L458)

### ❌ What's Missing

1. **Blueprint-to-Blueprint Inheritance**
   - **Current Limitation**: The `parent_class` parameter only resolves to native C++ `UClass*` objects
   - **What's Needed**: Support for passing Blueprint asset paths as parent classes
   - **Impact**: Cannot create child blueprints of other blueprints (e.g., `BP_Enemy_Melee` inheriting from `BP_Enemy_Base`)

2. **Blueprint Introspection**
   - **Current State**: Can add interfaces but cannot introspect implementation status
   - **What's Needed**: Unified metadata query command for parent class, interfaces, variables, functions, etc.

---

## Implementation Roadmap

### Phase 1: Blueprint-to-Blueprint Inheritance (HIGH PRIORITY)

#### 1.1 Update C++ Command Layer

**File**: `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/Blueprint/CreateBlueprintCommand.cpp`

**Changes Needed**:
```cpp
// In ResolveParentClass(), add support for Blueprint paths using existing FAssetDiscoveryService:

UClass* FCreateBlueprintCommand::ResolveParentClass(const FString& ParentClassName) const
{
    // ... existing native class resolution ...

    // NEW: Use FAssetDiscoveryService to find Blueprint assets
    // This handles both full paths (/Game/Blueprints/BP_Base) and simple names (BP_Base)
    UBlueprint* ParentBlueprint = FindParentBlueprint(ParentClassName);
    if (ParentBlueprint && ParentBlueprint->GeneratedClass)
    {
        // Validate the parent is blueprintable
        if (FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentBlueprint->GeneratedClass))
        {
            UE_LOG(LogTemp, Log, TEXT("Resolved Blueprint parent class: %s"), *ParentBlueprint->GetName());
            return ParentBlueprint->GeneratedClass;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Blueprint '%s' is not blueprintable"), *ParentClassName);
        }
    }

    // ... rest of existing code (fallback to AActor) ...
}

// NEW HELPER FUNCTION:
UBlueprint* FCreateBlueprintCommand::FindParentBlueprint(const FString& ParentClassName) const
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    // Handle full path (e.g., /Game/Blueprints/BP_Base)
    if (ParentClassName.StartsWith(TEXT("/Game/")) || ParentClassName.StartsWith(TEXT("/Script/")))
    {
        FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(ParentClassName));
        if (AssetData.IsValid())
        {
            UBlueprint* BP = Cast<UBlueprint>(AssetData.GetAsset());
            if (BP && BP->BlueprintType == BPTYPE_Normal)
            {
                return BP;
            }
        }
    }

    // Handle simple name (e.g., BP_Base) - use FAssetDiscoveryService
    TArray<FString> FoundBlueprints = FAssetDiscoveryService::Get().FindBlueprints(ParentClassName);
    if (FoundBlueprints.Num() > 0)
    {
        UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *FoundBlueprints[0]);
        if (BP && BP->BlueprintType == BPTYPE_Normal)
        {
            return BP;
        }
    }

    return nullptr;
}
```

**Key Design Decisions**:
- ✅ **Use existing `FAssetDiscoveryService::Get().FindBlueprints()`** - Already handles asset registry searching
- ✅ **Follow existing pattern from `AddInterfaceToBlueprintCommand.cpp:62-92`** - Similar logic for Blueprint discovery
- ✅ **Validate `BlueprintType == BPTYPE_Normal`** - Prevent using interfaces as parents
- ✅ **Use `Blueprint->GeneratedClass`** as parent for `FKismetEditorUtilities::CreateBlueprint()`
- ✅ **Validate with `FKismetEditorUtilities::CanCreateBlueprintOfClass()`** before creation

**References**:
- [FKismetEditorUtilities::CreateBlueprint Documentation](https://docs.unrealengine.com/5.1/en-US/API/Editor/UnrealEd/Kismet2/FKismetEditorUtilities/CreateBlueprint/)
- [Working with Blueprint (BP) in Unreal Engine](https://vrealmatic.com/unreal-engine/blueprint)
- [Create Child Blueprint Class from Editor Utility Blueprint](https://forums.unrealengine.com/t/create-child-blueprint-class-from-editor-utility-blueprint/631807)

#### 1.2 Update Service Layer

**File**: `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Services/Blueprint/BlueprintCreationService.cpp`

**Changes Needed**:
- Move Blueprint parent resolution logic to `FBlueprintCreationService::ResolveParentClass()`
- **Utilize `FAssetDiscoveryService::Get().FindBlueprints()`** for asset discovery
- Update the existing `ResolveParentClass()` method at line 207 to support Blueprint parents
- Add validation to prevent circular inheritance (check if child would be ancestor of parent)

**Key Pattern**:
```cpp
UClass* FBlueprintCreationService::ResolveParentClass(const FString& ParentClassName) const
{
    // Try native classes first (existing logic at line 230-257)
    UClass* NativeClass = TryResolveNativeClass(ParentClassName);
    if (NativeClass)
    {
        return NativeClass;
    }

    // NEW: Try Blueprint assets using FAssetDiscoveryService
    TArray<FString> FoundBlueprints = FAssetDiscoveryService::Get().FindBlueprints(ParentClassName);
    if (FoundBlueprints.Num() > 0)
    {
        UBlueprint* ParentBP = LoadObject<UBlueprint>(nullptr, *FoundBlueprints[0]);
        if (ParentBP && ParentBP->GeneratedClass && ParentBP->BlueprintType == BPTYPE_Normal)
        {
            return ParentBP->GeneratedClass;
        }
    }

    // Fallback to AActor (existing behavior)
    return AActor::StaticClass();
}
```

#### 1.3 Update Python MCP Tool

**File**: `Python/blueprint_tools/blueprint_tools.py`

**Changes Needed**:
- Update `create_blueprint()` docstring to document Blueprint parent class support
- Add examples showing both C++ and Blueprint parent classes:
  ```python
  # Native C++ parent
  create_blueprint(name="MyActor", parent_class="Actor")

  # Blueprint parent
  create_blueprint(name="BP_EnemyMelee", parent_class="/Game/Blueprints/BP_EnemyBase")
  ```

#### 1.4 Testing Requirements
- Create blueprint with C++ parent (existing functionality)
- Create blueprint with Blueprint parent
- Create multi-level inheritance chain (BP_A → BP_B → BP_C)
- Verify compiled blueprint hierarchy in editor
- Test component/variable inheritance
- Test function override capability

---

### Phase 2: Blueprint Introspection Command (MEDIUM PRIORITY)

#### 2.1 Unified Blueprint Metadata Query

**New Command**: `GetBlueprintMetadata`

**Design Philosophy**: Single command with selective field queries - no redundant commands, fetch only what you need.

**Available Fields** (based on UBlueprint properties):

1. **`parent_class`** - Parent class information
   - Parent class name, type (Native/Blueprint), asset path
   - Full inheritance chain

2. **`interfaces`** - Implemented interfaces (from `Blueprint->ImplementedInterfaces`)
   - Interface names, asset paths
   - **Interface validation**: Functions per interface with implementation status (implemented/unimplemented)
   - **Function signatures**: Input/output parameters, types, default values, metadata (pure, const, category)
   - **Signature mismatches**: Detect if implemented functions don't match interface signatures

3. **`variables`** - Blueprint variables (from `Blueprint->NewVariables`)
   - Name, type, category, instance editable flag, default values

4. **`functions`** - Custom functions (from `Blueprint->FunctionGraphs`)
   - Function names, parameters, pure/const flags, access level, category

5. **`components`** - Component templates (from `Blueprint->ComponentTemplates`)
   - Component names, types, hierarchy, transform data

6. **`graphs`** - Event/function graphs (from `Blueprint->UbergraphPages`, etc.)
   - Graph names, types, node counts

7. **`status`** - Compilation status (from `Blueprint->Status`)
   - Blueprint status, type, last compile info

8. **`metadata`** - Display metadata
   - BlueprintDisplayName, BlueprintDescription, BlueprintCategory, BlueprintNamespace

9. **`timelines`** - Timeline templates (from `Blueprint->Timelines`)
   - Timeline names, track counts

10. **`asset_info`** - Asset-level information
    - Asset path, package name, disk size, last modified, blueprint system version

**C++ Implementation**:
```cpp
// File: Commands/Blueprint/GetBlueprintMetadataCommand.cpp
// Uses bitfield or array of FString for selective field querying
// Returns JSON object with only requested fields

class FGetBlueprintMetadataCommand : public IUnrealMCPCommand
{
    FString Execute(const FString& Parameters) override;

private:
    TSharedPtr<FJsonObject> BuildParentClassInfo(UBlueprint* BP);
    TSharedPtr<FJsonObject> BuildInterfacesInfo(UBlueprint* BP);
    TSharedPtr<FJsonObject> BuildVariablesInfo(UBlueprint* BP);
    TSharedPtr<FJsonObject> BuildFunctionsInfo(UBlueprint* BP);
    TSharedPtr<FJsonObject> BuildComponentsInfo(UBlueprint* BP);
    TSharedPtr<FJsonObject> BuildGraphsInfo(UBlueprint* BP);
    TSharedPtr<FJsonObject> BuildStatusInfo(UBlueprint* BP);
    TSharedPtr<FJsonObject> BuildMetadataInfo(UBlueprint* BP);
    TSharedPtr<FJsonObject> BuildTimelinesInfo(UBlueprint* BP);
    TSharedPtr<FJsonObject> BuildAssetInfo(UBlueprint* BP);
};
```

**Python MCP Tool**:
```python
@mcp.tool()
def get_blueprint_metadata(
    ctx: Context,
    blueprint_name: str,
    fields: List[str] = None
) -> Dict[str, Any]:
    """
    Get metadata about a Blueprint with selective field querying.

    Args:
        blueprint_name: Name of the Blueprint
        fields: List of metadata fields to retrieve. If None or ["*"], returns all.
                Options: "parent_class", "interfaces", "variables", "functions",
                        "components", "graphs", "status", "metadata",
                        "timelines", "asset_info"

    Returns:
        Dictionary with requested metadata fields

    Examples:
        # Get parent and interfaces only (performance-optimized)
        get_blueprint_metadata(
            blueprint_name="BP_Enemy",
            fields=["parent_class", "interfaces"]
        )

        # Get all metadata
        get_blueprint_metadata(
            blueprint_name="BP_Enemy",
            fields=["*"]
        )

        # Check compilation status before modifying
        status = get_blueprint_metadata(
            blueprint_name="BP_Player",
            fields=["status"]
        )

        # Validate interface implementation before compiling
        interfaces_info = get_blueprint_metadata(
            blueprint_name="BP_Door",
            fields=["interfaces"]
        )
        # Returns:
        # {
        #   "interfaces": {
        #     "BPI_Interactable": {
        #       "path": "/Game/Interfaces/BPI_Interactable",
        #       "functions": [
        #         {
        #           "name": "Interact",
        #           "implemented": true,
        #           "signature_match": true,
        #           "inputs": [{"name": "Interactor", "type": "AActor*"}],
        #           "outputs": [{"name": "Success", "type": "bool"}]
        #         },
        #         {
        #           "name": "CanInteract",
        #           "implemented": false,  # ← Missing implementation!
        #           "signature_match": null,
        #           "inputs": [],
        #           "outputs": [{"name": "Result", "type": "bool"}]
        #         }
        #       ]
        #     }
        #   }
        # }
    """
```

**Key Benefits**:
- ✅ **No redundant commands** - Single unified introspection endpoint
- ✅ **Performance** - Selective querying, only compute what's requested
- ✅ **Extensible** - Easy to add new fields without new commands
- ✅ **AI-friendly** - Assistants can introspect before modifying blueprints

---

## Next Scope

### Phase 1: Blueprint-to-Blueprint Inheritance
- Update `ResolveParentClass()` to use `FAssetDiscoveryService`
- Add circular inheritance detection
- Comprehensive testing for multi-level inheritance chains

### Phase 2: Unified Blueprint Introspection (GetBlueprintMetadata)
- **Essential fields**: `parent_class`, `interfaces` (with validation), `status`
- **Incremental expansion**: `variables`, `functions`, `components`, `graphs`, `timelines`, `asset_info`
- Performance optimization for large blueprints

### Already Implemented ✅
- Blueprint interface creation
- Add interface to blueprint
- Native C++ parent class support
- Asset discovery service

---

## Architecture Considerations

### File Organization (Follow Existing Patterns)

**New C++ Files**:
```
Commands/Blueprint/
  └── GetBlueprintMetadataCommand.h/.cpp  (unified introspection)

Services/Blueprint/
  └── BlueprintMetadataService.h/.cpp  (metadata extraction and validation)
```

**New Python Files**:
```
Python/blueprint_tools/
  └── blueprint_tools.py  (extend existing file, keep under 800 lines)
```

### Synchronization Requirements

**CRITICAL**: Both Python and C++ sides MUST remain synchronized:
- Command names must match exactly
- Parameter names, types, and order must be identical
- JSON schema must be consistent
- Return value structures must match

### Error Handling

**Blueprint Parent Class Resolution**:
- Clear error messages when Blueprint asset not found
- Validation that parent is blueprintable
- Circular inheritance detection
- Type compatibility validation

**Interface Operations**:
- Interface existence validation
- Duplicate interface detection
- Function signature mismatch warnings

---

## Testing Strategy

### Unit Tests
- Blueprint parent class resolution (native C++ classes)
- Blueprint parent class resolution (Blueprint classes)
- Interface addition validation
- Circular inheritance prevention

### Integration Tests
- Create child blueprint chain (3+ levels deep)
- Add multiple interfaces to single blueprint
- Create blueprint with Blueprint parent + interface
- Compile and verify inheritance in editor

### Manual Testing Checklist
- [ ] Create `BP_Base` (native Actor parent)
- [ ] Create `BP_Child` (BP_Base parent)
- [ ] Create `BP_Grandchild` (BP_Child parent)
- [ ] Open BP_Grandchild in editor and verify 3-level hierarchy
- [ ] Add component to BP_Base, verify it appears in BP_Grandchild
- [ ] Create interface `BPI_Interactable`
- [ ] Add interface to `BP_Child`
- [ ] Verify interface appears in `BP_Grandchild`

---

## Code Examples

### Example 1: Creating Child Blueprint (After Implementation)

**Python**:
```python
# Create base blueprint
create_blueprint(
    name="BP_EnemyBase",
    parent_class="Character"
)

# Create child blueprint inheriting from BP_EnemyBase
create_blueprint(
    name="BP_EnemyMelee",
    parent_class="/Game/Blueprints/BP_EnemyBase"  # Blueprint parent!
)

# Create grandchild
create_blueprint(
    name="BP_EnemyMeleeBoss",
    parent_class="/Game/Blueprints/BP_EnemyMelee"
)
```

### Example 2: Interface Implementation

**Python**:
```python
# Create interface
create_blueprint_interface(
    name="BPI_Interactable",
    folder_path="Interfaces"
)

# Add interface to blueprint
add_interface_to_blueprint(
    blueprint_name="BP_Door",
    interface_name="/Game/Interfaces/BPI_Interactable"
)

# Introspect interface implementation (using GetBlueprintMetadata)
metadata = get_blueprint_metadata(
    blueprint_name="BP_Door",
    fields=["interfaces"]
)
# Returns interface validation with implementation status
```

---

## References

### External Documentation
- [FKismetEditorUtilities API Documentation](https://docs.unrealengine.com/5.1/en-US/API/Editor/UnrealEd/Kismet2/FKismetEditorUtilities/CreateBlueprint/)
- [Creating Blueprint Classes in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/creating-blueprint-classes-in-unreal-engine)
- [Working with Blueprint (BP) in Unreal Engine](https://vrealmatic.com/unreal-engine/blueprint)
- [Create Child Blueprint Class Forum Discussion](https://forums.unrealengine.com/t/create-child-blueprint-class-from-editor-utility-blueprint/631807)
- [How to get the parent class of a UObject, Blueprint or Data Asset](https://zomgmoz.tv/unreal/How-to-get-the-parent-class-of-a-UObject,-Blueprint-or-Data-Asset)

### Internal Documentation
- [Architecture Guide](MCPGameProject/Plugins/UnrealMCP/Documentation/Architecture_Guide.md)
- [CLAUDE.md Project Instructions](CLAUDE.md)
- [Legacy Command Migration](MCPGameProject/Plugins/UnrealMCP/Documentation/LegacyCommandMigration.md)

---

## Summary

### Current Capabilities
- ✅ Create blueprints with native C++ parent classes
- ✅ Create Blueprint interfaces
- ✅ Add interfaces to blueprints
- ✅ Robust FAssetDiscoveryService for Blueprint asset discovery

### Missing Features
- ❌ **Blueprint-to-Blueprint inheritance**
- ❌ **Unified Blueprint introspection** (GetBlueprintMetadata command)
  - Interface validation & signature inspection
  - Parent class introspection
  - Variables, functions, components metadata

---

**Document Version**: 1.0
**Last Updated**: 2025-12-13
**Author**: Claude Code Analysis
