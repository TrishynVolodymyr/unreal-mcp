#include "Commands/PCG/SetPCGNodePropertyCommand.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGSettings.h"
#include "UObject/UnrealType.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/SavePackage.h"
#include "Utils/PCGEditorRefreshUtils.h"
#include "Utils/PCGNodeUtils.h"

FSetPCGNodePropertyCommand::FSetPCGNodePropertyCommand()
{
}

FString FSetPCGNodePropertyCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    // Parse required parameters
    FString GraphPath;
    if (!JsonObject->TryGetStringField(TEXT("graph_path"), GraphPath))
    {
        return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
    }

    FString NodeId;
    if (!JsonObject->TryGetStringField(TEXT("node_id"), NodeId))
    {
        return CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
    }

    FString PropertyName;
    if (!JsonObject->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    FString PropertyValue;
    if (!JsonObject->TryGetStringField(TEXT("property_value"), PropertyValue))
    {
        return CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }

    // Load the PCG Graph asset
    UPCGGraph* Graph = LoadObject<UPCGGraph>(nullptr, *GraphPath);
    if (!Graph)
    {
        return CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found at path: %s"), *GraphPath));
    }

    // Find the node
    UPCGNode* Node = PCGNodeUtils::FindNodeByName(Graph, NodeId);
    if (!Node)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
    }

    // Get the settings object (GetSettings() returns UPCGSettings* — non-const, mutable)
    UPCGSettings* Settings = Node->GetSettings();
    if (!Settings)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Node '%s' has no settings"), *NodeId));
    }

    // Find the property on the settings object
    FProperty* Property = Settings->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Property '%s' not found on settings class '%s'"),
            *PropertyName, *Settings->GetClass()->GetName()));
    }

    // Get property type string before modifying
    FString PropertyType = GetPropertyTypeString(Property);

    // Get pointer to the property's value within the settings object
    void* PropertyAddr = Property->ContainerPtrToValuePtr<void>(Settings);

#if WITH_EDITOR
    Settings->PreEditChange(Property);
#endif

    // Use ImportText to set the value from string — handles int, float, bool, FName, enums, structs, object paths, etc.
    const TCHAR* Result = Property->ImportText_Direct(*PropertyValue, PropertyAddr, Settings, PPF_None);

    if (!Result)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to parse value '%s' for property '%s' (type: %s)"),
            *PropertyValue, *PropertyName, *PropertyType));
    }

#if WITH_EDITOR
    FPropertyChangedEvent PropertyChangedEvent(Property);
    static_cast<UObject*>(Settings)->PostEditChangeProperty(PropertyChangedEvent);
#endif

    // Notify editor and save graph to disk
#if WITH_EDITOR
    Graph->OnGraphChangedDelegate.Broadcast(Graph, EPCGChangeType::Settings);
#endif
    Settings->MarkPackageDirty();
    Graph->MarkPackageDirty();
    {
        UPackage* Package = Graph->GetOutermost();
        FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Package, Graph, *PackageFileName, SaveArgs);
    }

    // Refresh the PCG editor graph if it's open
    PCGEditorRefreshUtils::RefreshEditorGraph(Graph);

    return CreateSuccessResponse(NodeId, PropertyName, PropertyValue, PropertyType);
}

FString FSetPCGNodePropertyCommand::GetCommandName() const
{
    return TEXT("set_pcg_node_property");
}

bool FSetPCGNodePropertyCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonObject->HasField(TEXT("graph_path")) &&
           JsonObject->HasField(TEXT("node_id")) &&
           JsonObject->HasField(TEXT("property_name")) &&
           JsonObject->HasField(TEXT("property_value"));
}

FString FSetPCGNodePropertyCommand::GetPropertyTypeString(const FProperty* Property) const
{
    if (!Property) return TEXT("unknown");

    if (Property->IsA<FBoolProperty>()) return TEXT("bool");
    if (Property->IsA<FIntProperty>()) return TEXT("int32");
    if (Property->IsA<FInt64Property>()) return TEXT("int64");
    if (Property->IsA<FFloatProperty>()) return TEXT("float");
    if (Property->IsA<FDoubleProperty>()) return TEXT("double");
    if (Property->IsA<FStrProperty>()) return TEXT("FString");
    if (Property->IsA<FNameProperty>()) return TEXT("FName");
    if (Property->IsA<FTextProperty>()) return TEXT("FText");

    if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        if (UEnum* Enum = EnumProp->GetEnum())
        {
            return Enum->GetName();
        }
        return TEXT("enum");
    }

    if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
    {
        if (UEnum* Enum = ByteProp->GetIntPropertyEnum())
        {
            return Enum->GetName();
        }
        return TEXT("uint8");
    }

    if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        return StructProp->Struct->GetName();
    }

    if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
    {
        return FString::Printf(TEXT("Object<%s>"), *ObjProp->PropertyClass->GetName());
    }

    if (const FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
    {
        return FString::Printf(TEXT("SoftObject<%s>"), *SoftObjProp->PropertyClass->GetName());
    }

    if (Property->IsA<FArrayProperty>()) return TEXT("Array");

    return Property->GetCPPType();
}

FString FSetPCGNodePropertyCommand::CreateSuccessResponse(const FString& NodeId, const FString& PropertyName, const FString& Value, const FString& PropertyType) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("node_id"), NodeId);
    ResponseObj->SetStringField(TEXT("property_name"), PropertyName);
    ResponseObj->SetStringField(TEXT("value"), Value);
    ResponseObj->SetStringField(TEXT("property_type"), PropertyType);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetPCGNodePropertyCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
