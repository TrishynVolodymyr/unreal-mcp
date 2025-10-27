# UnrealMCP Plugin Coding Style Guide

## Overview

This document defines the coding standards and style guidelines for the UnrealMCP plugin. Following these guidelines ensures consistency, maintainability, and readability across the codebase.

## General Principles

1. **Consistency**: Follow established patterns within the codebase
2. **Clarity**: Write self-documenting code with clear intent
3. **Maintainability**: Structure code for easy modification and extension
4. **Performance**: Consider performance implications of design decisions
5. **Safety**: Use type-safe constructs and proper error handling

## File Organization

### Header Files (.h)

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"  // Engine includes
#include "Dom/JsonObject.h" // Third-party includes
#include "MyLocalHeader.h"  // Local includes
#include "MyFile.generated.h" // Generated file (if applicable)

// Forward declarations
class UBlueprint;
class FMyService;

/**
 * Brief description of the class
 * More detailed description if needed
 */
class UNREALMCP_API FMyClass
{
public:
    // Public interface
    
private:
    // Private implementation
};
```

### Implementation Files (.cpp)

```cpp
#include "MyFile.h"

#include "Engine/Blueprint.h"
#include "OtherHeaders.h"

// Static member definitions
bool FMyClass::bStaticMember = false;

// Implementation
FMyClass::FMyClass()
{
    // Constructor implementation
}
```

### Include Order

1. Own header file (for .cpp files)
2. Unreal Engine headers
3. Third-party library headers
4. Local project headers
5. Generated headers (last)

## Naming Conventions

### Classes and Structs

```cpp
// Classes use 'F' prefix for regular classes
class UNREALMCP_API FBlueprintService
{
};

// Interfaces use 'I' prefix
class UNREALMCP_API IBlueprintService
{
};

// UObject-derived classes use 'U' prefix
class UNREALMCP_API UMyUObject : public UObject
{
};

// Actor-derived classes use 'A' prefix
class UNREALMCP_API AMyActor : public AActor
{
};

// Structs use 'F' prefix
struct UNREALMCP_API FMyStruct
{
};

// Enums use 'E' prefix
enum class UNREALMCP_API EMyEnum : uint8
{
    None = 0,
    FirstValue,
    SecondValue
};
```

### Variables and Functions

```cpp
class FMyClass
{
public:
    /**
     * Public functions use PascalCase
     */
    void DoSomething();
    
    /**
     * Public member variables use PascalCase
     */
    int32 PublicVariable;
    
private:
    /**
     * Private member variables use camelCase with prefix
     */
    bool bBooleanMember;
    int32 IntegerMember;
    FString StringMember;
    
    /**
     * Private functions use PascalCase
     */
    void PrivateFunction();
};

// Local variables use PascalCase
void FMyClass::DoSomething()
{
    int32 LocalVariable = 0;
    bool bLocalFlag = true;
    FString LocalString = TEXT("Hello");
}
```

### Constants and Macros

```cpp
// Constants use ALL_CAPS with underscores
static const int32 MAX_RETRY_COUNT = 3;
static const FString DEFAULT_BLUEPRINT_NAME = TEXT("DefaultBlueprint");

// Macros use ALL_CAPS with underscores
#define MCP_LOG_ERROR(Format, ...) UE_LOG(LogUnrealMCP, Error, Format, ##__VA_ARGS__)
```

## Documentation Standards

### Class Documentation

```cpp
/**
 * Brief one-line description of the class
 * 
 * Detailed description explaining:
 * - Purpose and responsibilities
 * - Usage patterns
 * - Important considerations
 * - Thread safety notes if applicable
 * 
 * Example usage:
 * @code
 * FMyClass Instance;
 * Instance.DoSomething();
 * @endcode
 */
