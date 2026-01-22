// MetaSoundOperations.cpp - MetaSound methods for FSoundService
// This file implements the MetaSound operations of FSoundService

#include "Services/SoundService.h"
#include "MetasoundSource.h"
#include "MetasoundBuilderSubsystem.h"
#include "MetasoundBuilderBase.h"
#include "MetasoundFrontendDocumentBuilder.h"
#include "MetasoundFrontendSearchEngine.h"
#include "MetasoundAssetManager.h"
#include "MetasoundAssetBase.h"
#include "MetasoundUObjectRegistry.h"
#include "MetasoundDocumentBuilderRegistry.h"
#include "MetasoundEditorSubsystem.h"
#include "NodeTemplates/MetasoundFrontendNodeTemplateInput.h"
#include "Dom/JsonValue.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"

// ============================================================================
// MetaSound Operations
// ============================================================================

UMetaSoundSource* FSoundService::CreateMetaSoundSource(const FMetaSoundSourceParams& Params, FString& OutAssetPath, FString& OutError)
{
    // Get the builder subsystem
    UMetaSoundBuilderSubsystem* BuilderSubsystem = UMetaSoundBuilderSubsystem::Get();
    if (!BuilderSubsystem)
    {
        OutError = TEXT("MetaSound Builder Subsystem not available");
        return nullptr;
    }

    // Determine output format
    EMetaSoundOutputAudioFormat OutputFormat = EMetaSoundOutputAudioFormat::Stereo;
    if (Params.OutputFormat.Equals(TEXT("Mono"), ESearchCase::IgnoreCase))
    {
        OutputFormat = EMetaSoundOutputAudioFormat::Mono;
    }
    else if (Params.OutputFormat.Equals(TEXT("Quad"), ESearchCase::IgnoreCase))
    {
        OutputFormat = EMetaSoundOutputAudioFormat::Quad;
    }
    else if (Params.OutputFormat.Equals(TEXT("FiveDotOne"), ESearchCase::IgnoreCase) || Params.OutputFormat.Equals(TEXT("5.1"), ESearchCase::IgnoreCase))
    {
        OutputFormat = EMetaSoundOutputAudioFormat::FiveDotOne;
    }
    else if (Params.OutputFormat.Equals(TEXT("SevenDotOne"), ESearchCase::IgnoreCase) || Params.OutputFormat.Equals(TEXT("7.1"), ESearchCase::IgnoreCase))
    {
        OutputFormat = EMetaSoundOutputAudioFormat::SevenDotOne;
    }

    // Create builder name
    FName BuilderName = FName(*FString::Printf(TEXT("MCP_Builder_%s"), *Params.AssetName));

    // Create the source builder
    EMetaSoundBuilderResult Result;
    FMetaSoundBuilderNodeOutputHandle OnPlayOutput;
    FMetaSoundBuilderNodeInputHandle OnFinishedInput;
    TArray<FMetaSoundBuilderNodeInputHandle> AudioOutInputs;

    UMetaSoundSourceBuilder* SourceBuilder = BuilderSubsystem->CreateSourceBuilder(
        BuilderName,
        OnPlayOutput,
        OnFinishedInput,
        AudioOutInputs,
        Result,
        OutputFormat,
        Params.bIsOneShot
    );

    if (Result != EMetaSoundBuilderResult::Succeeded || !SourceBuilder)
    {
        OutError = TEXT("Failed to create MetaSound source builder");
        return nullptr;
    }

    // Prevent builder from being garbage collected during asset creation
    SourceBuilder->AddToRoot();

    // Use IAssetTools to create the asset properly (handles package creation and saving)
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    UE_LOG(LogSoundService, Log, TEXT("Creating MetaSound asset '%s' in folder '%s'"), *Params.AssetName, *Params.FolderPath);

    UObject* NewMetaSoundObject = AssetTools.CreateAsset(
        Params.AssetName,
        Params.FolderPath,
        UMetaSoundSource::StaticClass(),
        nullptr  // No factory needed
    );

    if (!NewMetaSoundObject)
    {
        SourceBuilder->RemoveFromRoot();
        BuilderSubsystem->UnregisterBuilder(BuilderName);
        OutError = TEXT("Failed to create MetaSound asset via AssetTools");
        return nullptr;
    }

    UMetaSoundSource* MetaSoundSource = Cast<UMetaSoundSource>(NewMetaSoundObject);
    if (!MetaSoundSource)
    {
        SourceBuilder->RemoveFromRoot();
        BuilderSubsystem->UnregisterBuilder(BuilderName);
        OutError = FString::Printf(TEXT("Created asset is not a MetaSound Source. Actual type: %s"),
            NewMetaSoundObject ? *NewMetaSoundObject->GetClass()->GetName() : TEXT("null"));
        return nullptr;
    }

    // Initialize node locations
    SourceBuilder->InitNodeLocations();

    // Build the MetaSound with the existing asset
    FMetaSoundBuilderOptions BuildOptions;
    BuildOptions.Name = FName(*Params.AssetName);
    BuildOptions.bForceUniqueClassName = true;
    BuildOptions.bAddToRegistry = true;
    BuildOptions.ExistingMetaSound = MetaSoundSource;

    UE_LOG(LogSoundService, Log, TEXT("Building MetaSound '%s'"), *Params.AssetName);

    SourceBuilder->Build(BuildOptions);

    // Get the builder for the new asset and inject template nodes
    Metasound::Engine::FDocumentBuilderRegistry& BuilderRegistry = Metasound::Engine::FDocumentBuilderRegistry::GetChecked();
    UMetaSoundBuilderBase& NewDocBuilder = BuilderRegistry.FindOrBeginBuilding(*MetaSoundSource);

    EMetaSoundBuilderResult InjectResult = EMetaSoundBuilderResult::Failed;
    constexpr bool bForceNodeCreation = true;
    NewDocBuilder.InjectInputTemplateNodes(bForceNodeCreation, InjectResult);

    // Rebuild referenced asset classes
    FMetasoundAssetBase& Asset = NewDocBuilder.GetBuilder().GetMetasoundAsset();
    Asset.RebuildReferencedAssetClasses();

    // Mark asset as modified so it gets saved
    MetaSoundSource->MarkPackageDirty();

    // Save the asset
    if (!SaveAsset(MetaSoundSource, OutError))
    {
        SourceBuilder->RemoveFromRoot();
        BuilderSubsystem->UnregisterBuilder(BuilderName);
        return nullptr;
    }

    OutAssetPath = MetaSoundSource->GetPackage()->GetPathName();
    UE_LOG(LogSoundService, Log, TEXT("Created MetaSound Source: %s"), *OutAssetPath);

    // Clean up
    SourceBuilder->RemoveFromRoot();
    BuilderSubsystem->UnregisterBuilder(BuilderName);

    return MetaSoundSource;
}

