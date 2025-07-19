#include "Commands/CompileBlueprintCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/DateTime.h"

FCompileBlueprintCommand::FCompileBlueprintCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FCompileBlueprintCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the blueprint
    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }
    
    // Record compilation start time
    double StartTime = FPlatformTime::Seconds();
    
    // Compile the blueprint using the service
    FString CompilationError;
    bool bCompilationSuccess = BlueprintService.CompileBlueprint(Blueprint, CompilationError);
    
    // Calculate compilation time
    double EndTime = FPlatformTime::Seconds();
    float CompilationTime = static_cast<float>(EndTime - StartTime);
    
    if (!bCompilationSuccess)
    {
        // Extract detailed compilation errors
        TArray<FString> DetailedErrors = ExtractCompilationErrors(Blueprint);
        if (!CompilationError.IsEmpty())
        {
            DetailedErrors.Insert(CompilationError, 0);
        }
        
        return CreateErrorResponse(TEXT("Blueprint compilation failed"), DetailedErrors);
    }
    
    return CreateSuccessResponse(BlueprintName, CompilationTime);
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

FString FCompileBlueprintCommand::CreateSuccessResponse(const FString& BlueprintName, float CompilationTime) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetNumberField(TEXT("compilation_time_seconds"), CompilationTime);
    ResponseObj->SetStringField(TEXT("status"), TEXT("compiled successfully"));
    
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
        Errors.Add(TEXT("Blueprint is in error state"));
    }
    
    // Extract errors from compilation log if available
    // This would need access to the compilation log system
    // For now, we'll provide basic error information
    
    if (Blueprint->BlueprintType == BPTYPE_MacroLibrary && Blueprint->MacroGraphs.Num() == 0)
    {
        Errors.Add(TEXT("Macro library blueprint missing macro graphs"));
    }
    
    if (Blueprint->BlueprintType == BPTYPE_Interface && !Blueprint->FunctionGraphs.Num())
    {
        Errors.Add(TEXT("Interface blueprint has no function graphs"));
    }
    
    // Check for missing parent class
    if (!Blueprint->ParentClass)
    {
        Errors.Add(TEXT("Blueprint missing parent class"));
    }
    
    return Errors;
}