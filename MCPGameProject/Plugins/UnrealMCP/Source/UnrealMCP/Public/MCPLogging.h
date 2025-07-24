#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFilemanager.h"

// Forward declarations
class UMCPOperationContext;

/**
 * Declare the main UnrealMCP log category with all verbosity levels
 */
DECLARE_LOG_CATEGORY_EXTERN(LogUnrealMCP, Log, All);

/**
 * Declare specialized log categories for different MCP subsystems
 */
DECLARE_LOG_CATEGORY_EXTERN(LogMCPCommands, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogMCPServices, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogMCPValidation, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogMCPPerformance, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogMCPNetwork, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogMCPOperations, Log, All);

/**
 * Enumeration for different types of MCP operations for logging categorization
 */
enum class EMCPLogOperationType : uint8
{
    Command,
    Service,
    Validation,
    Network,
    Performance,
    General
};

/**
 * Structure for operation timing and performance logging
 */
struct UNREALMCP_API FMCPPerformanceLog
{
    FString OperationName;
    FString OperationId;
    FDateTime StartTime;
    FDateTime EndTime;
    float Duration;
    TMap<FString, FString> Metadata;
    
    FMCPPerformanceLog()
        : OperationName(TEXT(""))
        , OperationId(TEXT(""))
        , StartTime(FDateTime::Now())
        , EndTime(FDateTime::MinValue())
        , Duration(0.0f)
    {
    }
    
    FMCPPerformanceLog(const FString& InOperationName, const FString& InOperationId = TEXT(""))
        : OperationName(InOperationName)
        , OperationId(InOperationId)
        , StartTime(FDateTime::Now())
        , EndTime(FDateTime::MinValue())
        , Duration(0.0f)
    {
    }
    
    void Complete()
    {
        EndTime = FDateTime::Now();
        Duration = (EndTime - StartTime).GetTotalSeconds();
    }
    
    void AddMetadata(const FString& Key, const FString& Value)
    {
        Metadata.Add(Key, Value);
    }
    
    FString ToString() const
    {
        return FString::Printf(TEXT("Operation: %s [%s] Duration: %.3fs"), 
                              *OperationName, *OperationId, Duration);
    }
};

/**
 * Comprehensive logging utility for MCP operations
 * Provides structured logging, performance tracking, and operation tracing
 */
class UNREALMCP_API FMCPLogger
{
public:
    /**
     * Initialize the MCP logging system
     * @param bEnableFileLogging Whether to enable logging to files
     * @param LogDirectory Directory for log files (empty for default)
     */
    static void Initialize(bool bEnableFileLogging = true, const FString& LogDirectory = TEXT(""));

    /**
     * Shutdown the MCP logging system and flush all logs
     */
    static void Shutdown();

    /**
     * Log a structured message with operation context
     * @param Category Log category to use
     * @param Verbosity Log verbosity level
     * @param OperationType Type of operation being logged
     * @param OperationId Unique identifier for the operation
     * @param Message Main log message
     * @param Details Additional details
     * @param Context Optional context information
     */
    static void LogStructured(const FLogCategoryBase& Category, ELogVerbosity::Type Verbosity,
                            EMCPLogOperationType OperationType, const FString& OperationId,
                            const FString& Message, const FString& Details = TEXT(""),
                            const TMap<FString, FString>& Context = TMap<FString, FString>());

    /**
     * Log operation start with automatic timing
     * @param OperationName Name of the operation
     * @param OperationId Unique identifier for the operation
     * @param Parameters Operation parameters for context
     * @return Performance log structure for tracking
     */
    static FMCPPerformanceLog LogOperationStart(const FString& OperationName, 
                                               const FString& OperationId = TEXT(""),
                                               const TMap<FString, FString>& Parameters = TMap<FString, FString>());

    /**
     * Log operation completion with timing information
     * @param PerformanceLog The performance log from LogOperationStart
     * @param bSuccess Whether the operation was successful
     * @param ResultSummary Summary of the operation result
     */
    static void LogOperationEnd(FMCPPerformanceLog& PerformanceLog, bool bSuccess, 
                              const FString& ResultSummary = TEXT(""));

    /**
     * Log performance metrics for an operation
     * @param OperationName Name of the operation
     * @param Duration Duration in seconds
     * @param Metrics Additional performance metrics
     */
    static void LogPerformanceMetrics(const FString& OperationName, float Duration,
                                    const TMap<FString, float>& Metrics = TMap<FString, float>());

    /**
     * Log network operation details
     * @param Operation Network operation type (send/receive/connect/etc.)
     * @param Endpoint Network endpoint information
     * @param DataSize Size of data transferred
     * @param Duration Operation duration
     * @param bSuccess Whether the operation was successful
     */
    static void LogNetworkOperation(const FString& Operation, const FString& Endpoint,
                                  int32 DataSize, float Duration, bool bSuccess);

