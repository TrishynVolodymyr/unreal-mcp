#include "Commands/PCG/GetPCGGraphMetadataCommand.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGPin.h"
#include "PCGSettings.h"
#include "PCGCommon.h"
#include "PCGEdge.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"
#include "UObject/TextProperty.h"

// Helper: Convert pin AllowedTypes to a human-readable string
PRAGMA_DISABLE_DEPRECATION_WARNINGS
static FString PCGDataTypeToString(const FPCGDataTypeIdentifier& TypeId)
{
    EPCGDataType Type = static_cast<EPCGDataType>(TypeId);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
    if (Type == EPCGDataType::Any)    return TEXT("Any");
    if (Type == EPCGDataType::None)   return TEXT("None");

    TArray<FString> Parts;
    if (EnumHasAnyFlags(Type, EPCGDataType::Point))        Parts.Add(TEXT("Point"));
    if (EnumHasAnyFlags(Type, EPCGDataType::Spline))       Parts.Add(TEXT("Spline"));
    if (EnumHasAnyFlags(Type, EPCGDataType::Landscape))    Parts.Add(TEXT("Landscape"));
    if (EnumHasAnyFlags(Type, EPCGDataType::Volume))       Parts.Add(TEXT("Volume"));
    if (EnumHasAnyFlags(Type, EPCGDataType::Surface))      Parts.Add(TEXT("Surface"));
    if (EnumHasAnyFlags(Type, EPCGDataType::RenderTarget)) Parts.Add(TEXT("RenderTarget"));
    if (EnumHasAnyFlags(Type, EPCGDataType::Param))        Parts.Add(TEXT("Param"));
    if (EnumHasAnyFlags(Type, EPCGDataType::Texture))      Parts.Add(TEXT("Texture"));

    if (Parts.Num() == 0) return FString::Printf(TEXT("Unknown(%d)"), static_cast<int32>(Type));
    return FString::Join(Parts, TEXT("|"));
}

// Helper: Build a JSON array of pin descriptions
static TArray<TSharedPtr<FJsonValue>> BuildPinArray(const TArray<UPCGPin*>& Pins)
{
    TArray<TSharedPtr<FJsonValue>> PinArray;
    for (const UPCGPin* Pin : Pins)
    {
        if (!Pin) continue;

        TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
        PinObj->SetStringField(TEXT("label"), Pin->Properties.Label.ToString());
PRAGMA_DISABLE_DEPRECATION_WARNINGS
        PinObj->SetStringField(TEXT("type"), PCGDataTypeToString(Pin->Properties.AllowedTypes));
PRAGMA_ENABLE_DEPRECATION_WARNINGS

        // Collect connections: "OtherNodeName.OtherPinLabel"
        TArray<TSharedPtr<FJsonValue>> Connections;
        for (const UPCGEdge* Edge : Pin->Edges)
        {
            if (!Edge) continue;
            const UPCGPin* OtherPin = Edge->GetOtherPin(Pin);
            if (!OtherPin || !OtherPin->Node) continue;

            FString ConnStr = FString::Printf(TEXT("%s.%s"),
                *OtherPin->Node->GetName(),
                *OtherPin->Properties.Label.ToString());
            Connections.Add(MakeShared<FJsonValueString>(ConnStr));
        }
        PinObj->SetArrayField(TEXT("connected_to"), Connections);

        PinArray.Add(MakeShared<FJsonValueObject>(PinObj));
    }
    return PinArray;
}

// Helper: Build the pins object for a node
static TSharedPtr<FJsonObject> BuildPinsObject(const UPCGNode* Node)
{
    TSharedPtr<FJsonObject> PinsObj = MakeShared<FJsonObject>();
    PinsObj->SetArrayField(TEXT("inputs"), BuildPinArray(Node->GetInputPins()));
    PinsObj->SetArrayField(TEXT("outputs"), BuildPinArray(Node->GetOutputPins()));
    return PinsObj;
}

