#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "MCPResponse.h"
#include "MCPParameterValidator.h"

/**
 * Poolable JSON object wrapper for efficient reuse
 * Wraps TSharedPtr<FJsonObject> with reset functionality
 */
class UNREALMCP_API FPoolableJsonObject
{
public:
    /** Default constructor */
    FPoolableJsonObject()
    {
        JsonObject = MakeShared<FJsonObject>();
    }
    
    /** Destructor */
    ~FPoolableJsonObject() = default;
    
    /**
     * Reset the object to clean state for reuse
     */
    void Reset()
    {
        if (JsonObject.IsValid())
        {
            JsonObject->Values.Empty();
        }
        else
        {
            JsonObject = MakeShared<FJsonObject>();
        }
    }
    
    /**
     * Get the underlying JSON object
     * @return Shared pointer to JSON object
     */
    TSharedPtr<FJsonObject> GetJsonObject() const
    {
        return JsonObject;
    }
    
    /**
     * Set a string value
     * @param Key - Field name
     * @param Value - String value
     */
    void SetStringField(const FString& Key, const FString& Value)
    {
        if (JsonObject.IsValid())
        {
            JsonObject->SetStringField(Key, Value);
        }
    }
    
    /**
     * Set a number value
     * @param Key - Field name
     * @param Value - Number value
     */
    void SetNumberField(const FString& Key, double Value)
    {
        if (JsonObject.IsValid())
        {
            JsonObject->SetNumberField(Key, Value);
        }
    }
    
    /**
     * Set a boolean value
     * @param Key - Field name
     * @param Value - Boolean value
     */
    void SetBoolField(const FString& Key, bool Value)
    {
        if (JsonObject.IsValid())
        {
            JsonObject->SetBoolField(Key, Value);
        }
    }
    
    /**
     * Set an array value
     * @param Key - Field name
     * @param Value - Array value
     */
    void SetArrayField(const FString& Key, const TArray<TSharedPtr<FJsonValue>>& Value)
    {
        if (JsonObject.IsValid())
        {
            JsonObject->SetArrayField(Key, Value);
        }
    }
    
    /**
     * Set an object value
     * @param Key - Field name
     * @param Value - Object value
     */
    void SetObjectField(const FString& Key, const TSharedPtr<FJsonObject>& Value)
    {
        if (JsonObject.IsValid())
        {
            JsonObject->SetObjectField(Key, Value);
        }
    }

private:
    /** The underlying JSON object */
    TSharedPtr<FJsonObject> JsonObject;
};

/**
 * Poolable MCP response wrapper for efficient reuse
 * Wraps FMCPResponse with reset functionality
 */
class UNREALMCP_API FPoolableMCPResponse
{
public:
    /** Default constructor */
    FPoolableMCPResponse() = default;
    
    /** Destructor */
    ~FPoolableMCPResponse() = default;
    
    /**
     * Reset the response to clean state for reuse
     */
    void Reset()
    {
        Response.bSuccess = false;
        Response.Data.Empty();
        Response.Error = FMCPError();
        Response.Metadata.Empty();
    }
    
    /**
     * Get the underlying MCP response
     * @return Reference to MCP response
     */
    FMCPResponse& GetResponse()
    {
        return Response;
    }
    
    /**
     * Get the underlying MCP response (const)
     * @return Const reference to MCP response
     */
    const FMCPResponse& GetResponse() const
    {
        return Response;
    }
    
    /**
     * Set as successful response
     * @param ResponseData - Response data
     * @param ResponseMetadata - Optional metadata
     */
    void SetSuccess(const FString& ResponseData, const FString& ResponseMetadata = TEXT(""))
    {
        Response.bSuccess = true;
        Response.Data = ResponseData;
        Response.Metadata = ResponseMetadata;
        Response.Error = FMCPError(); // Clear error
    }
    