    /**
     * Log validation results
     * @param ValidationType Type of validation performed
     * @param InputData Summary of input data being validated
     * @param bPassed Whether validation passed
     * @param ErrorDetails Details of validation errors (if any)
     */
    static void LogValidation(const FString& ValidationType, const FString& InputData,
                            bool bPassed, const FString& ErrorDetails = TEXT(""));

    /**
     * Log command execution details
     * @param CommandName Name of the command being executed
     * @param Parameters Command parameters
     * @param ExecutionTime Time taken to execute
     * @param bSuccess Whether execution was successful
     * @param ResultSummary Summary of execution result
     */
    static void LogCommandExecution(const FString& CommandName, const FString& Parameters,
                                  float ExecutionTime, bool bSuccess, const FString& ResultSummary = TEXT(""));

    /**
     * Log service operation details
     * @param ServiceName Name of the service
     * @param Operation Operation being performed
     * @param Context Operation context
     * @param bSuccess Whether the operation was successful
     */
    static void LogServiceOperation(const FString& ServiceName, const FString& Operation,
                                  const TMap<FString, FString>& Context, bool bSuccess);

    /**
     * Enable or disable debug logging for troubleshooting
     * @param bEnable Whether to enable debug logging
     */
    static void SetDebugLoggingEnabled(bool bEnable);

    /**
     * Check if debug logging is enabled
     * @return True if debug logging is enabled
     */
    static bool IsDebugLoggingEnabled();

    /**
     * Flush all pending log messages to files
     */
    static void FlushLogs();

    /**
     * Get logging statistics
     * @return JSON string containing logging statistics
     */
    static FString GetLoggingStatistics();

    /**
     * Set the minimum log level for file logging
     * @param MinLevel Minimum log level to write to files
     */
    static void SetFileLoggingLevel(ELogVerbosity::Type MinLevel);

    /**
     * Add a custom log sink for external log processing
     * @param SinkName Name of the log sink
     * @param SinkFunction Function to call for each log message
     */
    static void AddLogSink(const FString& SinkName, TFunction<void(const FString&, ELogVerbosity::Type)> SinkFunction);

    /**
     * Remove a custom log sink
     * @param SinkName Name of the log sink to remove
     */
    static void RemoveLogSink(const FString& SinkName);

private:
    /** Whether the logging system has been initialized */
    static bool bInitialized;

    /** Whether file logging is enabled */
    static bool bFileLoggingEnabled;

    /** Whether debug logging is enabled */
    static bool bDebugLoggingEnabled;

    /** Directory for log files */
    static FString LogDirectory;

    /** Minimum log level for file logging */
    static ELogVerbosity::Type FileLoggingLevel;

    /** Custom log sinks */
    static TMap<FString, TFunction<void(const FString&, ELogVerbosity::Type)>> LogSinks;

    /** Logging statistics */
    static TMap<FString, int32> LoggingStats;

    /** Critical section for thread-safe logging */
    static FCriticalSection LoggingCriticalSection;

    /** Generate a unique operation ID */
    static FString GenerateOperationId();

    /** Format context information for logging */
    static FString FormatContext(const TMap<FString, FString>& Context);

    /** Write log message to file */
    static void WriteToFile(const FString& LogMessage, ELogVerbosity::Type Verbosity);

    /** Update logging statistics */
    static void UpdateLoggingStats(const FString& Category, ELogVerbosity::Type Verbosity);

    /** Get log file path for a specific category */
    static FString GetLogFilePath(const FString& Category);
};

/**
 * RAII class for automatic operation timing and logging
 */
class UNREALMCP_API FMCPScopedOperationLogger
{
public:
    FMCPScopedOperationLogger(const FString& OperationName, const FString& OperationId = TEXT(""),
                             const TMap<FString, FString>& Parameters = TMap<FString, FString>());
    
    ~FMCPScopedOperationLogger();

    /** Mark the operation as successful */
    void SetSuccess(bool bSuccess);

    /** Add result summary */
    void SetResultSummary(const FString& Summary);

    /** Add metadata to the operation log */
    void AddMetadata(const FString& Key, const FString& Value);

private:
    FMCPPerformanceLog PerformanceLog;
    bool bOperationSuccess;
    FString ResultSummary;
};

/**
 * Structured logging macros for different severity levels and categories
 */

