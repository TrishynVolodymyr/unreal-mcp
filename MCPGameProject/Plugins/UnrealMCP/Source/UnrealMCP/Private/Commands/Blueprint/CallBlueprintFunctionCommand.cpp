#include "Commands/Blueprint/CallBlueprintFunctionCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Class.h"
#include "EngineUtils.h"

FCallBlueprintFunctionCommand::FCallBlueprintFunctionCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FCallBlueprintFunctionCommand::Execute(const FString& Parameters)
{
    FBlueprintFunctionCallParams Params;
    FString ParseError;
    
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Validate parameters
    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }
    
    // Find the target object
    UObject* TargetObject = nullptr;
    
    // First try to find it as an actor in the editor world
    UWorld* World = nullptr;
    if (GEngine)
    {
        // Try editor world first
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.WorldType == EWorldType::Editor)
            {
                World = Context.World();
                break;
            }
        }
        
        // If no editor world, try PIE world
        if (!World)
        {
            World = GEngine->GetCurrentPlayWorld();
        }
    }
    
    if (World)
    {
        for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
        {
            AActor* Actor = *ActorItr;
            if (Actor && Actor->GetName() == Params.TargetName)
            {
                TargetObject = Actor;
                break;
            }
        }
    }
    
    // If not found as actor, try to find it as any UObject
    if (!TargetObject)
    {
        TargetObject = FindObject<UObject>(nullptr, *Params.TargetName);
    }
    
    if (!TargetObject)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Target object '%s' not found"), *Params.TargetName));
    }
    
    // Find the function
    UFunction* Function = TargetObject->GetClass()->FindFunctionByName(*Params.FunctionName);
    if (!Function)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Function '%s' not found on object '%s'"), *Params.FunctionName, *Params.TargetName));
    }
    
    // Check if function is BlueprintCallable
    if (!Function->HasAnyFunctionFlags(FUNC_BlueprintCallable))
    {
        return CreateErrorResponse(FString::Printf(TEXT("Function '%s' is not BlueprintCallable"), *Params.FunctionName));
    }
    
    // For now, we only support functions with string parameters
    // Create parameter buffer
    uint8* ParamBuffer = nullptr;
    int32 ParamSize = Function->ParmsSize;
    
    if (ParamSize > 0)
    {
        ParamBuffer = (uint8*)FMemory::Malloc(ParamSize);
        FMemory::Memzero(ParamBuffer, ParamSize);
        
        // Initialize parameters
        int32 StringParamIndex = 0;
        for (TFieldIterator<FProperty> PropIt(Function); PropIt; ++PropIt)
        {
            FProperty* Prop = *PropIt;
            if (Prop->HasAnyPropertyFlags(CPF_Parm) && !Prop->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                // Only handle string parameters for now
                if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
                {
                    if (StringParamIndex < Params.StringParams.Num())
                    {
                        StrProp->SetPropertyValue_InContainer(ParamBuffer, Params.StringParams[StringParamIndex]);
                        StringParamIndex++;
                    }
                }
            }
        }
    }
    
    // Call the function
    try
    {
        TargetObject->ProcessEvent(Function, ParamBuffer);
        
        // For now, just return success - we could extract return values later
        FString Result = TEXT("Function called successfully");
        
        if (ParamBuffer)
        {
            FMemory::Free(ParamBuffer);
        }
        
        return CreateSuccessResponse(Result);
    }
    catch (...)
    {
        if (ParamBuffer)
        {
            FMemory::Free(ParamBuffer);
        }
        
        return CreateErrorResponse(TEXT("Exception occurred while calling function"));
    }
}

FString FCallBlueprintFunctionCommand::GetCommandName() const
{
    return TEXT("call_blueprint_function");
}

bool FCallBlueprintFunctionCommand::ValidateParams(const FString& Parameters) const
{
    FBlueprintFunctionCallParams Params;
    FString ParseError;
    
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FCallBlueprintFunctionCommand::ParseParameters(const FString& JsonString, FBlueprintFunctionCallParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required target_name parameter
    if (!JsonObject->TryGetStringField(TEXT("target_name"), OutParams.TargetName))
    {
        OutError = TEXT("Missing required 'target_name' parameter");
        return false;
    }
    
    // Parse required function_name parameter
    if (!JsonObject->TryGetStringField(TEXT("function_name"), OutParams.FunctionName))
    {
        OutError = TEXT("Missing required 'function_name' parameter");
        return false;
    }
    
    // Parse optional string_params array
    const TArray<TSharedPtr<FJsonValue>>* StringParamsArray;
    if (JsonObject->TryGetArrayField(TEXT("string_params"), StringParamsArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *StringParamsArray)
        {
            FString StringParam;
            if (Value->TryGetString(StringParam))
            {
                OutParams.StringParams.Add(StringParam);
            }
        }
    }
    
    return true;
}

FString FCallBlueprintFunctionCommand::CreateSuccessResponse(const FString& Result) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("result"), Result);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FCallBlueprintFunctionCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}



