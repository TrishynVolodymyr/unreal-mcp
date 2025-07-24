// Research Test: UE API patterns for editable function creation
// This file documents the findings from studying UE source code

#include "Engine/Blueprint.h"
#include "BlueprintGraph/Classes/K2Node_FunctionEntry.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

/*
RESEARCH FINDINGS - UE API Patterns for Editable Blueprint Functions

1. FUNCTION CREATION PATTERN:
   - Use FBlueprintEditorUtils::AddFunctionGraph() instead of manual graph creation
   - This method properly sets up the function in the Blueprint's function list
   - It creates the UFunction object with proper metadata linkage

2. METADATA KEYS DISCOVERED:
   - FBlueprintMetadata::MD_Tooltip = "ToolTip" (function description)
   - FBlueprintMetadata::MD_FunctionCategory = "Category" (function category)
   - FBlueprintMetadata::MD_CallInEditor = "CallInEditor" (editor callable)
   - FBlueprintMetadata::MD_BlueprintProtected = "BlueprintProtected"
   - FBlueprintMetadata::MD_BlueprintPrivate = "BlueprintPrivate"

3. FUNCTION FLAGS REQUIRED:
   - FUNC_BlueprintCallable: Makes function visible in Blueprint
   - FUNC_Public/Protected/Private: Access control
   - FUNC_BlueprintPure: For pure functions (no exec pins)
   - FUNC_Const: For const functions

4. CRITICAL API SEQUENCE:
   a) Create function graph using FBlueprintEditorUtils::CreateNewGraph()
   b) Add to Blueprint using FBlueprintEditorUtils::AddFunctionGraph()
   c) Get the auto-created UK2Node_FunctionEntry
   d) Apply metadata to the entry node using SetMetaData()
   e) Set function flags using SetExtraFlags()
   f) Configure user-defined pins
   g) Call AllocateDefaultPins() and ReconstructNode()
   h) Mark Blueprint as structurally modified

5. METADATA APPLICATION PATTERN:
   - Metadata must be set on the UK2Node_FunctionEntry node
   - Use EntryNode->MetaData.SetMetaData(Key, Value)
   - Metadata is automatically transferred to the UFunction during compilation

6. ACCESS SPECIFIER HANDLING:
   - Default is Public (FUNC_Public flag)
   - Protected: Set FUNC_Protected flag and MD_BlueprintProtected metadata
   - Private: Set FUNC_Private flag and MD_BlueprintPrivate metadata

7. EDITOR INTEGRATION:
   - Functions appear in Blueprint editor's Functions list automatically
   - Right-click context menu works if metadata is properly set
   - Function properties dialog requires proper UFunction configuration

VALIDATION NEEDED:
- Test metadata persistence after Blueprint compilation
- Verify function editability in Blueprint editor
- Confirm access specifier behavior
- Test with different Blueprint types (Actor, Widget, etc.)
*/

// VALIDATED METADATA KEYS (from UE source code analysis):
// These are the exact string constants used by UE's Blueprint system
namespace ValidatedMetadataKeys {
    static const FString MD_Tooltip = TEXT("ToolTip");                    // Function description
    static const FString MD_FunctionCategory = TEXT("Category");          // Function category
    static const FString MD_CallInEditor = TEXT("CallInEditor");          // Editor callable
    static const FString MD_BlueprintProtected = TEXT("BlueprintProtected"); // Protected access
    static const FString MD_BlueprintPrivate = TEXT("BlueprintPrivate");  // Private access
    static const FString MD_CompactNodeTitle = TEXT("CompactNodeTitle");  // Compact display name
    static const FString MD_Keywords = TEXT("Keywords");                  // Search keywords
}

// VALIDATED FUNCTION FLAGS (from UE source code analysis):
// These flags control function behavior and visibility
namespace ValidatedFunctionFlags {
    static const uint32 EDITABLE_FUNCTION_BASE = FUNC_BlueprintCallable | FUNC_Public;
    static const uint32 PURE_FUNCTION = FUNC_BlueprintPure;
    static const uint32 CONST_FUNCTION = FUNC_Const;
    static const uint32 PROTECTED_FUNCTION = FUNC_Protected;
    static const uint32 PRIVATE_FUNCTION = FUNC_Private;
    static const uint32 EDITOR_CALLABLE = FUNC_CallInEditor;
}

// Test implementation pattern based on research
class FunctionCreationTest {
public:
    // Enhanced function creation with full metadata support
    static bool CreateEditableFunction(UBlueprint* Blueprint, const FString& FunctionName, 
                                     const FString& Description = TEXT(""), 
                                     const FString& Category = TEXT("Default"),
                                     bool bIsPure = false,
                                     bool bIsConst = false,
                                     bool bCallInEditor = false,
                                     const FString& AccessSpecifier = TEXT("Public")) {
        
        // Step 1: Create function graph using proper UE API
        UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
            Blueprint,
            FName(*FunctionName),
            UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass()
        );
        
