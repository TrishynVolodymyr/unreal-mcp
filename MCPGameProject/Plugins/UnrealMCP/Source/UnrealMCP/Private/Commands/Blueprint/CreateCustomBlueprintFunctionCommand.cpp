#include "Commands/Blueprint/CreateCustomBlueprintFunctionCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"
#include "Engine/UserDefinedStruct.h"
#include "UObject/StructOnScope.h"
#include "Services/BlueprintService.h"

FCreateCustomBlueprintFunctionCommand::FCreateCustomBlueprintFunctionCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FCreateCustomBlueprintFunctionCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }
    
    FString BlueprintName;
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }
    
    FString FunctionName;
    if (!JsonObject->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        return CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
    }
    
    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    // Get optional parameters
    bool bIsPure = false;
    JsonObject->TryGetBoolField(TEXT("is_pure"), bIsPure);
    
    FString Category = TEXT("Default");
    JsonObject->TryGetStringField(TEXT("category"), Category);
    
    // Check if a function graph with this name already exists
    UEdGraph* ExistingGraph = nullptr;
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == FunctionName)
        {
            ExistingGraph = Graph;
            break;
        }
    }

    if (ExistingGraph)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Function '%s' already exists in Blueprint '%s'"), *FunctionName, *BlueprintName));
    }

    // CRITICAL: Check if a function with this name is defined by any implemented interface
    // Interface functions get auto-generated stubs, and creating a custom function with the same name
    // causes a "Graph named 'X' already exists" compilation error that requires manual intervention
    for (const FBPInterfaceDescription& InterfaceDesc : Blueprint->ImplementedInterfaces)
    {
        if (InterfaceDesc.Interface)
        {
            // Check all functions defined by this interface
            for (TFieldIterator<UFunction> FuncIt(InterfaceDesc.Interface); FuncIt; ++FuncIt)
            {
                UFunction* InterfaceFunc = *FuncIt;
                if (InterfaceFunc && InterfaceFunc->GetName() == FunctionName)
                {
                    return CreateErrorResponse(FString::Printf(
                        TEXT("Cannot create function '%s' - a function with this name is already defined by interface '%s'. Use the interface's function graph instead."),
                        *FunctionName,
                        *InterfaceDesc.Interface->GetName()));
                }
            }
        }
    }
    
    // Create the function graph using the working UMG pattern
    UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint,
        FName(*FunctionName),
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass()
    );
    
    if (!FuncGraph)
    {
        return CreateErrorResponse(TEXT("Failed to create function graph"));
    }
    
    // Use the proper UE API to add the function graph — handles pure/impure setup,
    // graph flags, and creates the Entry node automatically.
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, FuncGraph, bIsPure, nullptr);
    
    // Graph editability flags (AddFunctionGraph sets some, but ensure all are set)
    FuncGraph->bAllowDeletion = true;
    FuncGraph->bAllowRenaming = true;
    
    // Find the Entry node created by AddFunctionGraph
    UK2Node_FunctionEntry* EntryNode = nullptr;
    for (UEdGraphNode* Node : FuncGraph->Nodes)
    {
        if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
        {
            EntryNode = Entry;
            break;
        }
    }
    
    if (!EntryNode)
    {
        return CreateErrorResponse(FString::Printf(TEXT("AddFunctionGraph did not create Entry node for '%s'"), *FunctionName));
    }
    
    // ALWAYS create a function result node — even for pure functions.
    // Pure functions still need internal exec flow (Entry→Return) for member variable access.
    UK2Node_FunctionResult* ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
    FuncGraph->AddNode(ResultNode, false, false);
    ResultNode->NodePosX = 400;
    ResultNode->NodePosY = 0;
    
    // Set category metadata if provided
    if (!Category.IsEmpty() && Category != TEXT("Default"))
    {
        EntryNode->MetaData.SetMetaData(FBlueprintMetadata::MD_FunctionCategory, Category);
    }
    
    // Clear any existing user defined pins to avoid duplicates
    EntryNode->UserDefinedPins.Empty();
    
    // Process input parameters
    const TArray<TSharedPtr<FJsonValue>>* InputsArray = nullptr;
    if (JsonObject->TryGetArrayField(TEXT("inputs"), InputsArray))
    {
        for (const auto& InputValue : *InputsArray)
        {
            const TSharedPtr<FJsonObject>& InputObj = InputValue->AsObject();
            if (InputObj.IsValid())
            {
                FString ParamName;
                FString ParamType;
                if (InputObj->TryGetStringField(TEXT("name"), ParamName) && InputObj->TryGetStringField(TEXT("type"), ParamType))
                {
                    // Convert string to pin type using dynamic type resolution
                    FEdGraphPinType PinType;
                    if (!FBlueprintService::Get().ConvertStringToPinType(ParamType, PinType))
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Failed to convert type '%s' for parameter '%s', using Float as default"), *ParamType, *ParamName);
                        PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
                        PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
                    }
                    
                    // Add input parameter to function entry
                    EntryNode->UserDefinedPins.Add(MakeShared<FUserPinInfo>());
                    FUserPinInfo& NewPin = *EntryNode->UserDefinedPins.Last();
                    NewPin.PinName = FName(*ParamName);
                    NewPin.PinType = PinType;
                    NewPin.DesiredPinDirection = EGPD_Output; // Entry node outputs are function inputs
                }
            }
        }
    }
    
    // Process output parameters
    const TArray<TSharedPtr<FJsonValue>>* OutputsArray = nullptr;
    if (JsonObject->TryGetArrayField(TEXT("outputs"), OutputsArray))
    {
        // Clear any existing user defined pins to avoid duplicates
        ResultNode->UserDefinedPins.Empty();
        
        for (const auto& OutputValue : *OutputsArray)
        {
            const TSharedPtr<FJsonObject>& OutputObj = OutputValue->AsObject();
            if (OutputObj.IsValid())
            {
                FString ParamName;
                FString ParamType;
                if (OutputObj->TryGetStringField(TEXT("name"), ParamName) && OutputObj->TryGetStringField(TEXT("type"), ParamType))
                {
                    // Convert string to pin type for output using dynamic type resolution
                    FEdGraphPinType PinType;
                    if (!FBlueprintService::Get().ConvertStringToPinType(ParamType, PinType))
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Failed to convert type '%s' for output parameter '%s', using Float as default"), *ParamType, *ParamName);
                        PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
                        PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
                    }
                    
                    // Add output parameter to function result
                    ResultNode->UserDefinedPins.Add(MakeShared<FUserPinInfo>());
                    FUserPinInfo& NewPin = *ResultNode->UserDefinedPins.Last();
                    NewPin.PinName = FName(*ParamName);
                    NewPin.PinType = PinType;
                    NewPin.DesiredPinDirection = EGPD_Input; // Result node inputs are function outputs
                }
            }
        }
        
        // Allocate pins for result node after adding all outputs
        ResultNode->AllocateDefaultPins();
        ResultNode->ReconstructNode();
    }
    else
    {
        // No outputs specified — still need to allocate default pins for exec flow
        ResultNode->AllocateDefaultPins();
        ResultNode->ReconstructNode();
    }
    
    // Allocate pins for entry node AFTER setting up user defined pins
    EntryNode->AllocateDefaultPins();
    
    // Reconstruct the entry node to immediately update the visual representation
    EntryNode->ReconstructNode();
    
    // Force refresh the graph
    FuncGraph->NotifyGraphChanged();
    
    // Force the Blueprint to recognize this as a user-defined function
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    
    // Refresh the Blueprint to ensure the function is properly integrated
    FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
    
    // Invalidate MCP's internal blueprint metadata cache
    FBlueprintService::Get().InvalidateBlueprintCache(Blueprint->GetName());
    
    // CRITICAL FIX: Connect internal execution flow between Entry and Return nodes.
    // This MUST happen AFTER all ReconstructNode/RefreshAllNodes calls, because those
    // recreate pins and destroy any existing connections.
    // Without this, pure functions that read member variables get default values (0,0,0,0).
    // Even though pure functions have no EXTERNAL exec pins on the call node,
    // they still need INTERNAL exec flow for member variable access to work.
    {
        UEdGraphPin* EntryExecPin = nullptr;
        for (UEdGraphPin* Pin : EntryNode->Pins)
        {
            if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Output)
            {
                EntryExecPin = Pin;
                break;
            }
        }

        UEdGraphPin* ResultExecPin = nullptr;
        for (UEdGraphPin* Pin : ResultNode->Pins)
        {
            if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Input)
            {
                ResultExecPin = Pin;
                break;
            }
        }

        if (EntryExecPin && ResultExecPin)
        {
            EntryExecPin->MakeLinkTo(ResultExecPin);
            UE_LOG(LogTemp, Log, TEXT("CreateCustomBlueprintFunction: Connected internal exec flow Entry->Return for '%s' (pure=%d)"), *FunctionName, (int)bIsPure);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CreateCustomBlueprintFunction: Could not find exec pins for '%s' (Entry: %s, Result: %s)"),
                *FunctionName,
                EntryExecPin ? TEXT("Found") : TEXT("Missing"),
                ResultExecPin ? TEXT("Found") : TEXT("Missing"));
        }
    }
    
    return CreateSuccessResponse(BlueprintName, FunctionName);
}

