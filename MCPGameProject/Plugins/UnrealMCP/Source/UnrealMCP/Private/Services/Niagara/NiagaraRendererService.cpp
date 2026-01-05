// NiagaraRendererService.cpp - Renderers (Feature 5)
// AddRenderer, SetRendererProperty

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraRendererProperties.h"

// ============================================================================
// Renderers (Feature 5)
// ============================================================================

bool FNiagaraService::AddRenderer(const FNiagaraRendererParams& Params, FString& OutRendererId, FString& OutError)
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

    // Get the emitter for modification
    UNiagaraEmitter* Emitter = EmitterHandle.GetInstance().Emitter;
    if (!Emitter)
    {
        OutError = TEXT("Could not get emitter instance");
        return false;
    }

    // Create the renderer
    UNiagaraRendererProperties* NewRenderer = CreateRendererByType(Params.RendererType, Emitter);
    if (!NewRenderer)
    {
        OutError = FString::Printf(TEXT("Failed to create renderer of type '%s'. Supported types: Sprite, Mesh, Ribbon, Light, Component"), *Params.RendererType);
        return false;
    }

    // Set custom name if provided
    if (!Params.RendererName.IsEmpty())
    {
        NewRenderer->Rename(*Params.RendererName);
    }

    // Mark for modification
    System->Modify();
    Emitter->Modify();

    // Add the renderer to the emitter
    Emitter->AddRenderer(NewRenderer, EmitterData->Version.VersionGuid);

    // Get renderer ID (use name or generate one)
    OutRendererId = NewRenderer->GetName();

    // Mark dirty
    MarkSystemDirty(System);

    // Broadcast post-edit change to trigger parameter map rebuilding
    System->OnSystemPostEditChange().Broadcast(System);

    // Request synchronous compilation and wait for it to complete
    System->RequestCompile(false);
    System->WaitForCompilationComplete();

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Added renderer '%s' of type '%s' to emitter '%s'"),
        *OutRendererId, *Params.RendererType, *Params.EmitterName);

    return true;
}

