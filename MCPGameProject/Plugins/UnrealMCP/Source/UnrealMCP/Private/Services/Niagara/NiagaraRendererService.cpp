// NiagaraRendererService.cpp - Renderers (Feature 5)
// AddRenderer, SetRendererProperty

#include "Services/NiagaraService.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraCommon.h"
#include "NiagaraSpriteRendererProperties.h"

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
    else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        // Handle struct properties - specifically FNiagaraVariableAttributeBinding for bindings
        if (StructProp->Struct && StructProp->Struct->GetName() == TEXT("NiagaraVariableAttributeBinding"))
        {
            // Get the binding struct
            FNiagaraVariableAttributeBinding* Binding = StructProp->ContainerPtrToValuePtr<FNiagaraVariableAttributeBinding>(FoundRenderer);
            if (Binding)
            {
                // Get emitter for context
                UNiagaraEmitter* Emitter = EmitterHandle.GetInstance().Emitter;
                FVersionedNiagaraEmitterBase EmitterBase(Emitter, EmitterData->Version.VersionGuid);

                // Get source mode from renderer
                ENiagaraRendererSourceDataMode SourceMode = FoundRenderer->GetCurrentSourceMode();

                // Set the binding value (e.g., "Particles.Color" or just "Color")
                FName BindingName(*ValueStr);
                Binding->SetValue(BindingName, EmitterBase, SourceMode);

                // Cache values to ensure binding is properly resolved
                Binding->CacheValues(EmitterBase, SourceMode);

                UE_LOG(LogNiagaraService, Log, TEXT("Set attribute binding '%s' to '%s'"), *PropertyName, *ValueStr);
            }
            else
            {
                OutError = FString::Printf(TEXT("Failed to get binding struct for '%s'"), *PropertyName);
                return false;
            }
        }
        // Handle FVector2D (e.g., SubImageSize)
        else if (StructProp->Struct && StructProp->Struct->GetName() == TEXT("Vector2D"))
        {
            FVector2D* Vec = StructProp->ContainerPtrToValuePtr<FVector2D>(FoundRenderer);
            if (Vec)
            {
                // Parse "X,Y" format (e.g., "4,2" or "(X=4,Y=2)")
                TArray<FString> Components;
                FString CleanedStr = ValueStr;
                CleanedStr.ReplaceInline(TEXT("("), TEXT(""));
                CleanedStr.ReplaceInline(TEXT(")"), TEXT(""));
                CleanedStr.ReplaceInline(TEXT("X="), TEXT(""));
                CleanedStr.ReplaceInline(TEXT("Y="), TEXT(""));
                CleanedStr.ReplaceInline(TEXT(" "), TEXT(""));
                CleanedStr.ParseIntoArray(Components, TEXT(","), true);

                if (Components.Num() >= 2)
                {
                    Vec->X = FCString::Atof(*Components[0]);
                    Vec->Y = FCString::Atof(*Components[1]);
                    UE_LOG(LogNiagaraService, Log, TEXT("Set Vector2D property '%s' to (%f, %f)"), *PropertyName, Vec->X, Vec->Y);
                }
                else
                {
                    OutError = FString::Printf(TEXT("Invalid Vector2D format for '%s'. Expected 'X,Y' (e.g., '4,2')"), *PropertyName);
                    return false;
                }
            }
            else
            {
                OutError = FString::Printf(TEXT("Failed to get Vector2D struct for '%s'"), *PropertyName);
                return false;
            }
        }
        // Handle FVector
        else if (StructProp->Struct && StructProp->Struct->GetName() == TEXT("Vector"))
        {
            FVector* Vec = StructProp->ContainerPtrToValuePtr<FVector>(FoundRenderer);
            if (Vec)
            {
                // Parse "X,Y,Z" format
                TArray<FString> Components;
                FString CleanedStr = ValueStr;
                CleanedStr.ReplaceInline(TEXT("("), TEXT(""));
                CleanedStr.ReplaceInline(TEXT(")"), TEXT(""));
                CleanedStr.ReplaceInline(TEXT("X="), TEXT(""));
                CleanedStr.ReplaceInline(TEXT("Y="), TEXT(""));
                CleanedStr.ReplaceInline(TEXT("Z="), TEXT(""));
                CleanedStr.ReplaceInline(TEXT(" "), TEXT(""));
                CleanedStr.ParseIntoArray(Components, TEXT(","), true);

                if (Components.Num() >= 3)
                {
                    Vec->X = FCString::Atof(*Components[0]);
                    Vec->Y = FCString::Atof(*Components[1]);
                    Vec->Z = FCString::Atof(*Components[2]);
                    UE_LOG(LogNiagaraService, Log, TEXT("Set Vector property '%s' to (%f, %f, %f)"), *PropertyName, Vec->X, Vec->Y, Vec->Z);
                }
                else
                {
                    OutError = FString::Printf(TEXT("Invalid Vector format for '%s'. Expected 'X,Y,Z' (e.g., '1,2,3')"), *PropertyName);
                    return false;
                }
            }
            else
            {
                OutError = FString::Printf(TEXT("Failed to get Vector struct for '%s'"), *PropertyName);
                return false;
            }
        }
        // Handle FLinearColor
        else if (StructProp->Struct && StructProp->Struct->GetName() == TEXT("LinearColor"))
        {
            FLinearColor* Color = StructProp->ContainerPtrToValuePtr<FLinearColor>(FoundRenderer);
            if (Color)
            {
                // Parse "R,G,B,A" format
                TArray<FString> Components;
                FString CleanedStr = ValueStr;
                CleanedStr.ReplaceInline(TEXT("("), TEXT(""));
                CleanedStr.ReplaceInline(TEXT(")"), TEXT(""));
                CleanedStr.ReplaceInline(TEXT("R="), TEXT(""));
                CleanedStr.ReplaceInline(TEXT("G="), TEXT(""));
                CleanedStr.ReplaceInline(TEXT("B="), TEXT(""));
                CleanedStr.ReplaceInline(TEXT("A="), TEXT(""));
                CleanedStr.ReplaceInline(TEXT(" "), TEXT(""));
                CleanedStr.ParseIntoArray(Components, TEXT(","), true);

                if (Components.Num() >= 3)
                {
                    Color->R = FCString::Atof(*Components[0]);
                    Color->G = FCString::Atof(*Components[1]);
                    Color->B = FCString::Atof(*Components[2]);
                    Color->A = Components.Num() >= 4 ? FCString::Atof(*Components[3]) : 1.0f;
                    UE_LOG(LogNiagaraService, Log, TEXT("Set LinearColor property '%s' to (%f, %f, %f, %f)"), *PropertyName, Color->R, Color->G, Color->B, Color->A);
                }
                else
                {
                    OutError = FString::Printf(TEXT("Invalid LinearColor format for '%s'. Expected 'R,G,B' or 'R,G,B,A' (e.g., '1,0.5,0,1')"), *PropertyName);
                    return false;
                }
            }
            else
            {
                OutError = FString::Printf(TEXT("Failed to get LinearColor struct for '%s'"), *PropertyName);
                return false;
            }
        }
        else
        {
            OutError = FString::Printf(TEXT("Unsupported struct type '%s' for property '%s'"),
                StructProp->Struct ? *StructProp->Struct->GetName() : TEXT("null"), *PropertyName);
            return false;
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

bool FNiagaraService::GetRendererProperties(const FString& SystemPath, const FString& EmitterName, const FString& RendererName, TSharedPtr<FJsonObject>& OutProperties, FString& OutError)
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

    // Create output JSON object
    OutProperties = MakeShared<FJsonObject>();
    OutProperties->SetBoolField(TEXT("success"), true);
    OutProperties->SetStringField(TEXT("renderer_name"), FoundRenderer->GetName());
    OutProperties->SetStringField(TEXT("renderer_type"), FoundRenderer->GetClass()->GetName());

    // Create properties object
    TSharedPtr<FJsonObject> PropertiesObj = MakeShared<FJsonObject>();

    // Create bindings object
    TSharedPtr<FJsonObject> BindingsObj = MakeShared<FJsonObject>();

    // Iterate over all UPROPERTY fields using reflection
    UClass* RendererClass = FoundRenderer->GetClass();
    for (TFieldIterator<FProperty> PropIt(RendererClass); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (!Property)
        {
            continue;
        }

        // Skip properties that are not editable or not from this class hierarchy
        if (!Property->HasAnyPropertyFlags(CPF_Edit))
        {
            continue;
        }

        FString PropName = Property->GetName();

        // Handle different property types
        if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
        {
            UObject* Value = ObjectProp->GetObjectPropertyValue_InContainer(FoundRenderer);
            if (Value)
            {
                PropertiesObj->SetStringField(PropName, Value->GetPathName());
            }
            else
            {
                PropertiesObj->SetStringField(PropName, TEXT("None"));
            }
        }
        else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
        {
            bool Value = BoolProp->GetPropertyValue_InContainer(FoundRenderer);
            PropertiesObj->SetBoolField(PropName, Value);
        }
        else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
        {
            float Value = FloatProp->GetPropertyValue_InContainer(FoundRenderer);
            PropertiesObj->SetNumberField(PropName, Value);
        }
        else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
        {
            double Value = DoubleProp->GetPropertyValue_InContainer(FoundRenderer);
            PropertiesObj->SetNumberField(PropName, Value);
        }
        else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
        {
            int32 Value = IntProp->GetPropertyValue_InContainer(FoundRenderer);
            PropertiesObj->SetNumberField(PropName, Value);
        }
        else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
        {
            UEnum* EnumClass = EnumProp->GetEnum();
            if (EnumClass)
            {
                FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
                if (UnderlyingProp)
                {
                    const void* PropertyAddress = EnumProp->ContainerPtrToValuePtr<void>(FoundRenderer);
                    int64 EnumValue = UnderlyingProp->GetSignedIntPropertyValue(PropertyAddress);
                    FString EnumName = EnumClass->GetNameStringByValue(EnumValue);
                    // Remove enum prefix if present (e.g., "ENiagaraSpriteAlignment::" -> "")
                    EnumName.RemoveFromStart(EnumClass->GetName() + TEXT("::"));
                    PropertiesObj->SetStringField(PropName, EnumName);
                }
            }
        }
        else if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
        {
            UEnum* EnumClass = ByteProp->Enum;
            if (EnumClass)
            {
                uint8 Value = ByteProp->GetPropertyValue_InContainer(FoundRenderer);
                FString EnumName = EnumClass->GetNameStringByValue(Value);
                EnumName.RemoveFromStart(EnumClass->GetName() + TEXT("::"));
                PropertiesObj->SetStringField(PropName, EnumName);
            }
            else
            {
                uint8 Value = ByteProp->GetPropertyValue_InContainer(FoundRenderer);
                PropertiesObj->SetNumberField(PropName, Value);
            }
        }
        else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            // Handle FNiagaraVariableAttributeBinding (bindings)
            if (StructProp->Struct && StructProp->Struct->GetName() == TEXT("NiagaraVariableAttributeBinding"))
            {
                const FNiagaraVariableAttributeBinding* Binding = StructProp->ContainerPtrToValuePtr<FNiagaraVariableAttributeBinding>(FoundRenderer);
                if (Binding)
                {
                    // Get the bound variable name
                    FName BoundVar = Binding->GetDataSetBindableVariable().GetName();
                    FString BindingValue = BoundVar.IsNone() ? TEXT("None") : BoundVar.ToString();
                    BindingsObj->SetStringField(PropName, BindingValue);
                }
            }
            // Handle FVector2D
            else if (StructProp->Struct && StructProp->Struct->GetName() == TEXT("Vector2D"))
            {
                const FVector2D* Vec = StructProp->ContainerPtrToValuePtr<FVector2D>(FoundRenderer);
                if (Vec)
                {
                    TArray<TSharedPtr<FJsonValue>> VecArray;
                    VecArray.Add(MakeShared<FJsonValueNumber>(Vec->X));
                    VecArray.Add(MakeShared<FJsonValueNumber>(Vec->Y));
                    PropertiesObj->SetArrayField(PropName, VecArray);
                }
            }
            // Handle FVector
            else if (StructProp->Struct && StructProp->Struct->GetName() == TEXT("Vector"))
            {
                const FVector* Vec = StructProp->ContainerPtrToValuePtr<FVector>(FoundRenderer);
                if (Vec)
                {
                    TArray<TSharedPtr<FJsonValue>> VecArray;
                    VecArray.Add(MakeShared<FJsonValueNumber>(Vec->X));
                    VecArray.Add(MakeShared<FJsonValueNumber>(Vec->Y));
                    VecArray.Add(MakeShared<FJsonValueNumber>(Vec->Z));
                    PropertiesObj->SetArrayField(PropName, VecArray);
                }
            }
            // Handle FLinearColor
            else if (StructProp->Struct && StructProp->Struct->GetName() == TEXT("LinearColor"))
            {
                const FLinearColor* Color = StructProp->ContainerPtrToValuePtr<FLinearColor>(FoundRenderer);
                if (Color)
                {
                    TArray<TSharedPtr<FJsonValue>> ColorArray;
                    ColorArray.Add(MakeShared<FJsonValueNumber>(Color->R));
                    ColorArray.Add(MakeShared<FJsonValueNumber>(Color->G));
                    ColorArray.Add(MakeShared<FJsonValueNumber>(Color->B));
                    ColorArray.Add(MakeShared<FJsonValueNumber>(Color->A));
                    PropertiesObj->SetArrayField(PropName, ColorArray);
                }
            }
            // Skip other struct types for now
        }
        else if (FUInt32Property* UInt32Prop = CastField<FUInt32Property>(Property))
        {
            uint32 Value = UInt32Prop->GetPropertyValue_InContainer(FoundRenderer);
            PropertiesObj->SetNumberField(PropName, static_cast<double>(Value));
        }
    }

    OutProperties->SetObjectField(TEXT("properties"), PropertiesObj);
    OutProperties->SetObjectField(TEXT("bindings"), BindingsObj);

    UE_LOG(LogNiagaraService, Log, TEXT("Retrieved properties for renderer '%s' of type '%s'"),
        *FoundRenderer->GetName(), *FoundRenderer->GetClass()->GetName());

    return true;
}