FString FCreateCustomBlueprintFunctionCommand::GetCommandName() const
{
    return TEXT("create_custom_blueprint_function");
}

bool FCreateCustomBlueprintFunctionCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }
    
    FString BlueprintName, FunctionName;
    return JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName) &&
           JsonObject->TryGetStringField(TEXT("function_name"), FunctionName);
}

bool FCreateCustomBlueprintFunctionCommand::ParseParameters(const FString& JsonString, 
                                                          FString& OutBlueprintName,
                                                          FString& OutFunctionName,
                                                          TArray<FFunctionParameter>& OutInputs,
                                                          TArray<FFunctionParameter>& OutOutputs,
                                                          bool& OutIsPure,
                                                          bool& OutIsConst,
                                                          FString& OutAccessSpecifier,
                                                          FString& OutCategory,
                                                          FString& OutError) const
{
    // This method is no longer used in the direct implementation
    OutError = TEXT("Method not implemented");
    return false;
}

bool FCreateCustomBlueprintFunctionCommand::ParseParameterArray(const TArray<TSharedPtr<FJsonValue>>& JsonArray, TArray<FFunctionParameter>& OutParameters) const
{
    // This method is no longer used in the direct implementation
    return false;
}

FString FCreateCustomBlueprintFunctionCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& FunctionName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("function_name"), FunctionName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created custom function '%s' in blueprint '%s'"), *FunctionName, *BlueprintName));
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FCreateCustomBlueprintFunctionCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}