bool FSoundService::GetMetaSoundMetadata(const FString& MetaSoundPath, TSharedPtr<FJsonObject>& OutMetadata, FString& OutError)
{
    UMetaSoundSource* MetaSound = FindMetaSoundSource(MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *MetaSoundPath);
        return false;
    }

    OutMetadata = MakeShared<FJsonObject>();
    OutMetadata->SetStringField(TEXT("name"), MetaSound->GetName());
    OutMetadata->SetStringField(TEXT("path"), MetaSoundPath);

    // Get the document interface
    const FMetasoundFrontendDocument& Document = MetaSound->GetConstDocument();

    // Metadata about the root graph
    OutMetadata->SetStringField(TEXT("class_name"), Document.RootGraph.Metadata.GetClassName().ToString());

    // Collect inputs
    TArray<TSharedPtr<FJsonValue>> InputsArray;
    for (const FMetasoundFrontendClassInput& Input : Document.RootGraph.Interface.Inputs)
    {
        TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();
        InputObj->SetStringField(TEXT("name"), Input.Name.ToString());
        InputObj->SetStringField(TEXT("type"), Input.TypeName.ToString());
        InputObj->SetStringField(TEXT("node_id"), Input.NodeID.ToString());
        InputObj->SetStringField(TEXT("vertex_id"), Input.VertexID.ToString());
        InputsArray.Add(MakeShared<FJsonValueObject>(InputObj));
    }
    OutMetadata->SetArrayField(TEXT("inputs"), InputsArray);

    // Collect outputs
    TArray<TSharedPtr<FJsonValue>> OutputsArray;
    for (const FMetasoundFrontendClassOutput& Output : Document.RootGraph.Interface.Outputs)
    {
        TSharedPtr<FJsonObject> OutputObj = MakeShared<FJsonObject>();
        OutputObj->SetStringField(TEXT("name"), Output.Name.ToString());
        OutputObj->SetStringField(TEXT("type"), Output.TypeName.ToString());
        OutputObj->SetStringField(TEXT("node_id"), Output.NodeID.ToString());
        OutputObj->SetStringField(TEXT("vertex_id"), Output.VertexID.ToString());
        OutputsArray.Add(MakeShared<FJsonValueObject>(OutputObj));
    }
    OutMetadata->SetArrayField(TEXT("outputs"), OutputsArray);

    // Get the default graph page (UE 5.5+ uses PagedGraphs instead of deprecated Graph field)
    const FMetasoundFrontendGraph& DefaultGraph = Document.RootGraph.GetConstDefaultGraph();

    // Collect nodes - need to look up class info from Dependencies
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    for (const FMetasoundFrontendNode& Node : DefaultGraph.Nodes)
    {
        TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
        NodeObj->SetStringField(TEXT("id"), Node.GetID().ToString());
        NodeObj->SetStringField(TEXT("class_id"), Node.ClassID.ToString());
        NodeObj->SetStringField(TEXT("name"), Node.Name.ToString());

        // Look up class name from dependencies
        for (const FMetasoundFrontendClass& Dependency : Document.Dependencies)
        {
            if (Dependency.ID == Node.ClassID)
            {
                const FMetasoundFrontendClassName& ClassName = Dependency.Metadata.GetClassName();
                NodeObj->SetStringField(TEXT("class_name"), ClassName.Name.ToString());
                NodeObj->SetStringField(TEXT("class_namespace"), ClassName.Namespace.ToString());
                break;
            }
        }

        // Node inputs
        TArray<TSharedPtr<FJsonValue>> NodeInputsArray;
        for (const FMetasoundFrontendVertex& Vertex : Node.Interface.Inputs)
        {
            TSharedPtr<FJsonObject> VertexObj = MakeShared<FJsonObject>();
            VertexObj->SetStringField(TEXT("name"), Vertex.Name.ToString());
            VertexObj->SetStringField(TEXT("type"), Vertex.TypeName.ToString());
            VertexObj->SetStringField(TEXT("id"), Vertex.VertexID.ToString());
            NodeInputsArray.Add(MakeShared<FJsonValueObject>(VertexObj));
        }
        NodeObj->SetArrayField(TEXT("inputs"), NodeInputsArray);

        // Node outputs
        TArray<TSharedPtr<FJsonValue>> NodeOutputsArray;
        for (const FMetasoundFrontendVertex& Vertex : Node.Interface.Outputs)
        {
            TSharedPtr<FJsonObject> VertexObj = MakeShared<FJsonObject>();
            VertexObj->SetStringField(TEXT("name"), Vertex.Name.ToString());
            VertexObj->SetStringField(TEXT("type"), Vertex.TypeName.ToString());
            VertexObj->SetStringField(TEXT("id"), Vertex.VertexID.ToString());
            NodeOutputsArray.Add(MakeShared<FJsonValueObject>(VertexObj));
        }
        NodeObj->SetArrayField(TEXT("outputs"), NodeOutputsArray);

        NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
    }
    OutMetadata->SetArrayField(TEXT("nodes"), NodesArray);

    // Collect edges
    TArray<TSharedPtr<FJsonValue>> EdgesArray;
    for (const FMetasoundFrontendEdge& Edge : DefaultGraph.Edges)
    {
        TSharedPtr<FJsonObject> EdgeObj = MakeShared<FJsonObject>();
        EdgeObj->SetStringField(TEXT("from_node_id"), Edge.FromNodeID.ToString());
        EdgeObj->SetStringField(TEXT("from_vertex_id"), Edge.FromVertexID.ToString());
        EdgeObj->SetStringField(TEXT("to_node_id"), Edge.ToNodeID.ToString());
        EdgeObj->SetStringField(TEXT("to_vertex_id"), Edge.ToVertexID.ToString());
        EdgesArray.Add(MakeShared<FJsonValueObject>(EdgeObj));
    }
    OutMetadata->SetArrayField(TEXT("edges"), EdgesArray);

    OutMetadata->SetNumberField(TEXT("node_count"), DefaultGraph.Nodes.Num());
    OutMetadata->SetNumberField(TEXT("edge_count"), DefaultGraph.Edges.Num());

    UE_LOG(LogSoundService, Log, TEXT("Retrieved metadata for MetaSound: %s"), *MetaSoundPath);
    return true;
}