class UNREALMCP_API FMyClass
{
};
```

### Function Documentation

```cpp
/**
 * Brief description of what the function does
 * 
 * Detailed description if needed, explaining:
 * - Algorithm or approach used
 * - Side effects
 * - Performance considerations
 * 
 * @param InputParam Description of input parameter
 * @param OutputParam Description of output parameter
 * @return Description of return value
 * 
 * @note Important notes about usage
 * @warning Warnings about potential issues
 * @see Related functions or classes
 */
bool DoSomethingComplex(const FString& InputParam, FString& OutputParam);
```

### Member Variable Documentation

```cpp
class FMyClass
{
private:
    /** Brief description of the member variable */
    int32 SimpleVariable;
    
    /**
     * Detailed description for complex variables
     * Explain purpose, valid ranges, thread safety, etc.
     */
    TMap<FString, TSharedPtr<IMyInterface>> ComplexVariable;
};
```

## Code Structure

### Class Layout

```cpp
class UNREALMCP_API FMyClass
{
public:
    // Constructors and destructor
    FMyClass();
    explicit FMyClass(const FString& Name);
    virtual ~FMyClass();
    
    // Copy/move constructors and assignment operators
    FMyClass(const FMyClass& Other);
    FMyClass(FMyClass&& Other) noexcept;
    FMyClass& operator=(const FMyClass& Other);
    FMyClass& operator=(FMyClass&& Other) noexcept;
    
    // Public interface methods
    void PublicMethod();
    
    // Getters and setters
    const FString& GetName() const { return Name; }
    void SetName(const FString& InName) { Name = InName; }
    
    // Static methods
    static FMyClass CreateDefault();

protected:
    // Protected methods for inheritance
    virtual void ProtectedMethod();

private:
    // Private implementation methods
    void PrivateMethod();
    
    // Member variables
    FString Name;
    bool bInitialized;
};
```

### Function Implementation

```cpp
bool FMyClass::ComplexFunction(const FString& Input, FString& Output)
{
    // Early returns for error conditions
    if (Input.IsEmpty())
    {
        UE_LOG(LogUnrealMCP, Warning, TEXT("Input is empty"));
        return false;
    }
    
    // Local variables at the top
    bool bSuccess = false;
    FString ProcessedInput = Input.ToLower();
    
    // Main logic with clear structure
    if (ProcessedInput.StartsWith(TEXT("valid")))
    {
        Output = ProcessedInput + TEXT("_processed");
        bSuccess = true;
    }
    else
    {
        UE_LOG(LogUnrealMCP, Error, TEXT("Invalid input: %s"), *Input);
    }
    
    return bSuccess;
}
```

## Error Handling

### Use Structured Error Types

```cpp
// Good: Use structured error types
FMCPError Error(EMCPErrorType::ValidationFailed, 1001, 
               TEXT("Parameter validation failed"), 
               TEXT("The 'name' parameter is required"));

// Bad: Use raw strings
FString ErrorMessage = TEXT("Something went wrong");
```

### Proper Error Propagation

```cpp
bool FMyService::DoOperation(const FString& Input, FString& OutError)
{
    // Validate input
    if (Input.IsEmpty())
    {
        OutError = TEXT("Input cannot be empty");
        return false;
    }
    
    // Call sub-operations with error propagation
    FString SubError;
    if (!SubOperation(Input, SubError))
    {
        OutError = FString::Printf(TEXT("Sub-operation failed: %s"), *SubError);
        return false;
    }
    
    return true;
}
```

### Logging Best Practices

```cpp
// Use appropriate log levels
UE_LOG(LogUnrealMCP, Error, TEXT("Critical error occurred: %s"), *ErrorMessage);
UE_LOG(LogUnrealMCP, Warning, TEXT("Potential issue detected: %s"), *WarningMessage);
UE_LOG(LogUnrealMCP, Log, TEXT("Operation completed: %s"), *OperationName);
UE_LOG(LogUnrealMCP, Verbose, TEXT("Debug information: %s"), *DebugInfo);

