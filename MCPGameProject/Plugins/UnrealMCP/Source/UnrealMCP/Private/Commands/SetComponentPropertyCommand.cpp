#include "Commands/SetComponentPropertyCommand.h"
#include "Services/PropertyService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"

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
    
    if (!SetComponentProperties(Blueprint, ComponentName, Properties, SuccessProperties, FailedProperties))
    {
        return CreateErrorResponse(TEXT("Failed to set any component properties"));
    }
    
    return CreateSuccessResponse(SuccessProperties, FailedProperties);
}

FString FSetComponentPropertyCommand::GetCommandName() const
{
    return TEXT("set_component_property");
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
    }
    else
    {
        // Try to get as string and parse
        FString KwargsString;
        if (JsonObject->TryGetStringField(TEXT("kwargs"), KwargsString))
        {
            TSharedRef<TJsonReader<>> KwargsReader = TJsonReaderFactory<>::Create(KwargsString);
            TSharedPtr<FJsonObject> ParsedObject;
            if (FJsonSerializer::Deserialize(KwargsReader, ParsedObject) && ParsedObject.IsValid())
            {
                OutProperties = ParsedObject;
            }
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
                                                         TMap<FString, FString>& OutFailedProperties) const
{
    // Find the component in the blueprint
    USCS_Node* ComponentNode = nullptr;
    UObject* ComponentTemplate = nullptr;
    
    if (Blueprint->SimpleConstructionScript)
    {
        for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (Node && Node->GetVariableName().ToString() == ComponentName)
            {
                ComponentNode = Node;
                ComponentTemplate = Node->ComponentTemplate;
                break;
            }
        }
    }
    
    // If not found in construction script, search inherited components on the CDO
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
        return false;
    }
    
    // Iterate through all properties to set
    TArray<FString> PropertyNames;
    Properties->Values.GetKeys(PropertyNames);
    
    for (const FString& PropertyName : PropertyNames)
    {
        TSharedPtr<FJsonValue> PropertyValue = Properties->TryGetField(PropertyName);
        if (!PropertyValue.IsValid())
        {
            OutFailedProperties.Add(PropertyName, TEXT("Invalid property value"));
            continue;
        }
        
        // Find the property on the component
        FProperty* Property = FindFProperty<FProperty>(ComponentTemplate->GetClass(), *PropertyName);
        if (!Property)
        {
            OutFailedProperties.Add(PropertyName, FString::Printf(TEXT("Property '%s' not found on component '%s' (Class: %s)"), 
                                                                 *PropertyName, *ComponentName, *ComponentTemplate->GetClass()->GetName()));
            continue;
        }
        
        // Set the property using PropertyService
        FString PropertyError;
        if (FPropertyService::Get().SetObjectProperty(ComponentTemplate, PropertyName, PropertyValue, PropertyError))
        {
            OutSuccessProperties.Add(PropertyName);
        }
        else
        {
            OutFailedProperties.Add(PropertyName, PropertyError);
        }
    }
    
    return OutSuccessProperties.Num() > 0;
}

FString FSetComponentPropertyCommand::CreateSuccessResponse(const TArray<FString>& SuccessProperties,
                                                           const TMap<FString, FString>& FailedProperties) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    
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