// NiagaraParameterService.cpp - Parameters (Feature 3)
// AddParameter, SetParameter

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraTypes.h"

// ============================================================================
// Parameters (Feature 3)
// ============================================================================

bool FNiagaraService::AddParameter(const FNiagaraParameterAddParams& Params, FString& OutError)
{
    // Validate params
    if (!Params.IsValid(OutError))
    {
        return false;
    }

    // Find the system
    UNiagaraSystem* System = FindSystem(Params.SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("System not found: %s"), *Params.SystemPath);
        return false;
    }

    // Determine the type definition
    FNiagaraTypeDefinition TypeDef;
    FString TypeLower = Params.ParameterType.ToLower();

    if (TypeLower == TEXT("float"))
    {
        TypeDef = FNiagaraTypeDefinition::GetFloatDef();
    }
    else if (TypeLower == TEXT("int") || TypeLower == TEXT("int32"))
    {
        TypeDef = FNiagaraTypeDefinition::GetIntDef();
    }
    else if (TypeLower == TEXT("bool") || TypeLower == TEXT("boolean"))
    {
        TypeDef = FNiagaraTypeDefinition::GetBoolDef();
    }
    else if (TypeLower == TEXT("vector") || TypeLower == TEXT("vec3") || TypeLower == TEXT("vector3"))
    {
        TypeDef = FNiagaraTypeDefinition::GetVec3Def();
    }
    else if (TypeLower == TEXT("linearcolor") || TypeLower == TEXT("color"))
    {
        TypeDef = FNiagaraTypeDefinition::GetColorDef();
    }
    else
    {
        OutError = FString::Printf(TEXT("Unsupported parameter type '%s'. Supported: Float, Int, Bool, Vector, LinearColor"), *Params.ParameterType);
        return false;
    }

    // Build the full parameter name with scope prefix
    FString FullParameterName = Params.ParameterName;
    if (!FullParameterName.Contains(TEXT(".")))
    {
        // Add default scope prefix if not already present
        if (Params.Scope.Equals(TEXT("user"), ESearchCase::IgnoreCase))
        {
            FullParameterName = TEXT("User.") + FullParameterName;
        }
        else if (Params.Scope.Equals(TEXT("system"), ESearchCase::IgnoreCase))
        {
            FullParameterName = TEXT("System.") + FullParameterName;
        }
        else if (Params.Scope.Equals(TEXT("emitter"), ESearchCase::IgnoreCase))
        {
            FullParameterName = TEXT("Emitter.") + FullParameterName;
        }
    }

    // Create the parameter variable
    FNiagaraVariable NewParam(TypeDef, FName(*FullParameterName));

    // Set default value if provided
    if (Params.DefaultValue.IsValid() && Params.DefaultValue->Type == EJson::String)
    {
        FString ValueStr = Params.DefaultValue->AsString();
        NewParam.AllocateData();

        if (TypeDef == FNiagaraTypeDefinition::GetFloatDef())
        {
            NewParam.SetValue<float>(FCString::Atof(*ValueStr));
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetIntDef())
        {
            NewParam.SetValue<int32>(FCString::Atoi(*ValueStr));
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetBoolDef())
        {
            bool bValue = ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("1"));
            NewParam.SetValue<FNiagaraBool>(FNiagaraBool(bValue));
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetVec3Def())
        {
            TArray<FString> Components;
            ValueStr.ParseIntoArray(Components, TEXT(","), true);
            if (Components.Num() >= 3)
            {
                FVector3f Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]));
                NewParam.SetValue<FVector3f>(Vec);
            }
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetColorDef())
        {
            TArray<FString> Components;
            ValueStr.ParseIntoArray(Components, TEXT(","), true);
            if (Components.Num() >= 3)
            {
                float A = Components.Num() >= 4 ? FCString::Atof(*Components[3]) : 1.0f;
                FLinearColor Color(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]), A);
                NewParam.SetValue<FLinearColor>(Color);
            }
        }
    }
    else
    {
        // Initialize with default values
        NewParam.AllocateData();
        if (TypeDef == FNiagaraTypeDefinition::GetFloatDef())
        {
            NewParam.SetValue<float>(0.0f);
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetIntDef())
        {
            NewParam.SetValue<int32>(0);
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetBoolDef())
        {
            NewParam.SetValue<FNiagaraBool>(FNiagaraBool(false));
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetVec3Def())
        {
            NewParam.SetValue<FVector3f>(FVector3f::ZeroVector);
        }
        else if (TypeDef == FNiagaraTypeDefinition::GetColorDef())
        {
            NewParam.SetValue<FLinearColor>(FLinearColor::White);
        }
    }

    // Get the exposed parameter store and add the parameter
    System->Modify();
    FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();

    // Check if parameter already exists
    if (ExposedParams.FindParameterOffset(NewParam) != nullptr)
    {
        OutError = FString::Printf(TEXT("Parameter '%s' already exists in system"), *FullParameterName);
        return false;
    }

    // Add the parameter
    ExposedParams.AddParameter(NewParam, true, true);

    // Mark dirty and refresh
    MarkSystemDirty(System);
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Added parameter '%s' (%s) to system '%s'"),
        *FullParameterName, *Params.ParameterType, *Params.SystemPath);

    return true;
}