// Helper: Recursively build properties, expanding sub-objects and struct members
static TSharedPtr<FJsonObject> BuildPropertiesObject(const UObject* Object, const UStruct* Struct, const UStruct* StopAtClass, int32 Depth, const void* Container = nullptr)
{
    TSharedPtr<FJsonObject> PropsObj = MakeShared<FJsonObject>();
    if (Depth <= 0 || !Struct) return PropsObj;

    const void* ActualContainer = Container ? Container : Object;

    for (TFieldIterator<FProperty> PropIt(Struct); PropIt; ++PropIt)
    {
        FProperty* Prop = *PropIt;
        if (!Prop) continue;

        // Skip base class properties
        if (StopAtClass && Prop->GetOwnerClass() &&
            (Prop->GetOwnerClass() == UObject::StaticClass() || Prop->GetOwnerClass() == StopAtClass))
        {
            continue;
        }

        const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(ActualContainer);
        TSharedPtr<FJsonObject> PropInfoObj = MakeShared<FJsonObject>();
        PropInfoObj->SetStringField(TEXT("type"), Prop->GetCPPType());

        // Recurse into UObject sub-objects
        if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
        {
            UObject* SubObject = ObjProp->GetObjectPropertyValue(ValuePtr);
            if (SubObject && Depth > 1)
            {
                PropInfoObj->SetStringField(TEXT("class"), SubObject->GetClass()->GetName());
                PropInfoObj->SetObjectField(TEXT("properties"), BuildPropertiesObject(SubObject, SubObject->GetClass(), nullptr, Depth - 1));
            }
            else
            {
                FString ValueStr;
                Prop->ExportTextItem_Direct(ValueStr, ValuePtr, nullptr, nullptr, PPF_None);
                PropInfoObj->SetStringField(TEXT("value"), ValueStr);
            }
        }
        // Recurse into structs
        else if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
        {
            if (Depth > 1)
            {
                PropInfoObj->SetObjectField(TEXT("properties"), BuildPropertiesObject(nullptr, StructProp->Struct, nullptr, Depth - 1, ValuePtr));
            }
            else
            {
                FString ValueStr;
                Prop->ExportTextItem_Direct(ValueStr, ValuePtr, nullptr, nullptr, PPF_None);
                PropInfoObj->SetStringField(TEXT("value"), ValueStr);
            }
        }
        // Expand arrays — show element count and recurse if struct/object elements
        else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
        {
            FScriptArrayHelper ArrayHelper(ArrayProp, ValuePtr);
            PropInfoObj->SetNumberField(TEXT("count"), ArrayHelper.Num());

            if (Depth > 1 && ArrayHelper.Num() > 0)
            {
                TArray<TSharedPtr<FJsonValue>> Elements;
                int32 MaxElements = FMath::Min(ArrayHelper.Num(), 10); // Cap at 10 elements

                for (int32 i = 0; i < MaxElements; ++i)
                {
                    void* ElemPtr = ArrayHelper.GetRawPtr(i);
                    FProperty* Inner = ArrayProp->Inner;

                    if (const FStructProperty* InnerStruct = CastField<FStructProperty>(Inner))
                    {
                        TSharedPtr<FJsonObject> ElemObj = BuildPropertiesObject(nullptr, InnerStruct->Struct, nullptr, Depth - 1, ElemPtr);
                        Elements.Add(MakeShared<FJsonValueObject>(ElemObj));
                    }
                    else if (const FObjectProperty* InnerObj = CastField<FObjectProperty>(Inner))
                    {
                        UObject* SubObj = InnerObj->GetObjectPropertyValue(ElemPtr);
                        if (SubObj)
                        {
                            TSharedPtr<FJsonObject> ElemObj = BuildPropertiesObject(SubObj, SubObj->GetClass(), nullptr, Depth - 1);
                            Elements.Add(MakeShared<FJsonValueObject>(ElemObj));
                        }
                    }
                    else
                    {
                        FString ElemStr;
                        Inner->ExportTextItem_Direct(ElemStr, ElemPtr, nullptr, nullptr, PPF_None);
                        Elements.Add(MakeShared<FJsonValueString>(ElemStr));
                    }
                }
                PropInfoObj->SetArrayField(TEXT("elements"), Elements);
            }
            else
            {
                FString ValueStr;
                Prop->ExportTextItem_Direct(ValueStr, ValuePtr, nullptr, nullptr, PPF_None);
                PropInfoObj->SetStringField(TEXT("value"), ValueStr);
            }
        }
        else
        {
            FString ValueStr;
            Prop->ExportTextItem_Direct(ValueStr, ValuePtr, nullptr, nullptr, PPF_None);
            PropInfoObj->SetStringField(TEXT("value"), ValueStr);
        }

        PropsObj->SetObjectField(Prop->GetName(), PropInfoObj);
    }
    return PropsObj;
}