        if (!FuncGraph) {
            UE_LOG(LogTemp, Error, TEXT("Failed to create function graph for: %s"), *FunctionName);
            return false;
        }
        
        // Step 2: Add to Blueprint using proper method (this is key!)
        FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, FuncGraph, bIsPure, nullptr);
        
        // Step 3: Find the auto-created function entry node
        UK2Node_FunctionEntry* EntryNode = nullptr;
        for (UEdGraphNode* Node : FuncGraph->Nodes) {
            if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node)) {
                EntryNode = Entry;
                break;
            }
        }
        
        if (!EntryNode) {
            UE_LOG(LogTemp, Error, TEXT("Failed to find function entry node for: %s"), *FunctionName);
            return false;
        }
        
        // Step 4: Apply metadata (CRITICAL for editability)
        if (!Description.IsEmpty()) {
            EntryNode->MetaData.SetMetaData(ValidatedMetadataKeys::MD_Tooltip, Description);
            UE_LOG(LogTemp, Log, TEXT("Set function description: %s"), *Description);
        }
        
        if (!Category.IsEmpty()) {
            EntryNode->MetaData.SetMetaData(ValidatedMetadataKeys::MD_FunctionCategory, Category);
            UE_LOG(LogTemp, Log, TEXT("Set function category: %s"), *Category);
        }
        
        if (bCallInEditor) {
            EntryNode->MetaData.SetMetaData(ValidatedMetadataKeys::MD_CallInEditor, TEXT("true"));
            UE_LOG(LogTemp, Log, TEXT("Enabled CallInEditor for function: %s"), *FunctionName);
        }
        
        // Step 5: Set function flags based on parameters
        uint32 FunctionFlags = ValidatedFunctionFlags::EDITABLE_FUNCTION_BASE;
        
        if (bIsPure) {
            FunctionFlags |= ValidatedFunctionFlags::PURE_FUNCTION;
        }
        
        if (bIsConst) {
            FunctionFlags |= ValidatedFunctionFlags::CONST_FUNCTION;
        }
        
        if (bCallInEditor) {
            FunctionFlags |= ValidatedFunctionFlags::EDITOR_CALLABLE;
        }
        
        // Handle access specifiers
        if (AccessSpecifier == TEXT("Protected")) {
            FunctionFlags |= ValidatedFunctionFlags::PROTECTED_FUNCTION;
            EntryNode->MetaData.SetMetaData(ValidatedMetadataKeys::MD_BlueprintProtected, TEXT("true"));
        } else if (AccessSpecifier == TEXT("Private")) {
            FunctionFlags |= ValidatedFunctionFlags::PRIVATE_FUNCTION;
            EntryNode->MetaData.SetMetaData(ValidatedMetadataKeys::MD_BlueprintPrivate, TEXT("true"));
        }
        
        EntryNode->SetExtraFlags(FunctionFlags);
        UE_LOG(LogTemp, Log, TEXT("Set function flags: 0x%X for function: %s"), FunctionFlags, *FunctionName);
        
        // Step 6: Ensure proper node setup
        EntryNode->AllocateDefaultPins();
        EntryNode->ReconstructNode();
        
        // Step 7: Mark Blueprint as modified (triggers editor refresh)
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        
        UE_LOG(LogTemp, Log, TEXT("Successfully created editable function: %s"), *FunctionName);
        return true;
    }
    
    // Test function to validate metadata persistence
    static bool ValidateMetadataPersistence(UBlueprint* Blueprint, const FString& FunctionName) {
        // Find the function graph
        UEdGraph* FuncGraph = nullptr;
        for (UEdGraph* Graph : Blueprint->FunctionGraphs) {
            if (Graph && Graph->GetName() == FunctionName) {
                FuncGraph = Graph;
                break;
            }
        }
        
        if (!FuncGraph) {
            UE_LOG(LogTemp, Error, TEXT("Function graph not found: %s"), *FunctionName);
            return false;
        }
        
        // Find the function entry node
        UK2Node_FunctionEntry* EntryNode = nullptr;
        for (UEdGraphNode* Node : FuncGraph->Nodes) {
            if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node)) {
                EntryNode = Entry;
                break;
            }
        }
        
        if (!EntryNode) {
            UE_LOG(LogTemp, Error, TEXT("Function entry node not found: %s"), *FunctionName);
            return false;
        }
        
        // Check if metadata exists
        FString Description = EntryNode->MetaData.GetMetaData(ValidatedMetadataKeys::MD_Tooltip);
        FString Category = EntryNode->MetaData.GetMetaData(ValidatedMetadataKeys::MD_FunctionCategory);
        
        UE_LOG(LogTemp, Log, TEXT("Function %s - Description: %s, Category: %s"), 
               *FunctionName, *Description, *Category);
        
        return !Description.IsEmpty() || !Category.IsEmpty();
    }
};