    /**
     * Set as failure response
     * @param ResponseError - Error information
     * @param ResponseMetadata - Optional metadata
     */
    void SetFailure(const FMCPError& ResponseError, const FString& ResponseMetadata = TEXT(""))
    {
        Response.bSuccess = false;
        Response.Error = ResponseError;
        Response.Metadata = ResponseMetadata;
        Response.Data.Empty(); // Clear data
    }
    
    /**
     * Convert response to JSON string
     * @return JSON string representation
     */
    FString ToJsonString() const
    {
        return Response.ToJsonString();
    }

private:
    /** The underlying MCP response */
    FMCPResponse Response;
};

/**
 * Poolable parameter validator wrapper for efficient reuse
 * Wraps FParameterValidator with reset functionality
 */
class UNREALMCP_API FPoolableParameterValidator
{
public:
    /** Default constructor */
    FPoolableParameterValidator() = default;
    
    /** Destructor */
    ~FPoolableParameterValidator() = default;
    
    /**
     * Reset the validator to clean state for reuse
     */
    void Reset()
    {
        Validator.ClearRules();
    }
    
    /**
     * Get the underlying parameter validator
     * @return Reference to parameter validator
     */
    FParameterValidator& GetValidator()
    {
        return Validator;
    }
    
    /**
     * Get the underlying parameter validator (const)
     * @return Const reference to parameter validator
     */
    const FParameterValidator& GetValidator() const
    {
        return Validator;
    }
    
    /**
     * Add a validation rule
     * @param Rule - Validation rule to add
     */
    void AddRule(const FValidationRule& Rule)
    {
        Validator.AddRule(Rule);
    }
    
    /**
     * Validate parameters
     * @param Params - JSON parameters to validate
     * @param OutError - Error message if validation fails
     * @return true if validation passes
     */
    bool ValidateParams(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
    {
        return Validator.ValidateParams(Params, OutError);
    }
    
    /**
     * Get number of validation rules
     * @return Number of rules
     */
    int32 GetRuleCount() const
    {
        return Validator.GetRuleCount();
    }

private:
    /** The underlying parameter validator */
    FParameterValidator Validator;
};

/**
 * Poolable JSON value wrapper for efficient reuse
 * Wraps TSharedPtr<FJsonValue> with reset functionality
 */
class UNREALMCP_API FPoolableJsonValue
{
public:
    /** Default constructor */
    FPoolableJsonValue()
    {
        JsonValue = MakeShared<FJsonValueNull>();
    }
    
    /** Destructor */
    ~FPoolableJsonValue() = default;
    
    /**
     * Reset the value to clean state for reuse
     */
    void Reset()
    {
        JsonValue = MakeShared<FJsonValueNull>();
    }
    
    /**
     * Get the underlying JSON value
     * @return Shared pointer to JSON value
     */
    TSharedPtr<FJsonValue> GetJsonValue() const
    {
        return JsonValue;
    }
    
    /**
     * Set as string value
     * @param Value - String value
     */
    void SetString(const FString& Value)
    {
        JsonValue = MakeShared<FJsonValueString>(Value);
    }
    
    /**
     * Set as number value
     * @param Value - Number value
     */
    void SetNumber(double Value)
    {
        JsonValue = MakeShared<FJsonValueNumber>(Value);
    }
    
    /**
     * Set as boolean value
     * @param Value - Boolean value
     */
    void SetBoolean(bool Value)
    {
        JsonValue = MakeShared<FJsonValueBoolean>(Value);
    }
    
    /**
     * Set as array value
     * @param Value - Array value
     */
    void SetArray(const TArray<TSharedPtr<FJsonValue>>& Value)
    {
        JsonValue = MakeShared<FJsonValueArray>(Value);
    }
    
    /**
     * Set as object value
     * @param Value - Object value
     */
    void SetObject(const TSharedPtr<FJsonObject>& Value)
    {
        JsonValue = MakeShared<FJsonValueObject>(Value);
    }
    
    /**
     * Set as null value
     */
    void SetNull()
    {
        JsonValue = MakeShared<FJsonValueNull>();
    }

private:
    /** The underlying JSON value */
    TSharedPtr<FJsonValue> JsonValue;
};
