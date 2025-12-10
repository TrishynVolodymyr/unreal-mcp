# Large Files Refactoring Plan

**Methodology**: EXACT COPY-PASTE (not rewriting)
- Copy exact code blocks from original files
- Create new service files with copied code
- Update original files to delegate to new services
- Use TUniquePtr with forward declarations
- Use TFunction callbacks for dependencies

**Target**: Keep all files under 800 lines (absolute max 1000 lines)

---

## Priority 1: UnrealMCPCommonUtils.cpp (2119 lines)

**Current Status**: 2119 lines - CRITICAL REFACTOR NEEDED

### Proposed Split:

#### Phase 1: Extract JsonUtils (Lines ~62-100)
**File**: `Utils/JsonUtils.h/.cpp` (~150 lines)
**Methods to Copy**:
- `GetIntArrayFromJson`
- `GetFloatArrayFromJson`
- `GetStringArrayFromJson`
- Other JSON parsing helpers

**Strategy**:
1. Create `JsonUtils.h` with static utility class
2. Copy-paste all JSON array/object parsing methods
3. Update `UnrealMCPCommonUtils.cpp` to call `FJsonUtils::` methods

---

#### Phase 2: Extract PropertyUtils (Lines ~635-1243)
**File**: `Utils/PropertyUtils.h/.cpp` (~650 lines)
**Methods to Copy**:
- `SetObjectProperty` (lines 635-988)
- `SetPropertyFromJson` (lines 989-1243)
- All property reflection and setting logic

**Strategy**:
1. Create `PropertyUtils.h` with static class `FPropertyUtils`
2. Copy-paste exact property manipulation code
3. Update `UnrealMCPCommonUtils` to delegate to `FPropertyUtils`

---

#### Phase 3: Extract GeometryUtils (Lines ~1244-1316)
**File**: `Utils/GeometryUtils.h/.cpp` (~120 lines)
**Methods to Copy**:
- `ParseVector` (lines 1244-1260)
- `ParseLinearColor` (lines 1261-1300)
- `ParseRotator` (lines 1301-1317)

**Strategy**:
1. Create `GeometryUtils.h` with static class
2. Copy-paste all vector/rotation/color parsing
3. Update to delegate

---

#### Phase 4: Extract AssetUtils (Lines ~1512-1873)
**File**: `Utils/AssetUtils.h/.cpp` (~400 lines)
**Methods to Copy**:
- `FindWidgetClass` (lines 1512-1620)
- `FindAssetByPath` (lines 1621-1638)
- `FindAssetByName` (lines 1639-1849)
- `NormalizeAssetPath` (lines 1850-1873)
- `IsValidAssetPath` (lines 1874+)

**Strategy**:
1. Create `AssetUtils.h` with static class
2. Copy-paste all asset finding/loading code
3. Update to delegate

---

#### Phase 5: Extract GraphUtils (Lines ~468-634)
**File**: `Utils/GraphUtils.h/.cpp` (~200 lines)
**Methods to Copy**:
- `ConnectGraphNodes` (lines 468-634)
- Any other graph manipulation utilities

**Strategy**:
1. Create `GraphUtils.h` with static class
2. Copy-paste graph connection logic
3. Update to delegate

---

#### Phase 6: Extract ActorUtils (Lines ~1318-1511)
**File**: `Utils/ActorUtils.h/.cpp` (~250 lines)
**Methods to Copy**:
- `FindActorByName` (lines 1318-1333)
- `CallFunctionByName` (lines 1334-1511)

**Strategy**:
1. Create `ActorUtils.h` with static class
2. Copy-paste actor finding and function calling
3. Update to delegate

---

### Expected Result for UnrealMCPCommonUtils:
- **Original**: 2119 lines
- **After Refactor**: ~350 lines (remaining coordination code)
- **Extracted Services**: 6 new utility files (~1770 lines total)

---

## Priority 2: BlueprintNodeService.cpp (1612 lines)

**Current Status**: 1612 lines - HIGH PRIORITY

### Proposed Split:

#### Phase 1: Extract NodeConnectionService
**File**: `Services/BlueprintNode/NodeConnectionService.h/.cpp` (~450 lines)
**Methods to Copy**:
- `ConnectBlueprintNodes` (lines 176-286)
- `ConnectPins` (lines 841-907)
- `ConnectNodesWithAutoCast` (lines 908-1137)
- `ArePinTypesCompatible` (lines 1138-1180)

**Strategy**:
1. Create service class with connection logic
2. Copy-paste all connection methods
3. Use TFunction callback for type compatibility checks

---

#### Phase 2: Extract NodeCreationService
**File**: `Services/BlueprintNode/NodeCreationService.h/.cpp` (~400 lines)
**Methods to Copy**:
- `AddInputActionNode` (lines 287-320)
- `AddVariableNode` (lines 586-619)
- `AddEventNode` (lines 653-685)
- `AddFunctionCallNode` (lines 686-718)
- `AddCustomEventNode` (lines 719-799)

**Strategy**:
1. Create service for node creation
2. Copy-paste all Add*Node methods
3. Delegate from main service

---

#### Phase 3: Extract NodeQueryService
**File**: `Services/BlueprintNode/NodeQueryService.h/.cpp` (~300 lines)
**Methods to Copy**:
- `FindBlueprintNodes` (lines 321-555)
- `GetBlueprintGraphs` (lines 556-585)
- `GetVariableInfo` (lines 620-652)

**Strategy**:
1. Create query service for finding/getting node info
2. Copy-paste all query methods
3. Delegate from main service

---

#### Phase 4: Extract NodeCastingService
**File**: `Services/BlueprintNode/NodeCastingService.h/.cpp` (~400 lines)
**Methods to Copy**:
- `CreateCastNode` (lines 1181-1244)
- `CreateIntToStringCast` (lines 1245-1289)
- `CreateFloatToStringCast` (lines 1290-1332)
- `CreateBoolToStringCast` (lines 1333-1375)
- `CreateStringToIntCast` (lines 1376-1418)
- `CreateStringToFloatCast` (lines 1419-1461)
- `CreateObjectCast` (lines 1462-1588)

**Strategy**:
1. Create service for type casting operations
2. Copy-paste all Cast creation methods
3. Delegate from main service

---

### Expected Result for BlueprintNodeService:
- **Original**: 1612 lines
- **After Refactor**: ~300 lines (coordination)
- **Extracted Services**: 4 new service files (~1550 lines total)

---

## Priority 3: BlueprintNodeCreationService.cpp (1168 lines)

**Current Status**: 1168 lines - MEDIUM PRIORITY

### Analysis Needed:
- Read file structure to identify logical groupings
- Look for event creation, function call creation, operator creation
- Likely split into 2-3 specialized services

**Proposed Approach**:
1. Identify method categories (will analyze in detail when starting)
2. Extract into 2-3 specialized creation services
3. Target: Reduce to ~400 lines

---

## Priority 4: WidgetComponentService.cpp (573 lines)

**Current Status**: 573 lines - LOW PRIORITY (Already under 800)

**Action**: MONITOR ONLY
- Already below 800 line target
- Keep an eye during future development
- If grows past 800, split widget operations by type

---

## Refactoring Execution Order

1. **Start with UnrealMCPCommonUtils.cpp** (Highest impact, most reusable utilities)
2. **Then BlueprintNodeService.cpp** (Complex logic, high value refactor)
3. **Finally BlueprintNodeCreationService.cpp** (If time permits)

---

## Testing Strategy

For each refactored file:
1. Compile after each phase
2. Run existing MCP test suite
3. Verify no functionality lost
4. Update `.github/large-cpp-files.md`

---

## Success Metrics

- ✅ All files under 800 lines (max 1000)
- ✅ 100% functionality preserved (copy-paste, not rewrite)
- ✅ All tests pass
- ✅ Clean compilation with no warnings
- ✅ Improved code organization and maintainability

---

## Next Steps

1. User approval of this plan
2. Start with UnrealMCPCommonUtils.cpp Phase 1 (JsonUtils)
3. Progress through phases systematically
4. Test after each extraction
5. Update documentation