bool FNiagaraService::SetRendererProperty(const FString& SystemPath, const FString& EmitterName, const FString& RendererName, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError)
{
    // Find the system
    UNiagaraSystem* System = FindSystem(SystemPath);
    if (!System)
    {
        OutError = FString::Printf(TEXT("System not found: %s"), *SystemPath);
        return false;
    }

    // Find the emitter handle by name
    int32 EmitterIndex = FindEmitterHandleIndex(System, EmitterName);
    if (EmitterIndex == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("Emitter '%s' not found in system '%s'"), *EmitterName, *SystemPath);
        return false;
    }

    const FNiagaraEmitterHandle& EmitterHandle = System->GetEmitterHandle(EmitterIndex);
    FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
    if (!EmitterData)
    {
        OutError = FString::Printf(TEXT("Could not get emitter data for '%s'"), *EmitterName);
        return false;
    }

    // Find the renderer by name
    UNiagaraRendererProperties* FoundRenderer = nullptr;
    for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
    {
        if (Renderer && (Renderer->GetName().Contains(RendererName, ESearchCase::IgnoreCase)))
        {
            FoundRenderer = Renderer;
            break;
        }
    }

    if (!FoundRenderer)
    {
        // List available renderers
        TArray<FString> RendererNames;
        for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
        {
            if (Renderer)
            {
                RendererNames.Add(Renderer->GetName());
            }
        }
        OutError = FString::Printf(TEXT("Renderer '%s' not found. Available: %s"),
            *RendererName, *FString::Join(RendererNames, TEXT(", ")));
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
    FoundRenderer->Modify();

    FProperty* Property = FoundRenderer->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        // Try common property name variations
        FString AltPropertyName = PropertyName;
        if (!PropertyName.StartsWith(TEXT("b")))
        {
            AltPropertyName = TEXT("b") + PropertyName;
        }
        Property = FoundRenderer->GetClass()->FindPropertyByName(FName(*AltPropertyName));
    }

    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property '%s' not found on renderer '%s'"), *PropertyName, *RendererName);
        return false;
    }

    // Handle different property types
    if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
    {
        // For object properties like Material, load the asset
        UObject* LoadedAsset = LoadObject<UObject>(nullptr, *ValueStr);
        if (LoadedAsset)
        {
            ObjectProp->SetObjectPropertyValue_InContainer(FoundRenderer, LoadedAsset);
        }
        else
        {
            OutError = FString::Printf(TEXT("Failed to load asset: %s"), *ValueStr);
            return false;
        }
    }
    else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        bool bValue = ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("1"));
        BoolProp->SetPropertyValue_InContainer(FoundRenderer, bValue);
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        FloatProp->SetPropertyValue_InContainer(FoundRenderer, FCString::Atof(*ValueStr));
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        IntProp->SetPropertyValue_InContainer(FoundRenderer, FCString::Atoi(*ValueStr));
    }
    else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        // Handle C++11 enum class properties (e.g., ENiagaraSpriteAlignment)
        UEnum* EnumClass = EnumProp->GetEnum();
        if (EnumClass)
        {
            // Try to find the enum value by name
            int64 EnumValue = EnumClass->GetValueByNameString(ValueStr);
            if (EnumValue == INDEX_NONE)
            {
                // Try with enum prefix (e.g., "VelocityAligned" -> "ENiagaraSpriteAlignment::VelocityAligned")
                FString QualifiedName = FString::Printf(TEXT("%s::%s"), *EnumClass->GetName(), *ValueStr);
                EnumValue = EnumClass->GetValueByNameString(QualifiedName);
            }

            if (EnumValue != INDEX_NONE)
            {
                FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
                if (UnderlyingProp)
                {
                    void* PropertyAddress = EnumProp->ContainerPtrToValuePtr<void>(FoundRenderer);
                    UnderlyingProp->SetIntPropertyValue(PropertyAddress, EnumValue);
                }
            }
            else
            {
                // List valid enum values for the error message
                TArray<FString> ValidValues;
                for (int32 i = 0; i < EnumClass->NumEnums() - 1; ++i) // -1 to skip _MAX
                {
                    ValidValues.Add(EnumClass->GetNameStringByIndex(i));
                }
                OutError = FString::Printf(TEXT("Invalid enum value '%s' for property '%s'. Valid values: %s"),
                    *ValueStr, *PropertyName, *FString::Join(ValidValues, TEXT(", ")));
                return false;
            }
        }
    }
    else if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
    {
        // Handle TEnumAsByte<T> style enums
        UEnum* EnumClass = ByteProp->Enum;
        if (EnumClass)
        {
            int64 EnumValue = EnumClass->GetValueByNameString(ValueStr);
            if (EnumValue == INDEX_NONE)
            {
                FString QualifiedName = FString::Printf(TEXT("%s::%s"), *EnumClass->GetName(), *ValueStr);
                EnumValue = EnumClass->GetValueByNameString(QualifiedName);
            }

            if (EnumValue != INDEX_NONE)
            {
                ByteProp->SetPropertyValue_InContainer(FoundRenderer, static_cast<uint8>(EnumValue));
            }
            else
            {
                TArray<FString> ValidValues;
                for (int32 i = 0; i < EnumClass->NumEnums() - 1; ++i)
                {
                    ValidValues.Add(EnumClass->GetNameStringByIndex(i));
                }
                OutError = FString::Printf(TEXT("Invalid enum value '%s' for property '%s'. Valid values: %s"),
                    *ValueStr, *PropertyName, *FString::Join(ValidValues, TEXT(", ")));
                return false;
            }
        }
        else
        {
            // Plain uint8, treat as integer
            ByteProp->SetPropertyValue_InContainer(FoundRenderer, static_cast<uint8>(FCString::Atoi(*ValueStr)));
        }
    }
    else
    {
        OutError = FString::Printf(TEXT("Unsupported property type for '%s'"), *PropertyName);
        return false;
    }

    // Mark dirty
    MarkSystemDirty(System);

    // Broadcast post-edit change to trigger parameter map rebuilding
    System->OnSystemPostEditChange().Broadcast(System);

    // Request synchronous compilation and wait for it to complete
    System->RequestCompile(false);
    System->WaitForCompilationComplete();

    // Refresh editors
    RefreshEditors(System);

    UE_LOG(LogNiagaraService, Log, TEXT("Set renderer property '%s' to '%s' on renderer '%s'"),
        *PropertyName, *ValueStr, *RendererName);

    return true;
}
