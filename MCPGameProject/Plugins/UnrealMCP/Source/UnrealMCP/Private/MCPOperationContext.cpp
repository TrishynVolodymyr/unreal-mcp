#include "MCPOperationContext.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/Guid.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFilemanager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPOperationContext, Log, All);

FString FMCPEnhancedError::ToJsonString() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    
    // Base error information
    TSharedPtr<FJsonObject> BaseErrorObject = MakeShareable(new FJsonObject);
    BaseErrorObject->SetNumberField(TEXT("errorType"), static_cast<int32>(BaseError.ErrorType));
    BaseErrorObject->SetNumberField(TEXT("errorCode"), BaseError.ErrorCode);
    BaseErrorObject->SetStringField(TEXT("errorMessage"), BaseError.ErrorMessage);
    BaseErrorObject->SetStringField(TEXT("errorDetails"), BaseError.ErrorDetails);
    JsonObject->SetObjectField(TEXT("baseError"), BaseErrorObject);
    
    // Enhanced error information
    JsonObject->SetNumberField(TEXT("severity"), static_cast<int32>(Severity));
    JsonObject->SetStringField(TEXT("timestamp"), Timestamp.ToString());
    JsonObject->SetStringField(TEXT("sourceLocation"), SourceLocation);
    JsonObject->SetStringField(TEXT("stackTrace"), StackTrace);
    
    // Context information
    TSharedPtr<FJsonObject> ContextObject = MakeShareable(new FJsonObject);
    for (const auto& ContextPair : Context)
    {
        ContextObject->SetStringField(ContextPair.Key, ContextPair.Value);
    }
    JsonObject->SetObjectField(TEXT("context"), ContextObject);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

FMCPEnhancedError FMCPEnhancedError::FromJsonString(const FString& JsonString)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return FMCPEnhancedError(FMCPError(EMCPErrorType::InternalError, 1002, TEXT("Failed to parse enhanced error JSON")));
    }
    
    FMCPEnhancedError Error;
    
    // Parse base error
    const TSharedPtr<FJsonObject>* BaseErrorObject;
    if (JsonObject->TryGetObjectField(TEXT("baseError"), BaseErrorObject) && BaseErrorObject->IsValid())
    {
        int32 ErrorTypeInt = 0;
        (*BaseErrorObject)->TryGetNumberField(TEXT("errorType"), ErrorTypeInt);
        Error.BaseError.ErrorType = static_cast<EMCPErrorType>(ErrorTypeInt);
        
        (*BaseErrorObject)->TryGetNumberField(TEXT("errorCode"), Error.BaseError.ErrorCode);
        (*BaseErrorObject)->TryGetStringField(TEXT("errorMessage"), Error.BaseError.ErrorMessage);
        (*BaseErrorObject)->TryGetStringField(TEXT("errorDetails"), Error.BaseError.ErrorDetails);
    }
    
    // Parse enhanced information
    int32 SeverityInt = 0;
    if (JsonObject->TryGetNumberField(TEXT("severity"), SeverityInt))
    {
        Error.Severity = static_cast<EMCPErrorSeverity>(SeverityInt);
    }
    
    FString TimestampString;
    if (JsonObject->TryGetStringField(TEXT("timestamp"), TimestampString))
    {
        FDateTime::Parse(TimestampString, Error.Timestamp);
    }
    
    JsonObject->TryGetStringField(TEXT("sourceLocation"), Error.SourceLocation);
    JsonObject->TryGetStringField(TEXT("stackTrace"), Error.StackTrace);
    
    // Parse context
    const TSharedPtr<FJsonObject>* ContextObject;
    if (JsonObject->TryGetObjectField(TEXT("context"), ContextObject) && ContextObject->IsValid())
    {
        for (const auto& ContextPair : (*ContextObject)->Values)
        {
            FString Value;
            if (ContextPair.Value->TryGetString(Value))
            {
                Error.Context.Add(ContextPair.Key, Value);
            }
        }
    }
    
    return Error;
}

UMCPOperationContext::UMCPOperationContext()
    : OperationName(TEXT(""))
    , OperationId(TEXT(""))
    , StartTime(FDateTime::Now())
    , EndTime(FDateTime::MinValue())
    , bOperationCompleted(false)
{
}

void UMCPOperationContext::Initialize(const FString& InOperationName, const FString& InOperationId)
{
    OperationName = InOperationName;
    OperationId = InOperationId.IsEmpty() ? GenerateOperationId() : InOperationId;
    StartTime = FDateTime::Now();
    bOperationCompleted = false;
    
    // Clear any existing errors
    Errors.Empty();
    OperationContext.Empty();
    
    UE_LOG(LogMCPOperationContext, Verbose, TEXT("Initialized operation context: %s [%s]"), *OperationName, *OperationId);
}

