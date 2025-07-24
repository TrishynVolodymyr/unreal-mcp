// Minimal test case to validate UE metadata patterns
// This file contains a minimal implementation to test the metadata application

#include "Engine/Blueprint.h"
#include "BlueprintGraph/Classes/K2Node_FunctionEntry.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"

// Test function that applies the validated metadata pattern
bool TestMetadataApplication(UBlueprint* Blueprint) {
    if (!Blueprint) {
        UE_LOG(LogTemp, Error, TEXT("Blueprint is null"));
        return false;
    }
    
    const FString TestFunctionName = TEXT("TestEditableFunction");
    const FString TestDescription = TEXT("This is a test function with metadata");
    const FString TestCategory = TEXT("TestCategory");
    
    // Step 1: Create function graph
    UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint,
        FName(*TestFunctionName),
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass()
    );
    
    if (!FuncGraph) {
        UE_LOG(LogTemp, Error, TEXT("Failed to create function graph"));
        return false;
    }
    
    // Step 2: Add to Blueprint (this creates the UFunction)
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, FuncGraph, false, nullptr);
    
    // Step 3: Find the function entry node
    UK2Node_FunctionEntry* EntryNode = nullptr;
    for (UEdGraphNode* Node : FuncGraph->Nodes) {
        if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node)) {
            EntryNode = Entry;
            break;
        }
    }
    
    if (!EntryNode) {
        UE_LOG(LogTemp, Error, TEXT("Failed to find function entry node"));
        return false;
    }
    
    // Step 4: Apply metadata using validated keys
    EntryNode->MetaData.SetMetaData(TEXT("ToolTip"), TestDescription);
    EntryNode->MetaData.SetMetaData(TEXT("Category"), TestCategory);
    EntryNode->MetaData.SetMetaData(TEXT("CallInEditor"), TEXT("true"));
    
    // Step 5: Set function flags
    EntryNode->SetExtraFlags(FUNC_BlueprintCallable | FUNC_Public);
    
    // Step 6: Finalize node setup
    EntryNode->AllocateDefaultPins();
    EntryNode->ReconstructNode();
    
    // Step 7: Mark Blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    
    UE_LOG(LogTemp, Log, TEXT("Successfully created test function with metadata"));
    
    // Validation: Check if metadata was applied
    FString AppliedDescription = EntryNode->MetaData.GetMetaData(TEXT("ToolTip"));
    FString AppliedCategory = EntryNode->MetaData.GetMetaData(TEXT("Category"));
    
    bool bSuccess = (AppliedDescription == TestDescription) && (AppliedCategory == TestCategory);
    
    UE_LOG(LogTemp, Log, TEXT("Metadata validation - Description: %s, Category: %s, Success: %s"), 
           *AppliedDescription, *AppliedCategory, bSuccess ? TEXT("TRUE") : TEXT("FALSE"));
    
    return bSuccess;
}

// Test function to validate that the function appears in Blueprint editor
bool ValidateFunctionEditability(UBlueprint* Blueprint, const FString& FunctionName) {
    // Check if function exists in Blueprint's function list
    bool bFoundInFunctionGraphs = false;
    for (UEdGraph* Graph : Blueprint->FunctionGraphs) {
        if (Graph && Graph->GetName() == FunctionName) {
            bFoundInFunctionGraphs = true;
            break;
        }
    }
    
    if (!bFoundInFunctionGraphs) {
        UE_LOG(LogTemp, Error, TEXT("Function %s not found in Blueprint function graphs"), *FunctionName);
        return false;
    }
    
    // Check if UFunction was created (this is what makes it editable)
    UFunction* GeneratedFunction = nullptr;
    if (Blueprint->GeneratedClass) {
        GeneratedFunction = Blueprint->GeneratedClass->FindFunctionByName(FName(*FunctionName));
    }
    
    if (!GeneratedFunction) {
        UE_LOG(LogTemp, Warning, TEXT("UFunction not yet generated for %s (may need compilation)"), *FunctionName);
        // This is not necessarily an error - UFunction is created during compilation
    } else {
        UE_LOG(LogTemp, Log, TEXT("UFunction found for %s - function should be editable"), *FunctionName);
    }
    
    return bFoundInFunctionGraphs;
}

/*
VALIDATION CHECKLIST:

1. ✓ Function graph creation using FBlueprintEditorUtils::CreateNewGraph()
2. ✓ Function addition using FBlueprintEditorUtils::AddFunctionGraph()
3. ✓ Metadata application using EntryNode->MetaData.SetMetaData()
4. ✓ Function flags setting using EntryNode->SetExtraFlags()
5. ✓ Blueprint structural modification using MarkBlueprintAsStructurallyModified()

EXPECTED RESULTS:
- Function should appear in Blueprint editor's Functions list
- Right-click should show context menu with function properties
- Function properties dialog should show description and category
- Function should be fully editable in Blueprint graph editor

NEXT STEPS:
1. Integrate this pattern into HandleCreateCustomBlueprintFunction
2. Add description and access_specifier parameters to MCP tool
3. Update UMG binding function creation to use same pattern
4. Test with actual Blueprint editor to verify editability
*/