// Include context in log messages
UE_LOG(LogUnrealMCP, Error, TEXT("Failed to create Blueprint '%s' in folder '%s': %s"), 
       *BlueprintName, *FolderPath, *ErrorMessage);
```

## Memory Management

### Smart Pointers

```cpp
// Use TSharedPtr for shared ownership
TSharedPtr<IMyInterface> SharedService = MakeShared<FMyService>();

// Use TUniquePtr for exclusive ownership
TUniquePtr<FMyData> UniqueData = MakeUnique<FMyData>();

// Use TWeakPtr to break circular references
TWeakPtr<IMyInterface> WeakReference = SharedService;

// Use raw pointers only for non-owning references
void ProcessObject(UObject* Object) // Object lifetime managed elsewhere
{
    if (IsValid(Object))
    {
        // Use object
    }
}
```

### RAII Pattern

```cpp
class FResourceManager
{
public:
    FResourceManager()
    {
        // Acquire resources in constructor
        Resource = AcquireResource();
    }
    
    ~FResourceManager()
    {
        // Release resources in destructor
        if (Resource)
        {
            ReleaseResource(Resource);
        }
    }
    
    // Delete copy constructor and assignment to prevent issues
    FResourceManager(const FResourceManager&) = delete;
    FResourceManager& operator=(const FResourceManager&) = delete;
    
private:
    void* Resource;
};
```

## Thread Safety

### Use Proper Synchronization

```cpp
class FThreadSafeClass
{
public:
    void ThreadSafeMethod()
    {
        FScopeLock Lock(&CriticalSection);
        // Thread-safe operations
        SharedData.Add(TEXT("NewItem"));
    }
    
    int32 GetCount() const
    {
        FScopeLock Lock(&CriticalSection);
        return SharedData.Num();
    }
    
private:
    mutable FCriticalSection CriticalSection;
    TArray<FString> SharedData;
};
```

### Atomic Operations

```cpp
class FAtomicCounter
{
public:
    void Increment()
    {
        Counter.IncrementExchange();
    }
    
    int32 GetValue() const
    {
        return Counter.GetValue();
    }
    
private:
    TAtomic<int32> Counter{0};
};
```

## Performance Guidelines

### Prefer Move Semantics

```cpp
// Good: Use move semantics for expensive objects
FString ProcessString(FString Input) // Pass by value for move
{
    // Process input
    Input.ToLowerInline();
    return MoveTemp(Input); // Move return value
}

// Usage
FString Result = ProcessString(MoveTemp(MyString));
```

### Minimize Allocations

```cpp
// Good: Reserve capacity for containers
TArray<FString> Items;
Items.Reserve(ExpectedCount);

// Good: Use stack allocation for small, temporary objects
FString TempString;
TempString.Reserve(256); // Reserve reasonable capacity

// Bad: Frequent reallocations
for (int32 i = 0; i < 1000; ++i)
{
    Items.Add(FString::Printf(TEXT("Item_%d"), i)); // May cause reallocations
}
```

### Cache Expensive Operations

```cpp
class FExpensiveOperationCache
{
public:
    FString GetProcessedValue(const FString& Input)
    {
        if (FString* CachedValue = Cache.Find(Input))
        {
            return *CachedValue;
        }
        
        FString ProcessedValue = ExpensiveOperation(Input);
        Cache.Add(Input, ProcessedValue);
        return ProcessedValue;
    }
    
private:
    TMap<FString, FString> Cache;
    FString ExpensiveOperation(const FString& Input);
};
```

## Testing Guidelines

### Unit Test Structure

```cpp
// Test class naming: [ClassUnderTest]Test
class FBlueprintServiceTest
{
public:
    void RunTests()
    {
        TestCreateBlueprint_ValidParams_ReturnsBlueprint();
        TestCreateBlueprint_InvalidParams_ReturnsNull();
        TestCreateBlueprint_DuplicateName_ReturnsError();
    }
    
private:
    // Test method naming: Test[MethodName]_[Scenario]_[ExpectedResult]
    void TestCreateBlueprint_ValidParams_ReturnsBlueprint()
    {
        // Arrange
        FBlueprintCreationParams Params;
        Params.Name = TEXT("TestBlueprint");
        Params.ParentClass = AActor::StaticClass();
        
        // Act
        UBlueprint* Result = BlueprintService->CreateBlueprint(Params);
        
        // Assert
        check(Result != nullptr);
        check(Result->GetName() == TEXT("TestBlueprint"));
    }
    
