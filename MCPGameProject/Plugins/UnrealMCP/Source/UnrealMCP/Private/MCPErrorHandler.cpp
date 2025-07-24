#include "MCPErrorHandler.h"
#include "MCPOperationContext.h"
#include "MCPLogging.h"
#include "Engine/Engine.h"
#include "Logging/LogMacros.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FMCPError FMCPErrorHandler::CreateInvalidParametersError(const FString& Details)
{
    return FMCPError(
        EMCPErrorType::InvalidParameters,
        GetNextErrorCode(EMCPErrorType::InvalidParameters),
        TEXT("Invalid parameters provided"),
        Details
    );
}

FMCPError FMCPErrorHandler::CreateCommandNotFoundError(const FString& CommandName)
{
    return FMCPError(
        EMCPErrorType::CommandNotFound,
        GetNextErrorCode(EMCPErrorType::CommandNotFound),
        FString::Printf(TEXT("Command '%s' not found"), *CommandName),
        FString::Printf(TEXT("The requested command '%s' is not registered in the command registry"), *CommandName)
    );
}

FMCPError FMCPErrorHandler::CreateExecutionFailedError(const FString& Details)
{
    return FMCPError(
        EMCPErrorType::ExecutionFailed,
        GetNextErrorCode(EMCPErrorType::ExecutionFailed),
        TEXT("Command execution failed"),
        Details
    );
}

FMCPError FMCPErrorHandler::CreateValidationFailedError(const FString& Details)
{
    return FMCPError(
        EMCPErrorType::ValidationFailed,
        GetNextErrorCode(EMCPErrorType::ValidationFailed),
        TEXT("Parameter validation failed"),
        Details
    );
}

FMCPError FMCPErrorHandler::CreateInternalError(const FString& Details)
{
    return FMCPError(
        EMCPErrorType::InternalError,
        GetNextErrorCode(EMCPErrorType::InternalError),
        TEXT("Internal system error"),
        Details
    );
}

void FMCPErrorHandler::LogError(const FMCPError& Error, const FLogCategoryBase* LogCategory)
{
    const FLogCategoryBase* Category = LogCategory ? LogCategory : &LogUnrealMCP;
    
    FString LogMessage = FString::Printf(
        TEXT("MCP Error [%d:%d] %s - %s"),
        static_cast<int32>(Error.ErrorType),
        Error.ErrorCode,
        *Error.ErrorMessage,
        *Error.ErrorDetails
    );
    
    switch (Error.ErrorType)
    {
        case EMCPErrorType::InvalidParameters:
        case EMCPErrorType::ValidationFailed:
            UE_LOG(LogUnrealMCP, Warning, TEXT("%s"), *LogMessage);
            break;
        case EMCPErrorType::CommandNotFound:
            UE_LOG(LogUnrealMCP, Warning, TEXT("%s"), *LogMessage);
            break;
        case EMCPErrorType::ExecutionFailed:
        case EMCPErrorType::InternalError:
        case EMCPErrorType::NetworkError:
        case EMCPErrorType::TimeoutError:
            UE_LOG(LogUnrealMCP, Error, TEXT("%s"), *LogMessage);
            break;
        default:
            UE_LOG(LogUnrealMCP, Log, TEXT("%s"), *LogMessage);
            break;
    }
}

void FMCPErrorHandler::HandleError(const FMCPError& Error, bool bShouldCrash)
{
    LogError(Error);
    
    if (bShouldCrash && (Error.ErrorType == EMCPErrorType::InternalError))
    {
        UE_LOG(LogUnrealMCP, Fatal, TEXT("Critical MCP error encountered: %s"), *Error.ErrorMessage);
    }
}

int32 FMCPErrorHandler::GetNextErrorCode(EMCPErrorType ErrorType)
{
    static TMap<EMCPErrorType, int32> ErrorCodeCounters;
    
    int32 BaseCode = static_cast<int32>(ErrorType) * 1000;
    int32& Counter = ErrorCodeCounters.FindOrAdd(ErrorType, 0);
    
    return BaseCode + (++Counter);
}

// Enhanced error handling methods implementation

FMCPEnhancedError FMCPErrorHandler::CreateEnhancedError(const FMCPError& BaseError, EMCPErrorSeverity Severity, 
                                                       const FString& SourceLocation, 
                                                       const TMap<FString, FString>& Context)
{
    FMCPEnhancedError EnhancedError(BaseError, Severity);
    EnhancedError.SetSourceLocation(SourceLocation);
    
    for (const auto& ContextPair : Context)
    {
        EnhancedError.AddContext(ContextPair.Key, ContextPair.Value);
    }
    
    return EnhancedError;
}

void FMCPErrorHandler::AddErrorToContext(UMCPOperationContext* Context, const FMCPError& Error, 
                                       const FString& SourceLocation)
{
    if (!Context)
    {
        UE_LOG(LogUnrealMCP, Warning, TEXT("Attempted to add error to null operation context"));
        return;
    }
    
    EMCPErrorSeverity Severity = DetermineErrorSeverity(Error.ErrorType);
    Context->AddError(Error, Severity, SourceLocation);
}