void UMCPOperationContext::AddError(const FMCPError& Error, EMCPErrorSeverity Severity, const FString& SourceLocation)
{
    FMCPEnhancedError EnhancedError(Error, Severity);
    EnhancedError.SetSourceLocation(SourceLocation);
    
    // Add operation context to the error
    for (const auto& ContextPair : OperationContext)
    {
        EnhancedError.AddContext(ContextPair.Key, ContextPair.Value);
    }
    
    AddEnhancedError(EnhancedError);
}

void UMCPOperationContext::AddEnhancedError(const FMCPEnhancedError& Error)
{
    Errors.Add(Error);
    
    // Log the error based on severity
    FString LogMessage = FString::Printf(
        TEXT("Operation '%s' [%s] - %s: %s"),
        *OperationName,
        *OperationId,
        *SeverityToString(Error.Severity),
        *Error.BaseError.ErrorMessage
    );
    
    switch (Error.Severity)
    {
        case EMCPErrorSeverity::Info:
            UE_LOG(LogMCPOperationContext, Log, TEXT("%s"), *LogMessage);
            break;
        case EMCPErrorSeverity::Warning:
            UE_LOG(LogMCPOperationContext, Warning, TEXT("%s"), *LogMessage);
            break;
        case EMCPErrorSeverity::Error:
            UE_LOG(LogMCPOperationContext, Error, TEXT("%s"), *LogMessage);
            break;
        case EMCPErrorSeverity::Critical:
        case EMCPErrorSeverity::Fatal:
            UE_LOG(LogMCPOperationContext, Error, TEXT("CRITICAL: %s"), *LogMessage);
            break;
    }
}

void UMCPOperationContext::AddWarning(const FString& Warning, const FString& Details, const FString& SourceLocation)
{
    FMCPError WarningError(EMCPErrorType::ValidationFailed, 0, Warning, Details);
    AddError(WarningError, EMCPErrorSeverity::Warning, SourceLocation);
}

void UMCPOperationContext::AddInfo(const FString& Info, const FString& Details)
{
    FMCPError InfoError(EMCPErrorType::None, 0, Info, Details);
    AddError(InfoError, EMCPErrorSeverity::Info);
}

void UMCPOperationContext::AddOperationContext(const FString& Key, const FString& Value)
{
    OperationContext.Add(Key, Value);
}

bool UMCPOperationContext::HasErrors() const
{
    for (const FMCPEnhancedError& Error : Errors)
    {
        if (Error.Severity >= EMCPErrorSeverity::Error)
        {
            return true;
        }
    }
    return false;
}

bool UMCPOperationContext::HasWarnings() const
{
    for (const FMCPEnhancedError& Error : Errors)
    {
        if (Error.Severity == EMCPErrorSeverity::Warning)
        {
            return true;
        }
    }
    return false;
}

int32 UMCPOperationContext::GetErrorCount(EMCPErrorSeverity Severity) const
{
    int32 Count = 0;
    for (const FMCPEnhancedError& Error : Errors)
    {
        if (Error.Severity == Severity)
        {
            Count++;
        }
    }
    return Count;
}

TArray<FMCPEnhancedError> UMCPOperationContext::GetErrorsBySeverity(EMCPErrorSeverity Severity) const
{
    TArray<FMCPEnhancedError> FilteredErrors;
    for (const FMCPEnhancedError& Error : Errors)
    {
        if (Error.Severity == Severity)
        {
            FilteredErrors.Add(Error);
        }
    }
    return FilteredErrors;
}

FMCPEnhancedError UMCPOperationContext::GetMostSevereError() const
{
    if (Errors.Num() == 0)
    {
        return FMCPEnhancedError();
    }
    
    FMCPEnhancedError MostSevere = Errors[0];
    for (const FMCPEnhancedError& Error : Errors)
    {
        if (Error.Severity > MostSevere.Severity)
        {
            MostSevere = Error;
        }
    }
    
    return MostSevere;
}