/*
ANALYSIS OF CURRENT IMPLEMENTATION GAPS:

1. CURRENT IMPLEMENTATION ANALYSIS:
   - Current code uses FBlueprintEditorUtils::AddFunctionGraph() ✓ (CORRECT)
   - Sets FUNC_BlueprintCallable | FUNC_BlueprintEvent flags ✓ (CORRECT)
   - Sets MD_CallInEditor metadata ✓ (CORRECT)
   - BUT MISSING: Function description (MD_Tooltip)
   - BUT MISSING: Function category (MD_FunctionCategory)
   - BUT MISSING: Access specifier handling (Protected/Private)

2. SPECIFIC ISSUES IDENTIFIED:
   a) No description parameter support in HandleCreateCustomBlueprintFunction
   b) No category parameter support (has category but not applied to metadata)
   c) No access specifier parameter support
   d) Missing metadata keys for function properties dialog
   e) UMG binding functions also lack proper metadata

3. REQUIRED CHANGES:
   a) Add description parameter to create_custom_blueprint_function MCP tool
   b) Apply MD_Tooltip metadata when description is provided
   c) Apply MD_FunctionCategory metadata when category is provided
   d) Add access_specifier parameter support
   e) Apply MD_BlueprintProtected/MD_BlueprintPrivate metadata for access control
   f) Update UMG binding function creation to use same pattern

4. VALIDATION TESTS NEEDED:
   a) Create function with description and verify it appears in Blueprint editor
   b) Create function with category and verify it's organized correctly
   c) Create protected/private functions and verify access control
   d) Test metadata persistence after Blueprint compilation
   e) Test right-click context menu functionality
   f) Test function properties dialog editability
*/

// COMPARISON: Current vs Required Implementation
namespace ImplementationComparison {
    
    // CURRENT IMPLEMENTATION (from HandleCreateCustomBlueprintFunction):
    void CurrentImplementation() {
        // ✓ Creates graph correctly
        // UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(...)
        
        // ✓ Adds to Blueprint correctly  
        // FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, FuncGraph, bIsPure, nullptr);
        
        // ✓ Sets basic flags
        // EntryNode->SetExtraFlags(FUNC_BlueprintCallable | FUNC_BlueprintEvent);
        
        // ✓ Sets CallInEditor metadata
        // EntryNode->MetaData.SetMetaData(FBlueprintMetadata::MD_CallInEditor, FString(TEXT("true")));
        
        // ✗ MISSING: Description metadata
        // ✗ MISSING: Category metadata  
        // ✗ MISSING: Access specifier handling
    }
    
    // REQUIRED IMPLEMENTATION (based on research):
    void RequiredImplementation() {
        // All current functionality PLUS:
        
        // + Description metadata
        // if (!Description.IsEmpty()) {
        //     EntryNode->MetaData.SetMetaData(TEXT("ToolTip"), Description);
        // }
        
        // + Category metadata
        // if (!Category.IsEmpty()) {
        //     EntryNode->MetaData.SetMetaData(TEXT("Category"), Category);
        // }
        
        // + Access specifier handling
        // if (AccessSpecifier == TEXT("Protected")) {
        //     EntryNode->SetExtraFlags(EntryNode->GetExtraFlags() | FUNC_Protected);
        //     EntryNode->MetaData.SetMetaData(TEXT("BlueprintProtected"), TEXT("true"));
        // }
    }
}

// TEST CASE DEFINITIONS for validation
namespace TestCases {
    
    struct FunctionTestCase {
        FString Name;
        FString Description;
        FString Category;
        FString AccessSpecifier;
        bool bIsPure;
        bool bExpectedEditable;
    };
    
    // Test cases to validate the implementation
    static TArray<FunctionTestCase> GetValidationTestCases() {
        return {
            // Basic function with description
            {TEXT("TestBasicFunction"), TEXT("A basic test function"), TEXT("Testing"), TEXT("Public"), false, true},
            
            // Pure function with category
            {TEXT("TestPureFunction"), TEXT("A pure calculation function"), TEXT("Math"), TEXT("Public"), true, true},
            
            // Protected function
            {TEXT("TestProtectedFunction"), TEXT("A protected helper function"), TEXT("Helpers"), TEXT("Protected"), false, true},
            
            // Private function
            {TEXT("TestPrivateFunction"), TEXT("A private internal function"), TEXT("Internal"), TEXT("Private"), false, true},
            
            // Function without description (should still be editable)
            {TEXT("TestMinimalFunction"), TEXT(""), TEXT("Default"), TEXT("Public"), false, true}
        };
    }
}