void FMCPErrorHandler::AddEnhancedErrorToContext(UMCPOperationContext* Context, const FMCPEnhancedError& Error)
{
    if (!Context)
    {
        UE_LOG(LogUnrealMCP, Warning, TEXT("Attempted to add enhanced error to null operation context"));
        return;
    }
    
    Context->AddEnhancedError(Error);
}

FString FMCPErrorHandler::CreateContextualResponse(UMCPOperationContext* Context, 
                                                 const FString& SuccessData, 
                                                 const FString& Metadata)
{
    if (!Context)
    {
        // Create a basic error response if no context is available
        FMCPError NoContextError = CreateInternalError(TEXT("No operation context available"));
        return CreateStructuredErrorResponse(NoContextError);
    }
    
    return Context->CreateResponse(SuccessData, Metadata);
}

EMCPErrorSeverity FMCPErrorHandler::DetermineErrorSeverity(EMCPErrorType ErrorType)
{
    return GetDefaultSeverityForErrorType(ErrorType);
}

FString FMCPErrorHandler::CreateStructuredErrorResponse(const FMCPError& Error, 
                                                      const FString& RequestId, 
                                                      const TMap<FString, FString>& AdditionalContext)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShareable(new FJsonObject);
    
    ResponseObject->SetBoolField(TEXT("success"), false);
    ResponseObject->SetStringField(TEXT("requestId"), RequestId);
    ResponseObject->SetStringField(TEXT("timestamp"), FDateTime::Now().ToString());
    
    // Error information
    TSharedPtr<FJsonObject> ErrorObject = MakeShareable(new FJsonObject);
    ErrorObject->SetNumberField(TEXT("errorType"), static_cast<int32>(Error.ErrorType));
    ErrorObject->SetNumberField(TEXT("errorCode"), Error.ErrorCode);
    ErrorObject->SetStringField(TEXT("errorMessage"), Error.ErrorMessage);
    ErrorObject->SetStringField(TEXT("errorDetails"), Error.ErrorDetails);
    ErrorObject->SetNumberField(TEXT("severity"), static_cast<int32>(DetermineErrorSeverity(Error.ErrorType)));
    
    // Additional context
    if (AdditionalContext.Num() > 0)
    {
        TSharedPtr<FJsonObject> ContextObject = MakeShareable(new FJsonObject);
        for (const auto& ContextPair : AdditionalContext)
        {
            ContextObject->SetStringField(ContextPair.Key, ContextPair.Value);
        }
        ErrorObject->SetObjectField(TEXT("context"), ContextObject);
    }
    
    ResponseObject->SetObjectField(TEXT("error"), ErrorObject);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObject.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FMCPErrorHandler::CreateAggregatedErrorResponse(const TArray<FMCPError>& Errors, 
                                                      const FString& OperationName)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShareable(new FJsonObject);
    
    ResponseObject->SetBoolField(TEXT("success"), false);
    ResponseObject->SetStringField(TEXT("operationName"), OperationName);
    ResponseObject->SetStringField(TEXT("timestamp"), FDateTime::Now().ToString());
    ResponseObject->SetNumberField(TEXT("errorCount"), Errors.Num());
    
    // Create array of errors
    TArray<TSharedPtr<FJsonValue>> ErrorArray;
    for (const FMCPError& Error : Errors)
    {
        TSharedPtr<FJsonObject> ErrorObject = MakeShareable(new FJsonObject);
        ErrorObject->SetNumberField(TEXT("errorType"), static_cast<int32>(Error.ErrorType));
        ErrorObject->SetNumberField(TEXT("errorCode"), Error.ErrorCode);
        ErrorObject->SetStringField(TEXT("errorMessage"), Error.ErrorMessage);
        ErrorObject->SetStringField(TEXT("errorDetails"), Error.ErrorDetails);
        ErrorObject->SetNumberField(TEXT("severity"), static_cast<int32>(DetermineErrorSeverity(Error.ErrorType)));
        
        ErrorArray.Add(MakeShareable(new FJsonValueObject(ErrorObject)));
    }
    ResponseObject->SetArrayField(TEXT("errors"), ErrorArray);
    
    // Add summary statistics
    TMap<EMCPErrorType, int32> ErrorTypeCounts;
    TMap<EMCPErrorSeverity, int32> SeverityCounts;
    
    for (const FMCPError& Error : Errors)
    {
        int32& TypeCount = ErrorTypeCounts.FindOrAdd(Error.ErrorType, 0);
        TypeCount++;
        
        EMCPErrorSeverity Severity = DetermineErrorSeverity(Error.ErrorType);
        int32& SeverityCount = SeverityCounts.FindOrAdd(Severity, 0);
        SeverityCount++;
    }
    
    TSharedPtr<FJsonObject> SummaryObject = MakeShareable(new FJsonObject);
    SummaryObject->SetNumberField(TEXT("criticalErrors"), SeverityCounts.FindRef(EMCPErrorSeverity::Critical));
    SummaryObject->SetNumberField(TEXT("errors"), SeverityCounts.FindRef(EMCPErrorSeverity::Error));
    SummaryObject->SetNumberField(TEXT("warnings"), SeverityCounts.FindRef(EMCPErrorSeverity::Warning));
    SummaryObject->SetNumberField(TEXT("info"), SeverityCounts.FindRef(EMCPErrorSeverity::Info));
    
    ResponseObject->SetObjectField(TEXT("summary"), SummaryObject);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObject.ToSharedRef(), Writer);
    
    return OutputString;
}