bool FSoundService::AddMetaSoundNode(const FMetaSoundNodeParams& Params, FString& OutNodeId, FString& OutError)
{
#if WITH_EDITORONLY_DATA
    using namespace Metasound::Frontend;

    UMetaSoundSource* MetaSound = FindMetaSoundSource(Params.MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *Params.MetaSoundPath);
        return false;
    }

    // Get the builder for this MetaSound
    Metasound::Engine::FDocumentBuilderRegistry& BuilderRegistry = Metasound::Engine::FDocumentBuilderRegistry::GetChecked();
    UMetaSoundSourceBuilder& Builder = BuilderRegistry.FindOrBeginBuilding<UMetaSoundSourceBuilder>(*MetaSound);

    // Create the class name from namespace, name, and variant
    FMetasoundFrontendClassName ClassName;
    ClassName.Namespace = FName(*Params.NodeNamespace);
    ClassName.Name = FName(*Params.NodeClassName);
    if (!Params.NodeVariant.IsEmpty())
    {
        ClassName.Variant = FName(*Params.NodeVariant);
    }

    UE_LOG(LogSoundService, Log, TEXT("Adding node: Namespace='%s', Name='%s', Variant='%s'"),
        *ClassName.Namespace.ToString(), *ClassName.Name.ToString(), *ClassName.Variant.ToString());

    // Mark the MetaSound as modified before making changes
    MetaSound->Modify();

    // Add the node using the builder
    EMetaSoundBuilderResult Result;
    FMetaSoundNodeHandle NodeHandle = Builder.AddNodeByClassName(ClassName, Result, 1);

    if (Result != EMetaSoundBuilderResult::Succeeded || !NodeHandle.IsSet())
    {
        OutError = FString::Printf(TEXT("Failed to add node '%s::%s' (variant: '%s'). Use search_metasound_palette to find valid node names."),
            *Params.NodeNamespace, *Params.NodeClassName, *Params.NodeVariant);
        return false;
    }

    // Get the node ID from the handle
    OutNodeId = NodeHandle.NodeID.ToString();

    // CRITICAL: Set the node's location in the document so it appears in the editor graph
    // Without this, the editor's SynchronizeNodes won't visualize the node
    FVector2D NodeLocation(static_cast<float>(Params.PosX), static_cast<float>(Params.PosY));
    Builder.SetNodeLocation(NodeHandle, NodeLocation, Result);
    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to set node location for '%s::%s', node may not appear in editor graph"),
            *Params.NodeNamespace, *Params.NodeClassName);
        // Continue anyway - node was added, just location may be wrong
    }

    // CRITICAL: Mark the node as modified in the document's ModifyContext
    // This signals to the editor graph that a node was added and needs synchronization
    FMetasoundAssetBase* MetaSoundAsset = Metasound::IMetasoundUObjectRegistry::Get().GetObjectAsAssetBase(MetaSound);
    if (MetaSoundAsset)
    {
        MetaSoundAsset->GetModifyContext().AddNodeIDModified(NodeHandle.NodeID);
    }

    // Register the graph with the frontend to synchronize the editor graph
    // This creates the visual editor node representation
    UMetaSoundEditorSubsystem::GetChecked().RegisterGraphWithFrontend(*MetaSound, true);

    // Save the MetaSound
    if (!SaveAsset(MetaSound, OutError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save MetaSound after adding node: %s"), *OutError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Added node '%s::%s' (ID: %s) to MetaSound: %s"),
        *Params.NodeNamespace, *Params.NodeClassName, *OutNodeId, *Params.MetaSoundPath);

    return true;
