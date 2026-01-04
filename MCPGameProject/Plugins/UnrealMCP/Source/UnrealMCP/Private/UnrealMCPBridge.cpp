#include "UnrealMCPBridge.h"
#include "MCPServerRunnable.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "HAL/RunnableThread.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "EditorScriptingUtilities/Public/EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "JsonObjectConverter.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
// Add Blueprint related includes
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
// UE5.5 correct includes
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/Field.h"
#include "UObject/FieldPath.h"
// Blueprint Graph specific includes
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "GameFramework/InputSettings.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
// Include our new command handler classes
#include "Commands/Editor/UnrealMCPEditorCommands.h"
#include "Commands/Blueprint/UnrealMCPBlueprintCommands.h"
#include "Commands/BlueprintNode/UnrealMCPBlueprintNodeCommands.h"
#include "Commands/Project/UnrealMCPProjectCommands.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Commands/UMG/UnrealMCPUMGCommands.h"

#include "Commands/BlueprintAction/UnrealMCPBlueprintActionCommandsHandler.h"
#include "Commands/EditorCommandRegistration.h"
#include "Commands/DataTableCommandRegistration.h"

// Default settings
#define MCP_SERVER_HOST "127.0.0.1"
#define MCP_SERVER_PORT 55557

UUnrealMCPBridge::UUnrealMCPBridge()
{
    EditorCommands = MakeShared<FUnrealMCPEditorCommands>();
    BlueprintCommands = MakeShared<FUnrealMCPBlueprintCommands>();
    BlueprintNodeCommands = MakeShared<FUnrealMCPBlueprintNodeCommands>();
    ProjectCommands = MakeShared<FUnrealMCPProjectCommands>();
    UMGCommands = MakeShared<FUnrealMCPUMGCommands>();
    BlueprintActionCommands = MakeShared<FUnrealMCPBlueprintActionCommandsHandler>();
}

UUnrealMCPBridge::~UUnrealMCPBridge()
{
    EditorCommands.Reset();
    BlueprintCommands.Reset();
    BlueprintNodeCommands.Reset();
    ProjectCommands.Reset();
    UMGCommands.Reset();
    BlueprintActionCommands.Reset();
}

// Initialize subsystem
void UUnrealMCPBridge::Initialize(FSubsystemCollectionBase& Collection)
{
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Initializing"));
    
    bIsRunning = false;
    ListenerSocket = nullptr;
    ConnectionSocket = nullptr;
    ServerThread = nullptr;
    Port = MCP_SERVER_PORT;
    FIPv4Address::Parse(MCP_SERVER_HOST, ServerAddress);

    // Register new command system
    FEditorCommandRegistration::RegisterAllCommands();
    FDataTableCommandRegistration::RegisterAllCommands();
    
    // Start the server automatically
    StartServer();
}

// Clean up resources when subsystem is destroyed
void UUnrealMCPBridge::Deinitialize()
{
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Shutting down"));
    
    // Unregister editor commands
    FEditorCommandRegistration::UnregisterAllCommands();
    FDataTableCommandRegistration::UnregisterAllCommands();
    
    StopServer();
}

// Start the MCP server
void UUnrealMCPBridge::StartServer()
{
    if (bIsRunning)
    {
        UE_LOG(LogTemp, Warning, TEXT("UnrealMCPBridge: Server is already running"));
        return;
    }

    // Create socket subsystem
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to get socket subsystem"));
        return;
    }

    // Create listener socket
    TSharedPtr<FSocket> NewListenerSocket = MakeShareable(SocketSubsystem->CreateSocket(NAME_Stream, TEXT("UnrealMCPListener"), false));
    if (!NewListenerSocket.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to create listener socket"));
        return;
    }

    // Allow address reuse for quick restarts
    NewListenerSocket->SetReuseAddr(true);
    NewListenerSocket->SetNonBlocking(true);

    // Bind to address
    FIPv4Endpoint Endpoint(ServerAddress, Port);
    if (!NewListenerSocket->Bind(*Endpoint.ToInternetAddr()))
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to bind listener socket to %s:%d"), *ServerAddress.ToString(), Port);
        return;
    }

    // Start listening
    if (!NewListenerSocket->Listen(5))
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to start listening"));
        return;
    }

    ListenerSocket = NewListenerSocket;
    bIsRunning = true;
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Server started on %s:%d"), *ServerAddress.ToString(), Port);

    // Start server thread
    ServerThread = FRunnableThread::Create(
        new FMCPServerRunnable(this, ListenerSocket),
        TEXT("UnrealMCPServerThread"),
        0, TPri_Normal
    );

    if (!ServerThread)
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to create server thread"));
        StopServer();
        return;
    }
}