void FMCPErrorHandler::LogEnhancedError(const FMCPEnhancedError& Error, const FLogCategoryBase* LogCategory)
{
    const FLogCategoryBase* Category = LogCategory ? LogCategory : &LogUnrealMCP;
    
    FString LogMessage = FString::Printf(
        TEXT("Enhanced MCP Error [%d:%d] %s - %s"),
        static_cast<int32>(Error.BaseError.ErrorType),
        Error.BaseError.ErrorCode,
        *Error.BaseError.ErrorMessage,
        *Error.BaseError.ErrorDetails
    );
    
    // Add source location if available
    if (!Error.SourceLocation.IsEmpty())
    {
        LogMessage += FString::Printf(TEXT(" | Source: %s"), *Error.SourceLocation);
    }
    
    // Add context information
    if (Error.Context.Num() > 0)
    {
        LogMessage += FString::Printf(TEXT(" | Context: %s"), *FormatErrorContext(Error.Context));
    }
    
    // Add timestamp
    LogMessage += FString::Printf(TEXT(" | Time: %s"), *Error.Timestamp.ToString());
    
    switch (Error.Severity)
    {
        case EMCPErrorSeverity::Info:
            UE_LOG(LogUnrealMCP, Log, TEXT("%s"), *LogMessage);
            break;
        case EMCPErrorSeverity::Warning:
            UE_LOG(LogUnrealMCP, Warning, TEXT("%s"), *LogMessage);
            break;
        case EMCPErrorSeverity::Error:
            UE_LOG(LogUnrealMCP, Error, TEXT("%s"), *LogMessage);
            break;
        case EMCPErrorSeverity::Critical:
        case EMCPErrorSeverity::Fatal:
            UE_LOG(LogUnrealMCP, Error, TEXT("CRITICAL: %s"), *LogMessage);
            break;
    }
}

void FMCPErrorHandler::HandleContextErrors(UMCPOperationContext* Context, bool bShouldCrash)
{
    if (!Context)
    {
        return;
    }
    
    if (!Context->HasErrors())
    {
        return;
    }
    
    // Log all errors in the context
    TArray<FMCPEnhancedError> AllErrors = Context->GetErrorsBySeverity(EMCPErrorSeverity::Info);
    AllErrors.Append(Context->GetErrorsBySeverity(EMCPErrorSeverity::Warning));
    AllErrors.Append(Context->GetErrorsBySeverity(EMCPErrorSeverity::Error));
    AllErrors.Append(Context->GetErrorsBySeverity(EMCPErrorSeverity::Critical));
    AllErrors.Append(Context->GetErrorsBySeverity(EMCPErrorSeverity::Fatal));
    
    for (const FMCPEnhancedError& Error : AllErrors)
    {
        LogEnhancedError(Error);
    }
    
    // Check for critical errors that might require crashing
    if (bShouldCrash)
    {
        FMCPEnhancedError MostSevere = Context->GetMostSevereError();
        if (MostSevere.Severity >= EMCPErrorSeverity::Critical)
        {
            UE_LOG(LogUnrealMCP, Fatal, TEXT("Critical MCP error in operation context: %s"), 
                   *MostSevere.BaseError.ErrorMessage);
        }
    }
}

EMCPErrorSeverity FMCPErrorHandler::GetDefaultSeverityForErrorType(EMCPErrorType ErrorType)
{
    switch (ErrorType)
    {
        case EMCPErrorType::None:
            return EMCPErrorSeverity::Info;
        case EMCPErrorType::InvalidParameters:
        case EMCPErrorType::ValidationFailed:
            return EMCPErrorSeverity::Warning;
        case EMCPErrorType::CommandNotFound:
            return EMCPErrorSeverity::Warning;
        case EMCPErrorType::ExecutionFailed:
            return EMCPErrorSeverity::Error;
        case EMCPErrorType::InternalError:
            return EMCPErrorSeverity::Critical;
        case EMCPErrorType::NetworkError:
        case EMCPErrorType::TimeoutError:
            return EMCPErrorSeverity::Error;
        default:
            return EMCPErrorSeverity::Error;
    }
}

FString FMCPErrorHandler::FormatErrorContext(const TMap<FString, FString>& Context)
{
    if (Context.Num() == 0)
    {
        return TEXT("");
    }
    
    TArray<FString> ContextParts;
    for (const auto& ContextPair : Context)
    {
        ContextParts.Add(FString::Printf(TEXT("%s=%s"), *ContextPair.Key, *ContextPair.Value));
    }
    
    return FString::Join(ContextParts, TEXT(", "));
}