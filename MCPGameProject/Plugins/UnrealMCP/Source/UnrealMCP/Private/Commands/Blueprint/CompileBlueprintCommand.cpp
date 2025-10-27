#include "Commands/Blueprint/CompileBlueprintCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/DateTime.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"

FCompileBlueprintCommand::FCompileBlueprintCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FCompileBlueprintCommand::Execute(const FString& Parameters)
{
    UE_LOG(LogTemp, Warning, TEXT("CompileBlueprintCommand::Execute called with parameters: %s"), *Parameters);
    
    FString BlueprintName;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, ParseError))
    {
        UE_LOG(LogTemp, Error, TEXT("CompileBlueprintCommand: Parameter parsing failed: %s"), *ParseError);
        return CreateErrorResponse(ParseError);
    }
    
    // Find the blueprint
    UE_LOG(LogTemp, Warning, TEXT("CompileBlueprintCommand: Looking for blueprint: %s"), *BlueprintName);
    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("CompileBlueprintCommand: Blueprint not found: %s"), *BlueprintName);
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }
    
    // Record compilation start time
    double StartTime = FPlatformTime::Seconds();
    
    // Compile the blueprint using the service
    UE_LOG(LogTemp, Warning, TEXT("CompileBlueprintCommand: Starting compilation of blueprint: %s"), *BlueprintName);
    FString CompilationError;
    bool bCompilationSuccess = BlueprintService.CompileBlueprint(Blueprint, CompilationError);
    
    // Calculate compilation time
    double EndTime = FPlatformTime::Seconds();
    float CompilationTime = static_cast<float>(EndTime - StartTime);
    
    UE_LOG(LogTemp, Warning, TEXT("CompileBlueprintCommand: Compilation result - Success: %s, Error: %s"), 
        bCompilationSuccess ? TEXT("true") : TEXT("false"), *CompilationError);
    
    if (!bCompilationSuccess)
    {
        // Extract detailed compilation errors
        UE_LOG(LogTemp, Warning, TEXT("CompileBlueprintCommand: Extracting detailed compilation errors"));
        TArray<FString> DetailedErrors = ExtractCompilationErrors(Blueprint);
        if (!CompilationError.IsEmpty())
        {
            DetailedErrors.Insert(CompilationError, 0);
        }
        
        UE_LOG(LogTemp, Warning, TEXT("CompileBlueprintCommand: Found %d detailed errors"), DetailedErrors.Num());
        for (int32 i = 0; i < DetailedErrors.Num(); i++)
        {
            UE_LOG(LogTemp, Warning, TEXT("CompileBlueprintCommand: Error %d: %s"), i, *DetailedErrors[i]);
        }
        
        return CreateErrorResponse(TEXT("Blueprint compilation failed"), DetailedErrors);
    }
    
    // Check if there were warnings
    FString StatusMessage = TEXT("compiled successfully");
    TArray<FString> Warnings;
    
    if (Blueprint->Status == BS_UpToDateWithWarnings && !CompilationError.IsEmpty())
    {
        StatusMessage = TEXT("compiled with warnings");
        Warnings.Add(CompilationError);
    }
    
    return CreateSuccessResponse(BlueprintName, CompilationTime, StatusMessage, Warnings);
}

FString FCompileBlueprintCommand::GetCommandName() const
{
    return TEXT("compile_blueprint");
}

bool FCompileBlueprintCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, ParseError);
}

bool FCompileBlueprintCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required blueprint_name parameter
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }
    
    return true;
}

FString FCompileBlueprintCommand::CreateSuccessResponse(const FString& BlueprintName, float CompilationTime, const FString& Status, const TArray<FString>& Warnings) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetNumberField(TEXT("compilation_time_seconds"), CompilationTime);
    ResponseObj->SetStringField(TEXT("status"), Status);
    
    // Add warnings if any
    if (Warnings.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> WarningArray;
        for (const FString& Warning : Warnings)
        {
            WarningArray.Add(MakeShared<FJsonValueString>(Warning));
        }
        ResponseObj->SetArrayField(TEXT("warnings"), WarningArray);
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FCompileBlueprintCommand::CreateErrorResponse(const FString& ErrorMessage, const TArray<FString>& CompilationErrors) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    // Add detailed compilation errors if available
    if (CompilationErrors.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> ErrorArray;
        for (const FString& Error : CompilationErrors)
        {
            ErrorArray.Add(MakeShared<FJsonValueString>(Error));
        }
        ResponseObj->SetArrayField(TEXT("compilation_errors"), ErrorArray);
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

TArray<FString> FCompileBlueprintCommand::ExtractCompilationErrors(UBlueprint* Blueprint) const
{
    TArray<FString> Errors;
    
    if (!Blueprint)
    {
        return Errors;
    }
    
    // Extract errors from the blueprint's status
    if (Blueprint->Status == BS_Error)
    {
        Errors.Add(FString::Printf(TEXT("Blueprint '%s' is in error state"), *Blueprint->GetName()));
        
        // Check for specific error conditions
        if (!Blueprint->ParentClass)
        {
            Errors.Add(TEXT("Missing parent class - Blueprint has no valid parent class"));
        }
        
        // Check for graph-related errors
        for (UEdGraph* Graph : Blueprint->UbergraphPages)
        {
            if (Graph)
            {
                // Check for nodes with compilation errors
                for (UEdGraphNode* Node : Graph->Nodes)
                {
                    if (Node && Node->bHasCompilerMessage)
                    {
                        FString NodeError = FString::Printf(TEXT("Node '%s' in graph '%s' has compilation error"), 
                            *Node->GetNodeTitle(ENodeTitleType::ListView).ToString(),
                            *Graph->GetName());
                        Errors.Add(NodeError);
                    }
                }
            }
        }
        
        // Check function graphs
        for (UEdGraph* FunctionGraph : Blueprint->FunctionGraphs)
        {
            if (FunctionGraph)
            {
                for (UEdGraphNode* Node : FunctionGraph->Nodes)
                {
                    if (Node && Node->bHasCompilerMessage)
                    {
                        FString NodeError = FString::Printf(TEXT("Function '%s' - Node '%s' has compilation error"), 
                            *FunctionGraph->GetName(),
                            *Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
                        Errors.Add(NodeError);
                    }
                }
            }
        }
    }
    
    // Check for blueprint type-specific issues
    if (Blueprint->BlueprintType == BPTYPE_MacroLibrary && Blueprint->MacroGraphs.Num() == 0)
    {
        Errors.Add(TEXT("Macro library blueprint missing macro graphs"));
    }
    
    if (Blueprint->BlueprintType == BPTYPE_Interface && Blueprint->FunctionGraphs.Num() == 0)
    {
        Errors.Add(TEXT("Interface blueprint has no function graphs"));
    }
    
    // Check for component-related issues
    if (Blueprint->SimpleConstructionScript)
    {
        TArray<USCS_Node*> AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
        for (USCS_Node* Node : AllNodes)
        {
            if (Node && Node->ComponentTemplate)
            {
                // Check if component class is valid
                if (!Node->ComponentTemplate->GetClass())
                {
                    Errors.Add(FString::Printf(TEXT("Component '%s' has invalid class"), 
                        *Node->GetVariableName().ToString()));
                }
            }
        }
    }
    
    // Check for variable-related issues
    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (!Variable.VarType.PinCategory.IsValid())
        {
            Errors.Add(FString::Printf(TEXT("Variable '%s' has invalid type"), 
                *Variable.VarName.ToString()));
        }
    }
    
    return Errors;
}