    TSharedPtr<IBlueprintService> BlueprintService;
};
```

### Mock Objects

```cpp
class FMockBlueprintService : public IBlueprintService
{
public:
    // Override interface methods with test implementations
    virtual UBlueprint* CreateBlueprint(const FBlueprintCreationParams& Params) override
    {
        CreateBlueprintCallCount++;
        LastCreationParams = Params;
        return MockBlueprint;
    }
    
    // Test helpers
    int32 CreateBlueprintCallCount = 0;
    FBlueprintCreationParams LastCreationParams;
    UBlueprint* MockBlueprint = nullptr;
};
```

## Common Patterns

### Validation Pattern

```cpp
bool ValidateParameters(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    if (!Params.IsValid())
    {
        OutError = TEXT("Parameters object is null");
        return false;
    }
    
    if (!Params->HasField(TEXT("name")))
    {
        OutError = TEXT("Missing required parameter: name");
        return false;
    }
    
    FString Name = Params->GetStringField(TEXT("name"));
    if (Name.IsEmpty())
    {
        OutError = TEXT("Parameter 'name' cannot be empty");
        return false;
    }
    
    return true;
}
```

### Factory Pattern

```cpp
template<typename T>
class TFactory
{
public:
    static TFactory& Get()
    {
        static TFactory Instance;
        return Instance;
    }
    
    void RegisterType(const FString& TypeName, TFunction<TSharedPtr<T>()> Creator)
    {
        FScopeLock Lock(&FactoryLock);
        Creators.Add(TypeName, Creator);
    }
    
    TSharedPtr<T> Create(const FString& TypeName)
    {
        FScopeLock Lock(&FactoryLock);
        if (TFunction<TSharedPtr<T>()>* Creator = Creators.Find(TypeName))
        {
            return (*Creator)();
        }
        return nullptr;
    }
    
private:
    TMap<FString, TFunction<TSharedPtr<T>()>> Creators;
    FCriticalSection FactoryLock;
};
```

## Code Review Checklist

### Before Submitting Code

- [ ] All public interfaces are documented
- [ ] Error handling is implemented and tested
- [ ] Memory management follows RAII principles
- [ ] Thread safety is considered and implemented where needed
- [ ] Performance implications are considered
- [ ] Unit tests are written and passing
- [ ] Code follows naming conventions
- [ ] No unused includes or variables
- [ ] Logging is appropriate and informative

### During Code Review

- [ ] Code is readable and self-documenting
- [ ] Design patterns are used appropriately
- [ ] Error conditions are handled properly
- [ ] Performance is acceptable
- [ ] Thread safety is correct
- [ ] Tests cover edge cases
- [ ] Documentation is accurate and complete

## Tools and Automation

### Static Analysis

Use Unreal Engine's built-in static analysis tools and consider additional tools like:
- PVS-Studio for C++ static analysis
- Clang Static Analyzer
- PC-Lint Plus

### Code Formatting

Follow Unreal Engine's coding standards and use:
- Unreal Engine's built-in code formatting
- ClangFormat with Unreal Engine configuration

### Documentation Generation

Use Doxygen-style comments for automatic documentation generation:

```cpp
/**
 * @file MyFile.h
 * @brief Brief description of the file
 * @author Author Name
 * @date Creation date
 */

/**
 * @class FMyClass
 * @brief Brief description of the class
 * @details Detailed description with usage examples
 */
```

This style guide ensures consistency and maintainability across the UnrealMCP plugin codebase.