#else
    OutError = TEXT("MetaSound editing requires editor data");
    return false;
#endif
}

bool FSoundService::ConnectMetaSoundNodes(const FString& MetaSoundPath, const FString& SourceNodeId, const FString& SourcePinName, const FString& TargetNodeId, const FString& TargetPinName, FString& OutError)
{
    UMetaSoundSource* MetaSound = FindMetaSoundSource(MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *MetaSoundPath);
        return false;
    }

    // Get or create a builder for this existing MetaSound using the document builder registry
#if WITH_EDITORONLY_DATA
    Metasound::Engine::FDocumentBuilderRegistry& BuilderRegistry = Metasound::Engine::FDocumentBuilderRegistry::GetChecked();
    UMetaSoundSourceBuilder& Builder = BuilderRegistry.FindOrBeginBuilding<UMetaSoundSourceBuilder>(*MetaSound);
#else
    OutError = TEXT("MetaSound editing requires editor data");
    return false;
#endif

    // Parse node IDs
    FGuid SourceGuid, TargetGuid;
    if (!FGuid::Parse(SourceNodeId, SourceGuid))
    {
        OutError = FString::Printf(TEXT("Invalid source node ID format: %s"), *SourceNodeId);
        return false;
    }
    if (!FGuid::Parse(TargetNodeId, TargetGuid))
    {
        OutError = FString::Printf(TEXT("Invalid target node ID format: %s"), *TargetNodeId);
        return false;
    }

    // Create node handles - use FMetaSoundNodeHandle for generic nodes
    FMetaSoundNodeHandle SourceHandle;
    SourceHandle.NodeID = SourceGuid;

    FMetaSoundNodeHandle TargetHandle;
    TargetHandle.NodeID = TargetGuid;

    // Get output handle from source node
    EMetaSoundBuilderResult Result;
    FMetaSoundBuilderNodeOutputHandle OutputHandle = Builder.FindNodeOutputByName(SourceHandle, FName(*SourcePinName), Result);
    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Source pin '%s' not found on node %s"), *SourcePinName, *SourceNodeId);
        return false;
    }

    // Get input handle from target node
    FMetaSoundBuilderNodeInputHandle InputHandle = Builder.FindNodeInputByName(TargetHandle, FName(*TargetPinName), Result);
    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Target pin '%s' not found on node %s"), *TargetPinName, *TargetNodeId);
        return false;
    }

    // Mark the MetaSound as modified before making changes
    MetaSound->Modify();

    // Make the connection
    Builder.ConnectNodes(OutputHandle, InputHandle, Result);
    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Failed to connect '%s.%s' to '%s.%s'"),
            *SourceNodeId, *SourcePinName, *TargetNodeId, *TargetPinName);
        return false;
    }

    // Mark both connected nodes as modified for synchronization
    FMetasoundAssetBase* MetaSoundAsset = Metasound::IMetasoundUObjectRegistry::Get().GetObjectAsAssetBase(MetaSound);
    if (MetaSoundAsset)
    {
        MetaSoundAsset->GetModifyContext().AddNodeIDModified(SourceGuid);
        MetaSoundAsset->GetModifyContext().AddNodeIDModified(TargetGuid);
    }

    // Register the graph with the frontend to synchronize the editor graph
    UMetaSoundEditorSubsystem::GetChecked().RegisterGraphWithFrontend(*MetaSound, true);

    // Save the MetaSound
    if (!SaveAsset(MetaSound, OutError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save MetaSound after connecting nodes: %s"), *OutError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Connected '%s.%s' -> '%s.%s' in MetaSound: %s"),
        *SourceNodeId, *SourcePinName, *TargetNodeId, *TargetPinName, *MetaSoundPath);

    return true;
}

