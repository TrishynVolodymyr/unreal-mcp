#include "Commands/Blueprint/AddEventDispatcherCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"

FAddEventDispatcherCommand::FAddEventDispatcherCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FAddEventDispatcherCommand::Execute(const FString& Parameters)
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

    FString DispatcherName;
    if (!JsonObject->TryGetStringField(TEXT("dispatcher_name"), DispatcherName))
    {
        return CreateErrorResponse(TEXT("Missing 'dispatcher_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FName DispatcherFName(*DispatcherName);

    // Check if dispatcher already exists
    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == DispatcherFName)
        {
            if (Variable.VarType.PinCategory == UEdGraphSchema_K2::PC_MCDelegate)
            {
                return CreateErrorResponse(FString::Printf(
                    TEXT("Event Dispatcher '%s' already exists in Blueprint '%s'"),
                    *DispatcherName, *BlueprintName));
            }
            else
            {
                return CreateErrorResponse(FString::Printf(
                    TEXT("A variable named '%s' already exists in Blueprint '%s'. Cannot create Event Dispatcher with the same name."),
                    *DispatcherName, *BlueprintName));
            }
        }
    }

    // Step 1: Add the multicast delegate variable
    Blueprint->Modify();
    
    FEdGraphPinType DelegateType;
    DelegateType.PinCategory = UEdGraphSchema_K2::PC_MCDelegate;
    
    const bool bVarCreated = FBlueprintEditorUtils::AddMemberVariable(Blueprint, DispatcherFName, DelegateType);
    if (!bVarCreated)
    {
        return CreateErrorResponse(FString::Printf(
            TEXT("Failed to create Event Dispatcher variable '%s' in Blueprint '%s'"),
            *DispatcherName, *BlueprintName));
    }

    // Step 2: Create the delegate signature graph
    UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, DispatcherFName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    
    if (!NewGraph)
    {
        // Rollback: remove the variable we just created
        FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, DispatcherFName);
        return CreateErrorResponse(FString::Printf(
            TEXT("Failed to create delegate signature graph for '%s'. Variable has been rolled back."),
            *DispatcherName));
    }

    NewGraph->bEditable = false;

    // Step 3: Set up the graph with default nodes and function terminators
    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
    K2Schema->CreateDefaultNodesForGraph(*NewGraph);
    K2Schema->CreateFunctionGraphTerminators(*NewGraph, (UClass*)nullptr);
    K2Schema->AddExtraFunctionFlags(NewGraph, (FUNC_BlueprintCallable | FUNC_BlueprintEvent | FUNC_Public));
    K2Schema->MarkFunctionEntryAsEditable(NewGraph, true);

    // Step 4: Register the graph
    Blueprint->DelegateSignatureGraphs.Add(NewGraph);
    
    // Step 5: Mark as structurally modified (important for proper refresh)
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

    UE_LOG(LogTemp, Display, TEXT("AddEventDispatcher: Successfully created Event Dispatcher '%s' in Blueprint '%s'"),
        *DispatcherName, *BlueprintName);

    return CreateSuccessResponse(BlueprintName, DispatcherName);
}

FString FAddEventDispatcherCommand::GetCommandName() const
{
    return TEXT("add_event_dispatcher");
}

bool FAddEventDispatcherCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }
    
    return JsonObject->HasField(TEXT("blueprint_name")) && 
           JsonObject->HasField(TEXT("dispatcher_name"));
}

FString FAddEventDispatcherCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& DispatcherName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("dispatcher_name"), DispatcherName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(
        TEXT("Event Dispatcher '%s' created successfully in Blueprint '%s'"), *DispatcherName, *BlueprintName));
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FAddEventDispatcherCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}