FString UMCPOperationContext::CreateResponse(const FString& SuccessData, const FString& Metadata) const
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShareable(new FJsonObject);
    
    bool bSuccess = !HasErrors();
    ResponseObject->SetBoolField(TEXT("success"), bSuccess);
    ResponseObject->SetStringField(TEXT("operationName"), OperationName);
    ResponseObject->SetStringField(TEXT("operationId"), OperationId);
    ResponseObject->SetStringField(TEXT("data"), SuccessData);
    ResponseObject->SetStringField(TEXT("metadata"), Metadata);
    
    // Add timing information
    float Duration = GetOperationDuration();
    ResponseObject->SetNumberField(TEXT("duration"), Duration);
    
    // Add error information
    if (Errors.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> ErrorArray;
        for (const FMCPEnhancedError& Error : Errors)
        {
            TSharedPtr<FJsonObject> ErrorObject = MakeShareable(new FJsonObject);
            ErrorObject->SetNumberField(TEXT("severity"), static_cast<int32>(Error.Severity));
            ErrorObject->SetNumberField(TEXT("errorType"), static_cast<int32>(Error.BaseError.ErrorType));
            ErrorObject->SetNumberField(TEXT("errorCode"), Error.BaseError.ErrorCode);
            ErrorObject->SetStringField(TEXT("errorMessage"), Error.BaseError.ErrorMessage);
            ErrorObject->SetStringField(TEXT("errorDetails"), Error.BaseError.ErrorDetails);
            ErrorObject->SetStringField(TEXT("timestamp"), Error.Timestamp.ToString());
            ErrorObject->SetStringField(TEXT("sourceLocation"), Error.SourceLocation);
            
            // Add context
            if (Error.Context.Num() > 0)
            {
                TSharedPtr<FJsonObject> ContextObject = MakeShareable(new FJsonObject);
                for (const auto& ContextPair : Error.Context)
                {
                    ContextObject->SetStringField(ContextPair.Key, ContextPair.Value);
                }
                ErrorObject->SetObjectField(TEXT("context"), ContextObject);
            }
            
            ErrorArray.Add(MakeShareable(new FJsonValueObject(ErrorObject)));
        }
        ResponseObject->SetArrayField(TEXT("errors"), ErrorArray);
    }
    
    // Add operation statistics
    ResponseObject->SetStringField(TEXT("stats"), GetOperationStats());
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObject.ToSharedRef(), Writer);
    
    return OutputString;
}

FString UMCPOperationContext::CreateErrorSummary() const
{
    if (Errors.Num() == 0)
    {
        return TEXT("No errors or warnings");
    }
    
    TMap<EMCPErrorSeverity, int32> SeverityCounts;
    for (const FMCPEnhancedError& Error : Errors)
    {
        int32& Count = SeverityCounts.FindOrAdd(Error.Severity, 0);
        Count++;
    }
    
    TArray<FString> SummaryParts;
    for (const auto& CountPair : SeverityCounts)
    {
        SummaryParts.Add(FString::Printf(TEXT("%s: %d"), *SeverityToString(CountPair.Key), CountPair.Value));
    }
    
    return FString::Join(SummaryParts, TEXT(", "));
}

void UMCPOperationContext::Clear()
{
    Errors.Empty();
    OperationContext.Empty();
    bOperationCompleted = false;
    StartTime = FDateTime::Now();
    EndTime = FDateTime::MinValue();
}

float UMCPOperationContext::GetOperationDuration() const
{
    FDateTime EndTimeToUse = bOperationCompleted ? EndTime : FDateTime::Now();
    return (EndTimeToUse - StartTime).GetTotalSeconds();
}

void UMCPOperationContext::CompleteOperation()
{
    if (!bOperationCompleted)
    {
        EndTime = FDateTime::Now();
        bOperationCompleted = true;
        
        float Duration = GetOperationDuration();
        UE_LOG(LogMCPOperationContext, Verbose, TEXT("Completed operation '%s' [%s] in %.3f seconds"), 
               *OperationName, *OperationId, Duration);
    }
}

FString UMCPOperationContext::GetOperationStats() const
{
    TSharedPtr<FJsonObject> StatsObject = MakeShareable(new FJsonObject);
    
    StatsObject->SetStringField(TEXT("operationName"), OperationName);
    StatsObject->SetStringField(TEXT("operationId"), OperationId);
    StatsObject->SetNumberField(TEXT("duration"), GetOperationDuration());
    StatsObject->SetBoolField(TEXT("completed"), bOperationCompleted);
    StatsObject->SetNumberField(TEXT("totalErrors"), Errors.Num());
    StatsObject->SetNumberField(TEXT("criticalErrors"), GetErrorCount(EMCPErrorSeverity::Critical));
    StatsObject->SetNumberField(TEXT("errors"), GetErrorCount(EMCPErrorSeverity::Error));
    StatsObject->SetNumberField(TEXT("warnings"), GetErrorCount(EMCPErrorSeverity::Warning));
    StatsObject->SetNumberField(TEXT("info"), GetErrorCount(EMCPErrorSeverity::Info));
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(StatsObject.ToSharedRef(), Writer);
    
    return OutputString;
}

FString UMCPOperationContext::GenerateOperationId() const
{
    return FGuid::NewGuid().ToString();
}

FString UMCPOperationContext::SeverityToString(EMCPErrorSeverity Severity) const
{
    switch (Severity)
    {
        case EMCPErrorSeverity::Info:
            return TEXT("INFO");
        case EMCPErrorSeverity::Warning:
            return TEXT("WARNING");
        case EMCPErrorSeverity::Error:
            return TEXT("ERROR");
        case EMCPErrorSeverity::Critical:
            return TEXT("CRITICAL");
        case EMCPErrorSeverity::Fatal:
            return TEXT("FATAL");
        default:
            return TEXT("UNKNOWN");
    }
}