bool FSoundService::SetMetaSoundNodeInput(const FString& MetaSoundPath, const FString& NodeId, const FString& InputName, const TSharedPtr<FJsonValue>& Value, FString& OutError)
{
    UMetaSoundSource* MetaSound = FindMetaSoundSource(MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *MetaSoundPath);
        return false;
    }

    // Get the builder subsystem for literal creation helpers
    UMetaSoundBuilderSubsystem* BuilderSubsystem = UMetaSoundBuilderSubsystem::Get();
    if (!BuilderSubsystem)
    {
        OutError = TEXT("MetaSound Builder Subsystem not available");
        return false;
    }

    // Get or create a builder for this existing MetaSound using the document builder registry
#if WITH_EDITORONLY_DATA
    Metasound::Engine::FDocumentBuilderRegistry& BuilderRegistry = Metasound::Engine::FDocumentBuilderRegistry::GetChecked();
    UMetaSoundSourceBuilder& Builder = BuilderRegistry.FindOrBeginBuilding<UMetaSoundSourceBuilder>(*MetaSound);
#else
    OutError = TEXT("MetaSound editing requires editor data");
    return false;
#endif

    // Parse node ID
    FGuid NodeGuid;
    if (!FGuid::Parse(NodeId, NodeGuid))
    {
        OutError = FString::Printf(TEXT("Invalid node ID format: %s"), *NodeId);
        return false;
    }

    // Create node handle - use FMetaSoundNodeHandle for generic nodes
    FMetaSoundNodeHandle NodeHandle;
    NodeHandle.NodeID = NodeGuid;

    // Find the input
    EMetaSoundBuilderResult Result;
    FMetaSoundBuilderNodeInputHandle InputHandle = Builder.FindNodeInputByName(NodeHandle, FName(*InputName), Result);
    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Input '%s' not found on node %s"), *InputName, *NodeId);
        return false;
    }

    // Set the value based on JSON type - need FName references for literal creation
    FName DataType;
    if (Value->Type == EJson::Number)
    {
        FMetasoundFrontendLiteral Literal = BuilderSubsystem->CreateFloatMetaSoundLiteral(static_cast<float>(Value->AsNumber()), DataType);
        Builder.SetNodeInputDefault(InputHandle, Literal, Result);
    }
    else if (Value->Type == EJson::Boolean)
    {
        FMetasoundFrontendLiteral Literal = BuilderSubsystem->CreateBoolMetaSoundLiteral(Value->AsBool(), DataType);
        Builder.SetNodeInputDefault(InputHandle, Literal, Result);
    }
    else if (Value->Type == EJson::String)
    {
        FMetasoundFrontendLiteral Literal = BuilderSubsystem->CreateStringMetaSoundLiteral(Value->AsString(), DataType);
        Builder.SetNodeInputDefault(InputHandle, Literal, Result);
    }
    else
    {
        OutError = TEXT("Unsupported value type. Supported: number, boolean, string");
        return false;
    }

    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Failed to set input value for '%s' on node %s"), *InputName, *NodeId);
        return false;
    }

    // CRITICAL: Sync builder document changes to the MetaSound object
    MetaSound->ConformObjectToDocument();

    // Save the MetaSound
    MetaSound->Modify();
    if (!SaveAsset(MetaSound, OutError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save MetaSound after setting input: %s"), *OutError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Set input '%s' on node '%s' in MetaSound: %s"), *InputName, *NodeId, *MetaSoundPath);

    return true;
}

