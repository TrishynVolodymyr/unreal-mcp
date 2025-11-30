#include "Commands/Blueprint/SetComponentPropertyCommand.h"
#include "Services/PropertyService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "SubobjectDataSubsystem.h"
#include "SubobjectData.h"
#include "Engine/Engine.h"
#include "MCPLogging.h"

FSetComponentPropertyCommand::FSetComponentPropertyCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FSetComponentPropertyCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    FString ComponentName;
    TSharedPtr<FJsonObject> Properties;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, ComponentName, Properties, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the blueprint
    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }
    
    // Set properties using the service layer
    TArray<FString> SuccessProperties;
    TMap<FString, FString> FailedProperties;
    TArray<FString> AvailableProperties;
    
    SetComponentProperties(Blueprint, ComponentName, Properties, SuccessProperties, FailedProperties, AvailableProperties);
    
    // Always return success response with details about what succeeded and what failed
    return CreateSuccessResponse(SuccessProperties, FailedProperties, AvailableProperties);
}

FString FSetComponentPropertyCommand::GetCommandName() const
{
    return TEXT("modify_blueprint_component_properties");
}

bool FSetComponentPropertyCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName;
    FString ComponentName;
    TSharedPtr<FJsonObject> Properties;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, ComponentName, Properties, ParseError);
}

bool FSetComponentPropertyCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, 
                                                  FString& OutComponentName, TSharedPtr<FJsonObject>& OutProperties, 
                                                  FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required blueprint_name parameter
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }
    
    // Parse required component_name parameter
    if (!JsonObject->TryGetStringField(TEXT("component_name"), OutComponentName))
    {
        OutError = TEXT("Missing required 'component_name' parameter");
        return false;
    }
    
    // Parse kwargs parameter (can be object or string)
    const TSharedPtr<FJsonObject>* KwargsObjectPtr = nullptr;
    if (JsonObject->TryGetObjectField(TEXT("kwargs"), KwargsObjectPtr) && KwargsObjectPtr && KwargsObjectPtr->IsValid())
    {
        OutProperties = *KwargsObjectPtr;
        UE_LOG(LogUnrealMCP, Warning, TEXT("ParseParameters: Got kwargs as object with %d fields"), OutProperties->Values.Num());
    }
    else
    {
        // Try to get as string and parse
        FString KwargsString;
        if (JsonObject->TryGetStringField(TEXT("kwargs"), KwargsString))
        {
            UE_LOG(LogUnrealMCP, Warning, TEXT("ParseParameters: Got kwargs as string: %s"), *KwargsString);
            TSharedRef<TJsonReader<>> KwargsReader = TJsonReaderFactory<>::Create(KwargsString);
            TSharedPtr<FJsonObject> ParsedObject;
            if (FJsonSerializer::Deserialize(KwargsReader, ParsedObject) && ParsedObject.IsValid())
            {
                // Check if the parsed object has a "kwargs" field (double-wrapped)
                const TSharedPtr<FJsonObject>* InnerKwargsPtr = nullptr;
                if (ParsedObject->TryGetObjectField(TEXT("kwargs"), InnerKwargsPtr) && InnerKwargsPtr && InnerKwargsPtr->IsValid())
                {
                    // Use the inner kwargs object
                    OutProperties = *InnerKwargsPtr;
                    UE_LOG(LogUnrealMCP, Warning, TEXT("ParseParameters: Unwrapped double-nested kwargs with %d fields"), OutProperties->Values.Num());
                }
                else
                {
                    // Use the parsed object directly
                    OutProperties = ParsedObject;
                    UE_LOG(LogUnrealMCP, Warning, TEXT("ParseParameters: Parsed kwargs string into object with %d fields"), OutProperties->Values.Num());
                }
            }
            else
            {
                UE_LOG(LogUnrealMCP, Error, TEXT("ParseParameters: Failed to parse kwargs string as JSON"));
            }
        }
        else
        {
            UE_LOG(LogUnrealMCP, Error, TEXT("ParseParameters: kwargs is neither object nor string"));
        }
    }
    
    if (!OutProperties.IsValid())
    {
        OutError = TEXT("Missing or invalid 'kwargs' parameter (must be a dictionary of properties or a JSON string)");
        return false;
    }
    
    return true;
}