// Stop the MCP server
void UUnrealMCPBridge::StopServer()
{
    if (!bIsRunning)
    {
        return;
    }

    bIsRunning = false;

    // Clean up thread
    if (ServerThread)
    {
        ServerThread->Kill(true);
        delete ServerThread;
        ServerThread = nullptr;
    }

    // Close sockets
    if (ConnectionSocket.IsValid())
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket.Get());
        ConnectionSocket.Reset();
    }

    if (ListenerSocket.IsValid())
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket.Get());
        ListenerSocket.Reset();
    }

    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Server stopped"));
}

// Execute a command received from a client
FString UUnrealMCPBridge::ExecuteCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Executing command: %s"), *CommandType);
    
    // Create a promise to wait for the result
    TPromise<FString> Promise;
    TFuture<FString> Future = Promise.GetFuture();
    
    // Queue execution on Game Thread
    AsyncTask(ENamedThreads::GameThread, [this, CommandType, Params, Promise = MoveTemp(Promise)]() mutable
    {
        TSharedPtr<FJsonObject> ResponseJson = MakeShareable(new FJsonObject);
        
        try
        {
            TSharedPtr<FJsonObject> ResultJson;
            
            if (CommandType == TEXT("ping"))
            {
                ResultJson = MakeShareable(new FJsonObject);
                ResultJson->SetStringField(TEXT("message"), TEXT("pong"));
            }
            else
            {
                // Define command arrays for better maintenance
                static const TArray<FString> EditorCommands = {
                    TEXT("spawn_actor"),
                    TEXT("create_actor"),
                    TEXT("delete_actor"),
                    TEXT("set_actor_transform"),
                    TEXT("get_actor_properties"),
                    TEXT("set_actor_property"),
                    TEXT("set_light_property"),
                    TEXT("spawn_blueprint_actor"),
                    TEXT("focus_viewport"),
                    TEXT("take_screenshot"),
                    TEXT("get_level_metadata")
                };
                
                static const TArray<FString> BlueprintCommandsList = {
                    TEXT("create_blueprint"), 
                    TEXT("add_component_to_blueprint"), 
                    TEXT("modify_blueprint_component_properties"), 
                    TEXT("set_physics_properties"), 
                    TEXT("compile_blueprint"), 
                    TEXT("set_blueprint_property"), 
                    TEXT("set_static_mesh_properties"),
                    TEXT("set_pawn_properties"),
                    TEXT("call_function_by_name"),
                    TEXT("add_interface_to_blueprint"),
                    TEXT("create_blueprint_interface"),
                    TEXT("create_custom_blueprint_function")
                };
                
                static const TArray<FString> BlueprintNodeCommandsList = {
                    TEXT("connect_blueprint_nodes"), 
                    TEXT("find_blueprint_nodes"),
                    TEXT("add_blueprint_event_node"),
                    // TEXT("add_blueprint_input_action_node"),  // REMOVED: Use create_node_by_action_name instead
                    // TEXT("add_enhanced_input_action_node"),  // REMOVED: Use create_node_by_action_name instead
                    TEXT("add_blueprint_function_node"),
                    TEXT("add_blueprint_get_component_node"),
                    TEXT("add_blueprint_variable"),
                    TEXT("add_blueprint_custom_event_node"),
                    TEXT("get_variable_info"),
                    TEXT("create_node_by_action_name")
                };
                
                static const TArray<FString> ProjectCommandsList = {
                    TEXT("create_input_mapping"),
                    TEXT("create_enhanced_input_action"),
                    TEXT("create_input_mapping_context"),
                    TEXT("add_mapping_to_context"),
                    TEXT("create_folder"),
                    TEXT("create_struct"),
                    TEXT("update_struct"),
                    TEXT("get_project_dir"),
                    TEXT("get_project_metadata")
                };
                
                static const TArray<FString> UMGCommandsList = {
                    TEXT("create_umg_widget_blueprint"),
                    TEXT("bind_widget_component_event"),
                    TEXT("set_text_block_widget_component_binding"),
                    TEXT("add_child_widget_component_to_parent"),
                    TEXT("set_widget_component_placement"),
                    TEXT("add_widget_component_to_widget"),
                    TEXT("set_widget_component_property"),
                    TEXT("get_widget_blueprint_metadata"),
                    TEXT("capture_widget_screenshot")
                };
                
                static const TArray<FString> BlueprintActionCommandsList = {
                    TEXT("get_actions_for_pin"),
                    TEXT("get_actions_for_class"),
                    TEXT("get_node_pin_info"),
                    TEXT("create_node_by_action_name")
                };
                
                // First check the new command registry system
                FUnrealMCPCommandRegistry& CommandRegistry = FUnrealMCPCommandRegistry::Get();
                if (CommandRegistry.IsCommandRegistered(CommandType))
                {
                    // Convert TSharedPtr<FJsonObject> to JSON string for new command interface
                    FString ParamsString;
                    TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ParamsString);
                    FJsonSerializer::Serialize(Params.ToSharedRef(), Writer.Get());
                    
                    // Execute command through new registry
                    FString CommandResult = CommandRegistry.ExecuteCommand(CommandType, ParamsString);
                    
                    // Parse result back to JSON object
                    TSharedPtr<FJsonObject> ParsedResult;
                    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CommandResult);
                    if (FJsonSerializer::Deserialize(Reader, ParsedResult) && ParsedResult.IsValid())
                    {
                        ResultJson = ParsedResult;
                    }
                    else
                    {
                        // If parsing fails, create error response
                        ResultJson = MakeShared<FJsonObject>();
                        ResultJson->SetBoolField(TEXT("success"), false);
                        ResultJson->SetStringField(TEXT("error"), TEXT("Failed to parse command result"));
                    }
                }
                // Fall back to legacy command handlers
                else if (EditorCommands.Contains(CommandType))
                {
                    ResultJson = this->EditorCommands->HandleCommand(CommandType, Params);
                }
                else if (BlueprintCommandsList.Contains(CommandType))
                {
                    ResultJson = BlueprintCommands->HandleCommand(CommandType, Params);
                }
                else if (BlueprintNodeCommandsList.Contains(CommandType))
                {
                    ResultJson = BlueprintNodeCommands->HandleCommand(CommandType, Params);
                }
                else if (ProjectCommandsList.Contains(CommandType))
                {
                    ResultJson = ProjectCommands->HandleCommand(CommandType, Params);
                }
                else if (UMGCommandsList.Contains(CommandType))
                {
                    ResultJson = UMGCommands->HandleCommand(CommandType, Params);
                }
                else if (BlueprintActionCommandsList.Contains(CommandType))
                {
                    ResultJson = BlueprintActionCommands->HandleCommand(CommandType, Params);
                }
                else
                {
                    ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                    ResponseJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Unknown command: %s"), *CommandType));
                    
                    FString ResultString;
                    TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ResultString);
                    FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer.Get());
                    Promise.SetValue(ResultString);
                    return;
                }
            }
            
            // Check if the result contains an error
            bool bSuccess = true;
            FString ErrorMessage;
            
            if (ResultJson->HasField(TEXT("success")))
            {
                bSuccess = ResultJson->GetBoolField(TEXT("success"));
                if (!bSuccess && ResultJson->HasField(TEXT("error")))
                {
                    ErrorMessage = ResultJson->GetStringField(TEXT("error"));
                }
            }
            
            if (bSuccess)
            {
                // Set success status and include the result
                ResponseJson->SetStringField(TEXT("status"), TEXT("success"));
                ResponseJson->SetObjectField(TEXT("result"), ResultJson);
            }
            else
            {
                // Set error status and preserve ALL fields from the command result
                ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                ResponseJson->SetStringField(TEXT("error"), ErrorMessage);
                
                // Copy all additional fields from ResultJson to ResponseJson (e.g., compilation_errors)
                for (const auto& Pair : ResultJson->Values)
                {
                    const FString& Key = Pair.Key;
                    // Skip 'success' and 'error' as we already handled them
                    if (Key != TEXT("success") && Key != TEXT("error"))
                    {
                        ResponseJson->SetField(Key, Pair.Value);
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
            ResponseJson->SetStringField(TEXT("error"), UTF8_TO_TCHAR(e.what()));
        }
        
        FString ResultString;
        TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ResultString);
        FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer.Get());
        Promise.SetValue(ResultString);
    });
    
    return Future.Get();
}