bool FSoundService::AddMetaSoundInput(const FMetaSoundInputParams& Params, FString& OutInputNodeId, FString& OutError)
{
    UMetaSoundSource* MetaSound = FindMetaSoundSource(Params.MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *Params.MetaSoundPath);
        return false;
    }

    // Get or create a builder for this existing MetaSound using the document builder registry
#if WITH_EDITORONLY_DATA
    Metasound::Engine::FDocumentBuilderRegistry& BuilderRegistry = Metasound::Engine::FDocumentBuilderRegistry::GetChecked();
    UMetaSoundSourceBuilder& Builder = BuilderRegistry.FindOrBeginBuilding<UMetaSoundSourceBuilder>(*MetaSound);
#else
    OutError = TEXT("MetaSound editing requires editor data");
    return false;
#endif

    // Map data type string to MetaSound type name
    FName DataTypeName;
    if (Params.DataType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Float"));
    }
    else if (Params.DataType.Equals(TEXT("Int32"), ESearchCase::IgnoreCase) || Params.DataType.Equals(TEXT("Int"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Int32"));
    }
    else if (Params.DataType.Equals(TEXT("Bool"), ESearchCase::IgnoreCase) || Params.DataType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Bool"));
    }
    else if (Params.DataType.Equals(TEXT("Trigger"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Trigger"));
    }
    else if (Params.DataType.Equals(TEXT("Audio"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Audio"));
    }
    else if (Params.DataType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("String"));
    }
    else
    {
        // Use the type name as-is for custom types
        DataTypeName = FName(*Params.DataType);
    }

    // Create the default value literal based on data type
    FMetasoundFrontendLiteral DefaultLiteral;
    if (!Params.DefaultValue.IsEmpty())
    {
        if (DataTypeName == FName(TEXT("Float")))
        {
            DefaultLiteral.Set(FCString::Atof(*Params.DefaultValue));
        }
        else if (DataTypeName == FName(TEXT("Int32")))
        {
            DefaultLiteral.Set(FCString::Atoi(*Params.DefaultValue));
        }
        else if (DataTypeName == FName(TEXT("Bool")))
        {
            DefaultLiteral.Set(Params.DefaultValue.ToBool());
        }
        else if (DataTypeName == FName(TEXT("String")))
        {
            DefaultLiteral.Set(Params.DefaultValue);
        }
        // Trigger and Audio types don't have default values
    }

    // Add the input - AddGraphInputNode returns an OUTPUT handle (the output of the input node)
    EMetaSoundBuilderResult Result;
    FMetaSoundBuilderNodeOutputHandle OutputHandle = Builder.AddGraphInputNode(
        FName(*Params.InputName),
        DataTypeName,
        DefaultLiteral,
        Result,
        false  // Not a constructor input
    );

    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Failed to add input '%s' of type '%s'"), *Params.InputName, *Params.DataType);
        return false;
    }

    OutInputNodeId = OutputHandle.NodeID.ToString();

    // CRITICAL: Create the template input node for visual representation
    // The editor visualizes template nodes (FInputNodeTemplate), not the interface input nodes directly.
    // Without this, the input won't appear in the graph until the asset is reopened.
    FMetaSoundFrontendDocumentBuilder& DocBuilder = Builder.GetBuilder();
    const FMetasoundFrontendNode* TemplateNode = Metasound::Frontend::FInputNodeTemplate::CreateNode(
        DocBuilder,
        FName(*Params.InputName)
    );

    if (TemplateNode)
    {
        // Set location on the TEMPLATE node (this is what the editor visualizes)
        FVector2D NodeLocation(-200.0f, 200.0f);  // Default position for inputs
        DocBuilder.SetNodeLocation(TemplateNode->GetID(), NodeLocation);

        UE_LOG(LogSoundService, Log, TEXT("Created template input node for '%s' with ID: %s"),
            *Params.InputName, *TemplateNode->GetID().ToString());
    }
    else
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to create template input node for '%s' - input may not appear visually"), *Params.InputName);
    }

    // Register the graph with the frontend to synchronize the editor graph
    UMetaSoundEditorSubsystem::GetChecked().RegisterGraphWithFrontend(*MetaSound, true);

    // Save the MetaSound
    MetaSound->Modify();
    if (!SaveAsset(MetaSound, OutError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save MetaSound after adding input: %s"), *OutError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Added input '%s' (type: %s, ID: %s) to MetaSound: %s"),
        *Params.InputName, *Params.DataType, *OutInputNodeId, *Params.MetaSoundPath);

    return true;
}

bool FSoundService::AddMetaSoundOutput(const FMetaSoundOutputParams& Params, FString& OutOutputNodeId, FString& OutError)
{
    UMetaSoundSource* MetaSound = FindMetaSoundSource(Params.MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *Params.MetaSoundPath);
        return false;
    }

    // Get or create a builder for this existing MetaSound using the document builder registry
#if WITH_EDITORONLY_DATA
    Metasound::Engine::FDocumentBuilderRegistry& BuilderRegistry = Metasound::Engine::FDocumentBuilderRegistry::GetChecked();
    UMetaSoundSourceBuilder& Builder = BuilderRegistry.FindOrBeginBuilding<UMetaSoundSourceBuilder>(*MetaSound);
#else
    OutError = TEXT("MetaSound editing requires editor data");
    return false;
#endif

    // Map data type string to MetaSound type name
    FName DataTypeName;
    if (Params.DataType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Float"));
    }
    else if (Params.DataType.Equals(TEXT("Int32"), ESearchCase::IgnoreCase) || Params.DataType.Equals(TEXT("Int"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Int32"));
    }
    else if (Params.DataType.Equals(TEXT("Bool"), ESearchCase::IgnoreCase) || Params.DataType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Bool"));
    }
    else if (Params.DataType.Equals(TEXT("Trigger"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Trigger"));
    }
    else if (Params.DataType.Equals(TEXT("Audio"), ESearchCase::IgnoreCase))
    {
        DataTypeName = FName(TEXT("Audio"));
    }
    else
    {
        // Use the type name as-is for custom types
        DataTypeName = FName(*Params.DataType);
    }

    // Add the output - AddGraphOutputNode returns an INPUT handle (the input of the output node)
    EMetaSoundBuilderResult Result;
    FMetaSoundBuilderNodeInputHandle InputHandle = Builder.AddGraphOutputNode(
        FName(*Params.OutputName),
        DataTypeName,
        FMetasoundFrontendLiteral(),  // Default value (empty for now)
        Result,
        false  // Not a constructor output
    );

    if (Result != EMetaSoundBuilderResult::Succeeded)
    {
        OutError = FString::Printf(TEXT("Failed to add output '%s' of type '%s'"), *Params.OutputName, *Params.DataType);
        return false;
    }

    OutOutputNodeId = InputHandle.NodeID.ToString();

    // Output nodes (class type: Output) are visualized directly, unlike inputs which use template nodes.
    // Set the location using the document builder for the output node.
    FMetaSoundFrontendDocumentBuilder& DocBuilder = Builder.GetBuilder();
    FVector2D NodeLocation(400.0f, 200.0f);  // Default position for outputs (right side)
    DocBuilder.SetNodeLocation(InputHandle.NodeID, NodeLocation);

    UE_LOG(LogSoundService, Log, TEXT("Set location for output node '%s' at (%f, %f)"),
        *Params.OutputName, NodeLocation.X, NodeLocation.Y);

    // Register the graph with the frontend to synchronize the editor graph
    UMetaSoundEditorSubsystem::GetChecked().RegisterGraphWithFrontend(*MetaSound, true);

    // Save the MetaSound
    MetaSound->Modify();
    if (!SaveAsset(MetaSound, OutError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save MetaSound after adding output: %s"), *OutError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Added output '%s' (type: %s, ID: %s) to MetaSound: %s"),
        *Params.OutputName, *Params.DataType, *OutOutputNodeId, *Params.MetaSoundPath);

    return true;
}

bool FSoundService::CompileMetaSound(const FString& MetaSoundPath, FString& OutError)
{
    UMetaSoundSource* MetaSound = FindMetaSoundSource(MetaSoundPath);
    if (!MetaSound)
    {
        OutError = FString::Printf(TEXT("MetaSound not found: %s"), *MetaSoundPath);
        return false;
    }

    // Get the asset base for registration
    FMetasoundAssetBase* AssetBase = Metasound::IMetasoundUObjectRegistry::Get().GetObjectAsAssetBase(MetaSound);
    if (!AssetBase)
    {
        OutError = TEXT("Failed to get MetaSound asset base");
        return false;
    }

    // Compile by updating and registering for execution
    Metasound::Frontend::FMetaSoundAssetRegistrationOptions RegOptions;
    RegOptions.bForceReregister = true;
    AssetBase->UpdateAndRegisterForExecution(RegOptions);

    // Save the MetaSound
    MetaSound->Modify();
    if (!SaveAsset(MetaSound, OutError))
    {
        UE_LOG(LogSoundService, Warning, TEXT("Failed to save MetaSound after compile: %s"), *OutError);
    }

    UE_LOG(LogSoundService, Log, TEXT("Compiled MetaSound: %s"), *MetaSoundPath);

    return true;
}

bool FSoundService::SearchMetaSoundPalette(const FString& SearchQuery, int32 MaxResults, TArray<TSharedPtr<FJsonObject>>& OutResults, FString& OutError)
{
#if WITH_EDITORONLY_DATA
    using namespace Metasound::Frontend;

    // Get all registered MetaSound node classes
    ISearchEngine& SearchEngine = ISearchEngine::Get();
    TArray<FMetasoundFrontendClass> AllClasses = SearchEngine.FindAllClasses(false); // false = don't include deprecated versions

    FString LowerQuery = SearchQuery.ToLower();
    int32 ResultCount = 0;

    for (const FMetasoundFrontendClass& NodeClass : AllClasses)
    {
        if (MaxResults > 0 && ResultCount >= MaxResults)
        {
            break;
        }

        const FMetasoundFrontendClassMetadata& Metadata = NodeClass.Metadata;
        const FMetasoundFrontendClassName& ClassName = Metadata.GetClassName();

        // Build searchable strings
        FString NameStr = ClassName.Name.ToString();
        FString NamespaceStr = ClassName.Namespace.ToString();
        FString VariantStr = ClassName.Variant.ToString();
        FString DisplayNameStr = Metadata.GetDisplayName().ToString();
        FString DescriptionStr = Metadata.GetDescription().ToString();

        // Build category string from hierarchy
        FString CategoryStr;
        const TArray<FText>& CategoryHierarchy = Metadata.GetCategoryHierarchy();
        for (int32 i = 0; i < CategoryHierarchy.Num(); ++i)
        {
            if (i > 0) CategoryStr += TEXT(" > ");
            CategoryStr += CategoryHierarchy[i].ToString();
        }

        // Build keywords string
        FString KeywordsStr;
        const TArray<FText>& Keywords = Metadata.GetKeywords();
        for (const FText& Keyword : Keywords)
        {
            KeywordsStr += TEXT(" ") + Keyword.ToString();
        }

        // Check if any field matches the search query
        bool bMatches = LowerQuery.IsEmpty() ||
            NameStr.ToLower().Contains(LowerQuery) ||
            NamespaceStr.ToLower().Contains(LowerQuery) ||
            VariantStr.ToLower().Contains(LowerQuery) ||
            DisplayNameStr.ToLower().Contains(LowerQuery) ||
            DescriptionStr.ToLower().Contains(LowerQuery) ||
            CategoryStr.ToLower().Contains(LowerQuery) ||
            KeywordsStr.ToLower().Contains(LowerQuery);

        if (bMatches)
        {
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetStringField(TEXT("namespace"), NamespaceStr);
            ResultObj->SetStringField(TEXT("name"), NameStr);
            ResultObj->SetStringField(TEXT("variant"), VariantStr);
            ResultObj->SetStringField(TEXT("display_name"), DisplayNameStr);
            ResultObj->SetStringField(TEXT("description"), DescriptionStr);
            ResultObj->SetStringField(TEXT("category"), CategoryStr);

            // Build full class name for use with add_metasound_node
            FString FullClassName = NamespaceStr + TEXT("::") + NameStr;
            if (!VariantStr.IsEmpty())
            {
                FullClassName += TEXT(" (") + VariantStr + TEXT(")");
            }
            ResultObj->SetStringField(TEXT("full_name"), FullClassName);

            // Include inputs/outputs info
            const FMetasoundFrontendClassInterface& Interface = NodeClass.GetDefaultInterface();
            TArray<TSharedPtr<FJsonValue>> InputsArray;
            for (const FMetasoundFrontendClassInput& Input : Interface.Inputs)
            {
                TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();
                InputObj->SetStringField(TEXT("name"), Input.Name.ToString());
                InputObj->SetStringField(TEXT("type"), Input.TypeName.ToString());
                InputsArray.Add(MakeShared<FJsonValueObject>(InputObj));
            }
            ResultObj->SetArrayField(TEXT("inputs"), InputsArray);

            TArray<TSharedPtr<FJsonValue>> OutputsArray;
            for (const FMetasoundFrontendClassOutput& Output : Interface.Outputs)
            {
                TSharedPtr<FJsonObject> OutputObj = MakeShared<FJsonObject>();
                OutputObj->SetStringField(TEXT("name"), Output.Name.ToString());
                OutputObj->SetStringField(TEXT("type"), Output.TypeName.ToString());
                OutputsArray.Add(MakeShared<FJsonValueObject>(OutputObj));
            }
            ResultObj->SetArrayField(TEXT("outputs"), OutputsArray);

            OutResults.Add(ResultObj);
            ResultCount++;
        }
    }

    UE_LOG(LogSoundService, Log, TEXT("MetaSound palette search for '%s' returned %d results"), *SearchQuery, ResultCount);
    return true;
#else
    OutError = TEXT("MetaSound palette search requires editor data");
    return false;
#endif
}

UMetaSoundSource* FSoundService::FindMetaSoundSource(const FString& MetaSoundPath)
{
    return Cast<UMetaSoundSource>(StaticLoadObject(UMetaSoundSource::StaticClass(), nullptr, *MetaSoundPath));
}
