#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

/**
 * Structure representing a single validation rule for parameter validation
 */
struct UNREALMCP_API FValidationRule
{
    /** Name of the field to validate */
    FString FieldName;
    
    /** Whether this field is required */
    bool bRequired = false;
    
    /** Custom validation function that returns true if valid */
    TFunction<bool(const TSharedPtr<FJsonValue>&)> ValidationFunc;
    
    /** Error message to display if validation fails */
    FString ErrorMessage;
    
    /** Default constructor */
    FValidationRule() = default;
    
    /** Constructor with all parameters */
    FValidationRule(const FString& InFieldName, bool bInRequired, 
                   TFunction<bool(const TSharedPtr<FJsonValue>&)> InValidationFunc,
                   const FString& InErrorMessage)
        : FieldName(InFieldName)
        , bRequired(bInRequired)
        , ValidationFunc(InValidationFunc)
        , ErrorMessage(InErrorMessage)
    {
    }
};

/**
 * Parameter validation framework for MCP commands
 * Provides flexible validation rules system with common validation functions
 */
class UNREALMCP_API FParameterValidator
{
public:
    /** Default constructor */
    FParameterValidator() = default;
    
    /** Destructor */
    ~FParameterValidator() = default;
    
    /**
     * Add a validation rule to this validator
     * @param Rule The validation rule to add
     */
    void AddRule(const FValidationRule& Rule);
    
    /**
     * Validate parameters against all registered rules
     * @param Params The JSON parameters to validate
     * @param OutError Error message if validation fails
     * @return true if all validations pass, false otherwise
     */
    bool ValidateParams(const TSharedPtr<FJsonObject>& Params, FString& OutError) const;
    
    /**
     * Clear all validation rules
     */
    void ClearRules();
    
    /**
     * Get the number of validation rules
     * @return Number of rules registered
     */
    int32 GetRuleCount() const { return ValidationRules.Num(); }
    
    // Static validation functions for common types
    
    /**
     * Check if a JSON value is a valid non-empty string
     * @param Value The JSON value to check
     * @return true if value is a valid string
     */
    static bool IsValidString(const TSharedPtr<FJsonValue>& Value);
    
    /**
     * Check if a JSON value is a valid number (integer or float)
     * @param Value The JSON value to check
     * @return true if value is a valid number
     */
    static bool IsValidNumber(const TSharedPtr<FJsonValue>& Value);
    
    /**
     * Check if a JSON value is a valid array
     * @param Value The JSON value to check
     * @return true if value is a valid array
     */
    static bool IsValidArray(const TSharedPtr<FJsonValue>& Value);
    
    /**
     * Check if a JSON value is a valid boolean
     * @param Value The JSON value to check
     * @return true if value is a valid boolean
     */
    static bool IsValidBoolean(const TSharedPtr<FJsonValue>& Value);
    
    /**
     * Check if a JSON value is a valid object
     * @param Value The JSON value to check
     * @return true if value is a valid object
     */
    static bool IsValidObject(const TSharedPtr<FJsonValue>& Value);
    
    /**
     * Check if a string value matches a specific pattern (basic wildcard support)
     * @param Value The JSON value to check (must be string)
     * @param Pattern Pattern to match (* for wildcard)
     * @return true if string matches pattern
     */
    static bool MatchesPattern(const TSharedPtr<FJsonValue>& Value, const FString& Pattern);
    
    /**
     * Check if a number value is within a specified range
     * @param Value The JSON value to check (must be number)
     * @param MinValue Minimum allowed value
     * @param MaxValue Maximum allowed value
     * @return true if number is within range
     */
    static bool IsInRange(const TSharedPtr<FJsonValue>& Value, double MinValue, double MaxValue);
    
    /**
     * Check if an array has a minimum number of elements
     * @param Value The JSON value to check (must be array)
     * @param MinElements Minimum number of elements required
     * @return true if array has at least MinElements
     */
    static bool HasMinElements(const TSharedPtr<FJsonValue>& Value, int32 MinElements);
    
    /**
     * Check if an array has a maximum number of elements
     * @param Value The JSON value to check (must be array)
     * @param MaxElements Maximum number of elements allowed
     * @return true if array has at most MaxElements
     */
    static bool HasMaxElements(const TSharedPtr<FJsonValue>& Value, int32 MaxElements);

private:
    /** Array of validation rules */
    TArray<FValidationRule> ValidationRules;
};

/**
 * Helper class for building common validation rules
 */
class UNREALMCP_API FValidationRuleBuilder
{
public:
    /**
     * Create a required string validation rule
     * @param FieldName Name of the field
     * @param ErrorMessage Custom error message (optional)
     * @return Validation rule for required string
     */
    static FValidationRule RequiredString(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    
    /**
     * Create an optional string validation rule
     * @param FieldName Name of the field
     * @param ErrorMessage Custom error message (optional)
     * @return Validation rule for optional string
     */
    static FValidationRule OptionalString(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    
    /**
     * Create a required number validation rule
     * @param FieldName Name of the field
     * @param ErrorMessage Custom error message (optional)
     * @return Validation rule for required number
     */
    static FValidationRule RequiredNumber(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    
    /**
     * Create an optional number validation rule
     * @param FieldName Name of the field
     * @param ErrorMessage Custom error message (optional)
     * @return Validation rule for optional number
     */
    static FValidationRule OptionalNumber(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    
    /**
     * Create a required array validation rule
     * @param FieldName Name of the field
     * @param ErrorMessage Custom error message (optional)
     * @return Validation rule for required array
     */
    static FValidationRule RequiredArray(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    
    /**
     * Create an optional array validation rule
     * @param FieldName Name of the field
     * @param ErrorMessage Custom error message (optional)
     * @return Validation rule for optional array
     */
    static FValidationRule OptionalArray(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    
    /**
     * Create a required boolean validation rule
     * @param FieldName Name of the field
     * @param ErrorMessage Custom error message (optional)
     * @return Validation rule for required boolean
     */
    static FValidationRule RequiredBoolean(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
    
    /**
     * Create an optional boolean validation rule
     * @param FieldName Name of the field
     * @param ErrorMessage Custom error message (optional)
     * @return Validation rule for optional boolean
     */
    static FValidationRule OptionalBoolean(const FString& FieldName, const FString& ErrorMessage = TEXT(""));
};
