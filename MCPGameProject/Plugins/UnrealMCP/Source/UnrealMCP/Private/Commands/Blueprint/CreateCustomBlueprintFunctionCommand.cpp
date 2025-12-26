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
    
    // Use the proper method to add a user-defined function (like the Blueprint editor does)
    Blueprint->FunctionGraphs.Add(FuncGraph);
    
    // CRITICAL: Set the graph as user-defined to make it editable
    FuncGraph->bEditable = true;
    FuncGraph->bAllowDeletion = true;
    FuncGraph->bAllowRenaming = true;
    
    // Mark the graph as a user-defined function graph (this is key for editability)
    FuncGraph->GraphGuid = FGuid::NewGuid();
    
    // Create function entry node manually (like Blueprint editor does)
    UK2Node_FunctionEntry* EntryNode = NewObject<UK2Node_FunctionEntry>(FuncGraph);
    FuncGraph->AddNode(EntryNode, true, true);
    
    // Create function result node for non-pure functions
    UK2Node_FunctionResult* ResultNode = nullptr;
    if (!bIsPure)
    {
        ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
        FuncGraph->AddNode(ResultNode, true, true);
        ResultNode->NodePosX = 400;
        ResultNode->NodePosY = 0;
    }
    
    // Position the entry node
    EntryNode->NodePosX = 0;
    EntryNode->NodePosY = 0;
    
    // Set up the function signature properly
    EntryNode->CustomGeneratedFunctionName = FName(*FunctionName);
    EntryNode->bIsEditable = true;
    
    // Set function flags
    uint32 FunctionFlags = FUNC_BlueprintCallable;
    if (bIsPure)
    {
        FunctionFlags |= FUNC_BlueprintPure;
    }
    EntryNode->SetExtraFlags(FunctionFlags);
    
    // Set metadata to ensure the function is properly editable
    EntryNode->MetaData.SetMetaData(FBlueprintMetadata::MD_CallInEditor, FString(TEXT("true")));
    EntryNode->MetaData.SetMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly, FString(TEXT("false")));
    
    // Set category metadata if provided
    if (!Category.IsEmpty() && Category != TEXT("Default"))
    {
        EntryNode->MetaData.SetMetaData(FBlueprintMetadata::MD_FunctionCategory, Category);
    }
    
    // CRITICAL: Mark the entry node as user-defined
    EntryNode->bCanRenameNode = true;
    
    // Ensure the function entry node is properly configured
    // Note: Function entry nodes are automatically configured by the Blueprint system
    
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
        // Use the result node we created earlier, or create one if pure function has outputs
        if (!ResultNode && OutputsArray->Num() > 0)
        {
            ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
            FuncGraph->AddNode(ResultNode, true, true);
            ResultNode->NodePosX = 400;
            ResultNode->NodePosY = 0;
        }
        
        if (ResultNode)
        {
            // Clear any existing user defined pins to avoid duplicates
            ResultNode->UserDefinedPins.Empty();
        }
        
        for (const auto& OutputValue : *OutputsArray)
        {
            const TSharedPtr<FJsonObject>& OutputObj = OutputValue->AsObject();
            if (OutputObj.IsValid() && ResultNode)
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
        if (ResultNode)
        {
            ResultNode->AllocateDefaultPins();
            // Reconstruct the result node to immediately update the visual representation
            ResultNode->ReconstructNode();
        }
    }
    
    // Allocate pins for entry node AFTER setting up user defined pins
    EntryNode->AllocateDefaultPins();
    
    // Reconstruct the entry node to immediately update the visual representation
    EntryNode->ReconstructNode();
    
    // Force refresh the graph
    FuncGraph->NotifyGraphChanged();
    
    // CRITICAL: Reconstruct and refresh the function to ensure proper setup
    EntryNode->ReconstructNode();
    
    // Force the Blueprint to recognize this as a user-defined function
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    
    // Refresh the Blueprint to ensure the function is properly integrated
    FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
    
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



