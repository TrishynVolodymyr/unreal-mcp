// NiagaraDataInterfaceService.cpp - Data Interfaces (Feature 4)
// AddDataInterface, SetDataInterfaceProperty

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraDataInterface.h"
#include "NiagaraTypes.h"

// ============================================================================
// Data Interfaces (Feature 4)
// ============================================================================

bool FNiagaraService::AddDataInterface(const FNiagaraDataInterfaceParams& Params, FString& OutInterfaceId, FString& OutError)
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

    // Find the emitter handle by name
    int32 EmitterIndex = FindEmitterHandleIndex(System, Params.EmitterName);
    if (EmitterIndex == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("Emitter '%s' not found in system '%s'"), *Params.EmitterName, *Params.SystemPath);
        return false;
    }

    const FNiagaraEmitterHandle& EmitterHandle = System->GetEmitterHandle(EmitterIndex);
    FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
    if (!EmitterData)
    {
        OutError = FString::Printf(TEXT("Could not get emitter data for '%s'"), *Params.EmitterName);
        return false;
    }

    // Create the data interface
    UNiagaraDataInterface* NewDI = CreateDataInterfaceByType(Params.InterfaceType, EmitterHandle.GetInstance().Emitter);
    if (!NewDI)
    {
        OutError = FString::Printf(TEXT("Failed to create data interface of type '%s'. Supported types: StaticMesh, SkeletalMesh, Spline, Audio, Curve, Texture, Grid2D, Grid3D"), *Params.InterfaceType);
        return false;
    }

    // Generate interface name
    FString DIName = Params.InterfaceName.IsEmpty() ?
        FString::Printf(TEXT("%s_DI_%d"), *Params.InterfaceType, FMath::Rand() % 1000) :
        Params.InterfaceName;

    // Mark system modified
    System->Modify();

    // Create a parameter for the data interface and add to exposed parameters
    FNiagaraTypeDefinition DITypeDef(NewDI->GetClass());
    FNiagaraVariable DIVar(DITypeDef, FName(*DIName));

    // Add to system's exposed parameters so it can be found and modified later
    FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();
    ExposedParams.AddParameter(DIVar, true, true);

    // Set the data interface value using the parameter store's SetDataInterface
    const int32* DIOffset = ExposedParams.FindParameterOffset(DIVar);
    if (DIOffset != nullptr)
    {
        ExposedParams.SetDataInterface(NewDI, *DIOffset);
    }

    OutInterfaceId = DIName;

    UE_LOG(LogNiagaraService, Log, TEXT("Added data interface '%s' of type '%s' to emitter '%s'"),
        *DIName, *Params.InterfaceType, *Params.EmitterName);

    // Mark dirty and refresh
    MarkSystemDirty(System);
    RefreshEditors(System);

    return true;
}

bool FNiagaraService::SetDataInterfaceProperty(const FString& SystemPath, const FString& EmitterName, const FString& InterfaceName, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError)
{
    // Find the system
    UNiagaraSystem* System = FindSystem(SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("System not found: %s"), *SystemPath);
        return false;
    }

    // Get the exposed parameters and find the data interface
    FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();
    const TArray<UNiagaraDataInterface*>& DataInterfaces = ExposedParams.GetDataInterfaces();

    // Find the DI by name
    UNiagaraDataInterface* FoundDI = nullptr;
    for (UNiagaraDataInterface* DI : DataInterfaces)
    {
        if (DI && DI->GetName().Contains(InterfaceName, ESearchCase::IgnoreCase))
        {
            FoundDI = DI;
            break;
        }
    }

    if (!FoundDI)
    {
        // List available data interfaces
        TArray<FString> DINames;
        for (UNiagaraDataInterface* DI : DataInterfaces)
        {
            if (DI)
            {
                DINames.Add(DI->GetName());
            }
        }
        OutError = FString::Printf(TEXT("Data interface '%s' not found. Available: %s"),
            *InterfaceName, DINames.Num() > 0 ? *FString::Join(DINames, TEXT(", ")) : TEXT("none"));
        return false;
    }

    // Get value as string
    FString ValueStr;
    if (PropertyValue.IsValid() && PropertyValue->Type == EJson::String)
    {
        ValueStr = PropertyValue->AsString();
    }
    else
    {
        OutError = TEXT("Property value must be provided as a string");
        return false;
    }

    // Use reflection to set the property
    System->Modify();
    FoundDI->Modify();

    FProperty* Property = FoundDI->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property '%s' not found on data interface '%s'"), *PropertyName, *InterfaceName);
        return false;
    }

    // Handle different property types
    if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
    {
        // For object properties, load the asset
        UObject* LoadedAsset = LoadObject<UObject>(nullptr, *ValueStr);
        if (LoadedAsset)
        {
            ObjectProp->SetObjectPropertyValue_InContainer(FoundDI, LoadedAsset);
        }
        else
        {
            OutError = FString::Printf(TEXT("Failed to load asset: %s"), *ValueStr);
            return false;
        }
    }
    else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        bool BoolValue = ValueStr.ToBool() || ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("1"));
        BoolProp->SetPropertyValue_InContainer(FoundDI, BoolValue);
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        float FloatValue = FCString::Atof(*ValueStr);
        FloatProp->SetPropertyValue_InContainer(FoundDI, FloatValue);
    }
    else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
        double DoubleValue = FCString::Atod(*ValueStr);
        DoubleProp->SetPropertyValue_InContainer(FoundDI, DoubleValue);
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        int32 IntValue = FCString::Atoi(*ValueStr);
        IntProp->SetPropertyValue_InContainer(FoundDI, IntValue);
    }
    else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        StrProp->SetPropertyValue_InContainer(FoundDI, ValueStr);
    }
    else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        NameProp->SetPropertyValue_InContainer(FoundDI, FName(*ValueStr));
    }
    else
    {
        OutError = FString::Printf(TEXT("Unsupported property type for '%s'"), *PropertyName);
        return false;
    }

    UE_LOG(LogNiagaraService, Log, TEXT("Set data interface property '%s' to '%s' on '%s'"),
        *PropertyName, *ValueStr, *InterfaceName);

    // Mark dirty and refresh
    MarkSystemDirty(System);
    RefreshEditors(System);

    return true;
}