bool FNiagaraService::SetParameter(const FString& SystemPath, const FString& ParameterName, const TSharedPtr<FJsonValue>& Value, FString& OutError)
{
    // Find the system
    UNiagaraSystem* System = FindSystem(SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("System not found: %s"), *SystemPath);
        return false;
    }

    // Get the value - accept string, numeric, boolean, and array JSON types
    FString ValueStr;
    if (!Value.IsValid())
    {
        OutError = TEXT("Value is not valid");
        return false;
    }

    if (Value->Type == EJson::String)
    {
        ValueStr = Value->AsString();
    }
    else if (Value->Type == EJson::Number)
    {
        // Convert numeric value to string for unified processing
        ValueStr = FString::SanitizeFloat(Value->AsNumber());
    }
    else if (Value->Type == EJson::Boolean)
    {
        ValueStr = Value->AsBool() ? TEXT("true") : TEXT("false");
    }
    else if (Value->Type == EJson::Array)
    {
        // Handle arrays for vector/color values: [x, y, z] or [r, g, b, a]
        const TArray<TSharedPtr<FJsonValue>>& ArrayValues = Value->AsArray();
        TArray<FString> Components;
        for (const TSharedPtr<FJsonValue>& ArrayVal : ArrayValues)
        {
            if (ArrayVal->Type == EJson::Number)
            {
                Components.Add(FString::SanitizeFloat(ArrayVal->AsNumber()));
            }
        }
        ValueStr = FString::Join(Components, TEXT(","));
    }
    else
    {
        OutError = TEXT("Value must be a string, number, boolean, or array");
        return false;
    }

    // Get the exposed parameter store
    FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();

    // Get all parameters and find the one we want
    TArray<FNiagaraVariable> AllParams;
    ExposedParams.GetParameters(AllParams);

    FNiagaraVariable* FoundParam = nullptr;
    for (FNiagaraVariable& Param : AllParams)
    {
        if (Param.GetName().ToString().Equals(ParameterName, ESearchCase::IgnoreCase) ||
            Param.GetName().ToString().EndsWith(ParameterName, ESearchCase::IgnoreCase))
        {
            FoundParam = &Param;
            break;
        }
    }

    if (!FoundParam)
    {
        OutError = FString::Printf(TEXT("Parameter '%s' not found in system. Available: %s"),
            *ParameterName, *FString::JoinBy(AllParams, TEXT(", "), [](const FNiagaraVariable& V) { return V.GetName().ToString(); }));
        return false;
    }

    // Create a copy to set the value
    FNiagaraVariable UpdatedParam = *FoundParam;
    UpdatedParam.AllocateData();

    // Set value based on type
    FNiagaraTypeDefinition TypeDef = FoundParam->GetType();

    if (TypeDef == FNiagaraTypeDefinition::GetFloatDef())
    {
        UpdatedParam.SetValue<float>(FCString::Atof(*ValueStr));
    }
    else if (TypeDef == FNiagaraTypeDefinition::GetIntDef())
    {
        UpdatedParam.SetValue<int32>(FCString::Atoi(*ValueStr));
    }
    else if (TypeDef == FNiagaraTypeDefinition::GetBoolDef())
    {
        bool bValue = ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("1"));
        UpdatedParam.SetValue<FNiagaraBool>(FNiagaraBool(bValue));
    }
    else if (TypeDef == FNiagaraTypeDefinition::GetVec3Def())
    {
        TArray<FString> Components;
        ValueStr.ParseIntoArray(Components, TEXT(","), true);
        if (Components.Num() >= 3)
        {
            FVector3f Vec(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]));
            UpdatedParam.SetValue<FVector3f>(Vec);
        }
        else
        {
            OutError = TEXT("Vector value requires 3 comma-separated components (x,y,z)");
            return false;
        }
    }
    else if (TypeDef == FNiagaraTypeDefinition::GetColorDef())
    {
        TArray<FString> Components;
        ValueStr.ParseIntoArray(Components, TEXT(","), true);
        if (Components.Num() >= 3)
        {
            float A = Components.Num() >= 4 ? FCString::Atof(*Components[3]) : 1.0f;
            FLinearColor Color(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]), FCString::Atof(*Components[2]), A);
            UpdatedParam.SetValue<FLinearColor>(Color);
        }
        else
        {
            OutError = TEXT("Color value requires 3-4 comma-separated components (r,g,b[,a])");
            return false;
        }
    }
    else
    {
        OutError = FString::Printf(TEXT("Unsupported parameter type: %s"), *TypeDef.GetName());
        return false;
    }

    // Set the parameter value in the store
    System->Modify();
    ExposedParams.SetParameterData(UpdatedParam.GetData(), UpdatedParam, true);

    // Mark dirty and refresh
    MarkSystemDirty(System);
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set parameter '%s' to '%s' in system '%s'"),
        *ParameterName, *ValueStr, *SystemPath);

    return true;
}
