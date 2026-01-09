#include "Commands/Niagara/SetModuleColorCurveInputCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetModuleColorCurveInputCommand::FSetModuleColorCurveInputCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetModuleColorCurveInputCommand::Execute(const FString& Parameters)
{
    FNiagaraModuleColorCurveInputParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    bool bSuccess = NiagaraService.SetModuleColorCurveInput(Params, Error);

    if (!bSuccess)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.ModuleName, Params.InputName, Params.Keyframes.Num());
}

FString FSetModuleColorCurveInputCommand::GetCommandName() const
{
    return TEXT("set_module_color_curve_input");
}

bool FSetModuleColorCurveInputCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraModuleColorCurveInputParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FSetModuleColorCurveInputCommand::ParseParameters(const FString& JsonString, FNiagaraModuleColorCurveInputParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    // Required parameters
    if (!JsonObject->TryGetStringField(TEXT("system_path"), OutParams.SystemPath))
    {
        OutError = TEXT("Missing 'system_path' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("emitter_name"), OutParams.EmitterName))
    {
        OutError = TEXT("Missing 'emitter_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("module_name"), OutParams.ModuleName))
    {
        OutError = TEXT("Missing 'module_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("stage"), OutParams.Stage))
    {
        OutError = TEXT("Missing 'stage' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("input_name"), OutParams.InputName))
    {
        OutError = TEXT("Missing 'input_name' parameter");
        return false;
    }

    // Parse keyframes array
    const TArray<TSharedPtr<FJsonValue>>* KeyframesArray = nullptr;
    if (!JsonObject->TryGetArrayField(TEXT("keyframes"), KeyframesArray))
    {
        OutError = TEXT("Missing 'keyframes' array parameter");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& KeyframeValue : *KeyframesArray)
    {
        const TSharedPtr<FJsonObject>* KeyframeObj = nullptr;
        if (!KeyframeValue->TryGetObject(KeyframeObj) || !KeyframeObj->IsValid())
        {
            OutError = TEXT("Invalid keyframe object in array");
            return false;
        }

        FNiagaraColorCurveKeyframe Keyframe;

        if (!(*KeyframeObj)->TryGetNumberField(TEXT("time"), Keyframe.Time))
        {
            OutError = TEXT("Missing 'time' field in keyframe");
            return false;
        }

        // RGBA values - default to 1.0 if not provided
        if (!(*KeyframeObj)->TryGetNumberField(TEXT("r"), Keyframe.R))
        {
            Keyframe.R = 1.0f;
        }
        if (!(*KeyframeObj)->TryGetNumberField(TEXT("g"), Keyframe.G))
        {
            Keyframe.G = 1.0f;
        }
        if (!(*KeyframeObj)->TryGetNumberField(TEXT("b"), Keyframe.B))
        {
            Keyframe.B = 1.0f;
        }
        if (!(*KeyframeObj)->TryGetNumberField(TEXT("a"), Keyframe.A))
        {
            Keyframe.A = 1.0f;
        }

        OutParams.Keyframes.Add(Keyframe);
    }

    // Validate using the struct's validation
    return OutParams.IsValid(OutError);
}

FString FSetModuleColorCurveInputCommand::CreateSuccessResponse(const FString& ModuleName, const FString& InputName, int32 KeyframeCount) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("module_name"), ModuleName);
    ResponseObj->SetStringField(TEXT("input_name"), InputName);
    ResponseObj->SetNumberField(TEXT("keyframe_count"), KeyframeCount);
    ResponseObj->SetStringField(TEXT("message"),
        FString::Printf(TEXT("Set color curve input '%s' on module '%s' with %d keyframes"),
            *InputName, *ModuleName, KeyframeCount));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetModuleColorCurveInputCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