bool FSetComponentPropertyCommand::SetComponentProperties(UBlueprint* Blueprint, const FString& ComponentName,
                                                         const TSharedPtr<FJsonObject>& Properties,
                                                         TArray<FString>& OutSuccessProperties,
                                                         TMap<FString, FString>& OutFailedProperties,
                                                         TArray<FString>& OutAvailableProperties) const
{
    // Use USubobjectDataSubsystem to find component (UE 5.7+ API)
    USubobjectDataSubsystem* SubobjectSubsystem = GEngine->GetEngineSubsystem<USubobjectDataSubsystem>();
    if (!SubobjectSubsystem)
    {
        OutFailedProperties.Add(TEXT("subsystem"), TEXT("Failed to get SubobjectDataSubsystem"));
        UE_LOG(LogUnrealMCP, Error, TEXT("Failed to get SubobjectDataSubsystem"));
        return false;
    }

    // Gather all subobjects for this Blueprint
    TArray<FSubobjectDataHandle> SubobjectHandles;
    SubobjectSubsystem->K2_GatherSubobjectDataForBlueprint(Blueprint, SubobjectHandles);
    
    UE_LOG(LogUnrealMCP, Warning, TEXT("=== SetComponentProperty DEBUG ==="));
    UE_LOG(LogUnrealMCP, Warning, TEXT("Looking for component '%s' in Blueprint '%s', found %d subobjects"), 
        *ComponentName, *Blueprint->GetName(), SubobjectHandles.Num());

    // Find the component by name
    UObject* ComponentTemplate = nullptr;
    for (const FSubobjectDataHandle& Handle : SubobjectHandles)
    {
        const FSubobjectData* Data = Handle.GetData();
        if (Data)
        {
            FName VarName = Data->GetVariableName();
            UObject* Obj = const_cast<UObject*>(Data->GetObject());
            UE_LOG(LogUnrealMCP, Warning, TEXT("  Subobject: Name='%s', Class='%s'"), 
                *VarName.ToString(), 
                Obj ? *Obj->GetClass()->GetName() : TEXT("NULL"));
            
            if (VarName == FName(*ComponentName))
            {
                ComponentTemplate = Obj;
                UE_LOG(LogUnrealMCP, Warning, TEXT("  ✓ FOUND MATCH: '%s'"), *ComponentName);
                break;
            }
        }
    }
    
    // If not found in subobjects, check inherited components on CDO
    if (!ComponentTemplate)
    {
        UObject* DefaultObject = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
        AActor* DefaultActor = Cast<AActor>(DefaultObject);
        if (DefaultActor)
        {
            TArray<UActorComponent*> AllComponents;
            DefaultActor->GetComponents(AllComponents);
            for (UActorComponent* Comp : AllComponents)
            {
                if (Comp && Comp->GetName() == ComponentName)
                {
                    ComponentTemplate = Comp;
                    break;
                }
            }
        }
    }
    
    if (!ComponentTemplate)
    {
        OutFailedProperties.Add(TEXT("component"), FString::Printf(TEXT("Component not found: %s"), *ComponentName));
        UE_LOG(LogUnrealMCP, Error, TEXT("Component '%s' not found in Blueprint '%s'"), *ComponentName, *Blueprint->GetName());
        return false;
    }
    
    UE_LOG(LogUnrealMCP, Log, TEXT("Found component template: %s"), *ComponentTemplate->GetClass()->GetName());
    
    // Build list of available properties once for error reporting
    for (TFieldIterator<FProperty> PropIt(ComponentTemplate->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Prop = *PropIt;
        if (Prop && (Prop->HasAnyPropertyFlags(CPF_Edit) || Prop->HasAnyPropertyFlags(CPF_BlueprintVisible)))
        {
            OutAvailableProperties.Add(Prop->GetName());
        }
    }
    
    // Iterate through all properties to set
    TArray<FString> PropertyNames;
    Properties->Values.GetKeys(PropertyNames);
    
    UE_LOG(LogUnrealMCP, Log, TEXT("Attempting to set %d properties"), PropertyNames.Num());
    
    for (const FString& PropertyName : PropertyNames)
    {
        const TSharedPtr<FJsonValue>& JsonValue = Properties->Values[PropertyName];
        
        UE_LOG(LogUnrealMCP, Log, TEXT("  Setting property: %s"), *PropertyName);
        if (!JsonValue.IsValid())
        {
            FString ErrorMsg = FString::Printf(TEXT("Invalid or null value provided for property '%s'"), *PropertyName);
            OutFailedProperties.Add(PropertyName, ErrorMsg);
            UE_LOG(LogUnrealMCP, Warning, TEXT("%s"), *ErrorMsg);
            continue;
        }
        
        // Find the property on the component
        FProperty* Property = FindFProperty<FProperty>(ComponentTemplate->GetClass(), *PropertyName);
        if (!Property)
        {
            FString ErrorMsg = FString::Printf(
                TEXT("Property '%s' not found on component '%s' (Class: %s)"),
                *PropertyName, 
                *ComponentName, 
                *ComponentTemplate->GetClass()->GetName()
            );
            
            OutFailedProperties.Add(PropertyName, ErrorMsg);
            UE_LOG(LogUnrealMCP, Warning, TEXT("%s"), *ErrorMsg);
            continue;
        }
        
        // Set the property using PropertyService
        FString PropertyError;
        if (FPropertyService::Get().SetObjectProperty(ComponentTemplate, PropertyName, JsonValue, PropertyError))
        {
            OutSuccessProperties.Add(PropertyName);
            UE_LOG(LogUnrealMCP, Log, TEXT("  ✓ Successfully set property '%s'"), *PropertyName);
        }
        else
        {
            FString ErrorMsg = FString::Printf(
                TEXT("Failed to set property '%s' on component '%s': %s"),
                *PropertyName,
                *ComponentName,
                *PropertyError
            );
            OutFailedProperties.Add(PropertyName, ErrorMsg);
            UE_LOG(LogUnrealMCP, Warning, TEXT("  ✗ %s"), *ErrorMsg);
        }
    }
    
    return OutSuccessProperties.Num() > 0;
}

FString FSetComponentPropertyCommand::CreateSuccessResponse(const TArray<FString>& SuccessProperties,
                                                           const TMap<FString, FString>& FailedProperties,
                                                           const TArray<FString>& AvailableProperties) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    
    // Set success based on whether at least one property was set successfully
    bool bHasSuccess = SuccessProperties.Num() > 0;
    ResponseObj->SetBoolField(TEXT("success"), bHasSuccess);
    
    // Add success properties array
    TArray<TSharedPtr<FJsonValue>> SuccessArray;
    for (const FString& Prop : SuccessProperties)
    {
        SuccessArray.Add(MakeShared<FJsonValueString>(Prop));
    }
    ResponseObj->SetArrayField(TEXT("success_properties"), SuccessArray);
    
    // Add failed properties object
    TSharedPtr<FJsonObject> FailedObj = MakeShared<FJsonObject>();
    for (const auto& Pair : FailedProperties)
    {
        FailedObj->SetStringField(Pair.Key, Pair.Value);
    }
    ResponseObj->SetObjectField(TEXT("failed_properties"), FailedObj);
    
    // Add available properties list only if there were failures
    if (FailedProperties.Num() > 0 && AvailableProperties.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> AvailableArray;
        for (const FString& Prop : AvailableProperties)
        {
            AvailableArray.Add(MakeShared<FJsonValueString>(Prop));
        }
        ResponseObj->SetArrayField(TEXT("available_properties"), AvailableArray);
    }
    
    // Add summary message
    if (bHasSuccess && FailedProperties.Num() > 0)
    {
        ResponseObj->SetStringField(TEXT("message"), 
            FString::Printf(TEXT("Partially successful: %d properties set, %d failed. See 'available_properties' for valid options."), 
                SuccessProperties.Num(), FailedProperties.Num()));
    }
    else if (bHasSuccess)
    {
        ResponseObj->SetStringField(TEXT("message"), 
            FString::Printf(TEXT("All %d properties set successfully"), SuccessProperties.Num()));
    }
    else
    {
        ResponseObj->SetStringField(TEXT("message"), 
            FString::Printf(TEXT("Failed to set all %d properties. See 'available_properties' for valid options."), FailedProperties.Num()));
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FSetComponentPropertyCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}



