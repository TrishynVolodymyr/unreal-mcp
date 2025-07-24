#include "MCPLogging.h"
#include "MCPParameterValidator.h"

// Define the log category
DEFINE_LOG_CATEGORY(LogUnrealMCP);

// Stub implementations for FMCPLogger
void FMCPLogger::Initialize(bool bEnableDebugLogging, const FString& LogFilePath)
{
    // Stub implementation
}

void FMCPLogger::Shutdown()
{
    // Stub implementation
}

void FMCPLogger::SetDebugLoggingEnabled(bool bEnabled)
{
    // Stub implementation
}

// Stub implementations for FParameterValidator
void FParameterValidator::AddRule(const FValidationRule& Rule)
{
    // Stub implementation
}

bool FParameterValidator::ValidateParams(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
    // Stub implementation - always return true
    return true;
}

void FParameterValidator::ClearRules()
{
    // Stub implementation
}