// Helper: Build a node JSON object (for regular nodes)
static int32 MaxPropertyDepth = 2;
static TSharedPtr<FJsonObject> BuildNodeObject(const UPCGNode* Node, bool bIncludeProperties)
{
    TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
    NodeObj->SetStringField(TEXT("id"), Node->GetName());

    // Title — try GetNodeTitle with EPCGNodeTitleType if available
    FString Title;
    const UPCGSettings* Settings = Node->GetSettings();
    if (Settings)
    {
        // UPCGSettings::GetNodeTitle exists in most UE 5.x PCG versions
        Title = Node->GetNodeTitle(EPCGNodeTitleType::ListView).ToString();
        NodeObj->SetStringField(TEXT("settings_class"), Settings->GetClass()->GetName());
    }
    else
    {
        Title = Node->GetName();
        NodeObj->SetStringField(TEXT("settings_class"), TEXT("None"));
    }
    NodeObj->SetStringField(TEXT("title"), Title);

    // Position (editor only)
#if WITH_EDITOR
    TArray<TSharedPtr<FJsonValue>> PosArr;
    PosArr.Add(MakeShared<FJsonValueNumber>(Node->PositionX));
    PosArr.Add(MakeShared<FJsonValueNumber>(Node->PositionY));
    NodeObj->SetArrayField(TEXT("position"), PosArr);
#endif

    // Pins
    NodeObj->SetObjectField(TEXT("pins"), BuildPinsObject(Node));

    // Optional: settings properties via UE reflection (with sub-object recursion)
    if (bIncludeProperties && Settings)
    {
        NodeObj->SetObjectField(TEXT("properties"), BuildPropertiesObject(Settings, Settings->GetClass(), UPCGSettings::StaticClass(), MaxPropertyDepth));
    }

    return NodeObj;
}

FString FGetPCGGraphMetadataCommand::Execute(const FString& Parameters)
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString GraphPath;
    if (!JsonObject->TryGetStringField(TEXT("graph_path"), GraphPath))
    {
        return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
    }

    bool bIncludeProperties = false;
    JsonObject->TryGetBoolField(TEXT("include_properties"), bIncludeProperties);

    FString NodeIdFilter;
    JsonObject->TryGetStringField(TEXT("node_id"), NodeIdFilter);

    int32 MaxDepth = 2;
    JsonObject->TryGetNumberField(TEXT("max_depth"), MaxDepth);
    MaxDepth = FMath::Clamp(MaxDepth, 1, 5);
    MaxPropertyDepth = MaxDepth;

    // Load the PCG Graph asset
    UPCGGraph* Graph = LoadObject<UPCGGraph>(nullptr, *GraphPath);
    if (!Graph)
    {
        return CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found at path: %s"), *GraphPath));
    }

    // Build response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("graph_path"), GraphPath);

    // Input node
    UPCGNode* InputNode = Graph->GetInputNode();
    if (InputNode)
    {
        TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();
        InputObj->SetStringField(TEXT("id"), InputNode->GetName());
        InputObj->SetObjectField(TEXT("pins"), BuildPinsObject(InputNode));
        ResponseObj->SetObjectField(TEXT("input_node"), InputObj);
    }

    // Output node
    UPCGNode* OutputNode = Graph->GetOutputNode();
    if (OutputNode)
    {
        TSharedPtr<FJsonObject> OutputObj = MakeShared<FJsonObject>();
        OutputObj->SetStringField(TEXT("id"), OutputNode->GetName());
        OutputObj->SetObjectField(TEXT("pins"), BuildPinsObject(OutputNode));
        ResponseObj->SetObjectField(TEXT("output_node"), OutputObj);
    }

    // All other nodes (GetNodes excludes Input/Output)
    const TArray<UPCGNode*>& Nodes = Graph->GetNodes();
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    for (const UPCGNode* Node : Nodes)
    {
        if (!Node) continue;
        // Filter by node_id if specified
        if (!NodeIdFilter.IsEmpty() && Node->GetName() != NodeIdFilter)
        {
            continue;
        }
        NodesArray.Add(MakeShared<FJsonValueObject>(BuildNodeObject(Node, bIncludeProperties)));
    }
    ResponseObj->SetArrayField(TEXT("nodes"), NodesArray);
    ResponseObj->SetNumberField(TEXT("node_count"), NodesArray.Num());

    // Serialize response
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetPCGGraphMetadataCommand::GetCommandName() const
{
    return TEXT("get_pcg_graph_metadata");
}

bool FGetPCGGraphMetadataCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonObject->HasField(TEXT("graph_path"));
}

FString FGetPCGGraphMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