// General MCP logging macros
#define UE_LOG_MCP_ERROR(Format, ...) \
    UE_LOG(LogUnrealMCP, Error, TEXT("[MCP] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_WARNING(Format, ...) \
    UE_LOG(LogUnrealMCP, Warning, TEXT("[MCP] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_INFO(Format, ...) \
    UE_LOG(LogUnrealMCP, Log, TEXT("[MCP] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_VERBOSE(Format, ...) \
    UE_LOG(LogUnrealMCP, Verbose, TEXT("[MCP] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_VERYVERBOSE(Format, ...) \
    UE_LOG(LogUnrealMCP, VeryVerbose, TEXT("[MCP] ") Format, ##__VA_ARGS__)

// Command-specific logging macros
#define UE_LOG_MCP_COMMAND_ERROR(Format, ...) \
    UE_LOG(LogMCPCommands, Error, TEXT("[CMD] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_COMMAND_WARNING(Format, ...) \
    UE_LOG(LogMCPCommands, Warning, TEXT("[CMD] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_COMMAND_INFO(Format, ...) \
    UE_LOG(LogMCPCommands, Log, TEXT("[CMD] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_COMMAND_VERBOSE(Format, ...) \
    UE_LOG(LogMCPCommands, Verbose, TEXT("[CMD] ") Format, ##__VA_ARGS__)

// Service-specific logging macros
#define UE_LOG_MCP_SERVICE_ERROR(Format, ...) \
    UE_LOG(LogMCPServices, Error, TEXT("[SVC] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_SERVICE_WARNING(Format, ...) \
    UE_LOG(LogMCPServices, Warning, TEXT("[SVC] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_SERVICE_INFO(Format, ...) \
    UE_LOG(LogMCPServices, Log, TEXT("[SVC] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_SERVICE_VERBOSE(Format, ...) \
    UE_LOG(LogMCPServices, Verbose, TEXT("[SVC] ") Format, ##__VA_ARGS__)

// Validation-specific logging macros
#define UE_LOG_MCP_VALIDATION_ERROR(Format, ...) \
    UE_LOG(LogMCPValidation, Error, TEXT("[VAL] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_VALIDATION_WARNING(Format, ...) \
    UE_LOG(LogMCPValidation, Warning, TEXT("[VAL] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_VALIDATION_INFO(Format, ...) \
    UE_LOG(LogMCPValidation, Log, TEXT("[VAL] ") Format, ##__VA_ARGS__)

// Performance logging macros
#define UE_LOG_MCP_PERFORMANCE_INFO(Format, ...) \
    UE_LOG(LogMCPPerformance, Log, TEXT("[PERF] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_PERFORMANCE_WARNING(Format, ...) \
    UE_LOG(LogMCPPerformance, Warning, TEXT("[PERF] ") Format, ##__VA_ARGS__)

// Network logging macros
#define UE_LOG_MCP_NETWORK_ERROR(Format, ...) \
    UE_LOG(LogMCPNetwork, Error, TEXT("[NET] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_NETWORK_WARNING(Format, ...) \
    UE_LOG(LogMCPNetwork, Warning, TEXT("[NET] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_NETWORK_INFO(Format, ...) \
    UE_LOG(LogMCPNetwork, Log, TEXT("[NET] ") Format, ##__VA_ARGS__)

// Operation logging macros
#define UE_LOG_MCP_OPERATION_ERROR(Format, ...) \
    UE_LOG(LogMCPOperations, Error, TEXT("[OP] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_OPERATION_WARNING(Format, ...) \
    UE_LOG(LogMCPOperations, Warning, TEXT("[OP] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_OPERATION_INFO(Format, ...) \
    UE_LOG(LogMCPOperations, Log, TEXT("[OP] ") Format, ##__VA_ARGS__)

#define UE_LOG_MCP_OPERATION_VERBOSE(Format, ...) \
    UE_LOG(LogMCPOperations, Verbose, TEXT("[OP] ") Format, ##__VA_ARGS__)

// Structured logging macros with context
#define UE_LOG_MCP_STRUCTURED(Category, Verbosity, OperationType, OperationId, Message, Details, Context) \
    FMCPLogger::LogStructured(Category, ELogVerbosity::Verbosity, OperationType, OperationId, Message, Details, Context)

// Scoped operation logging macro
#define MCP_SCOPED_OPERATION_LOG(OperationName, ...) \
    FMCPScopedOperationLogger ANONYMOUS_VARIABLE(ScopedOpLog)(OperationName, ##__VA_ARGS__)

// Debug logging macros (only active when debug logging is enabled)
#define UE_LOG_MCP_DEBUG(Format, ...) \
    do { \
        if (FMCPLogger::IsDebugLoggingEnabled()) { \
            UE_LOG(LogUnrealMCP, VeryVerbose, TEXT("[DEBUG] ") Format, ##__VA_ARGS__); \
        } \
    } while(0)

#define UE_LOG_MCP_TRACE(Format, ...) \
    do { \
        if (FMCPLogger::IsDebugLoggingEnabled()) { \
            UE_LOG(LogUnrealMCP, VeryVerbose, TEXT("[TRACE] %s:%d - ") Format, TEXT(__FUNCTION__), __LINE__, ##__VA_ARGS__); \
        } \
    } while(0)
