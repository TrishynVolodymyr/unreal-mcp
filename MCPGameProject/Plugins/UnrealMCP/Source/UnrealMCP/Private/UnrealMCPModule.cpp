#include "UnrealMCPModule.h"
#include "UnrealMCPBridge.h"
#include "Factories/ComponentFactory.h"
#include "Factories/WidgetFactory.h"
#include "Services/ObjectPoolManager.h"
#include "Commands/UnrealMCPMainDispatcher.h"
#include "MCPLogging.h"
#include "Modules/ModuleManager.h"
#include "EditorSubsystem.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "FUnrealMCPModule"

void FUnrealMCPModule::StartupModule()
{
	// Initialize the MCP logging system first
	FMCPLogger::Initialize(true); // Enable file logging
	FMCPLogger::SetDebugLoggingEnabled(true); // Enable debug logging for development
	
	UE_LOG_MCP_INFO("Unreal MCP Module startup initiated");
	
	// Initialize the ObjectPoolManager for performance optimization
	FObjectPoolManager& PoolManager = FObjectPoolManager::Get();
	PoolManager.Initialize();
	
	UE_LOG_MCP_INFO("ObjectPoolManager initialized with object pools");
	
	// Initialize the ComponentFactory with default types
	FComponentFactory& ComponentFactory = FComponentFactory::Get();
	ComponentFactory.InitializeDefaultTypes();
	
	UE_LOG_MCP_INFO("ComponentFactory initialized with default component types");
	
	// Initialize the WidgetFactory with default types
	FWidgetFactory& WidgetFactory = FWidgetFactory::Get();
	WidgetFactory.InitializeDefaultWidgetTypes();
	
	UE_LOG_MCP_INFO("WidgetFactory initialized with default widget types");
	
	// Initialize the command dispatcher and registry
	FUnrealMCPMainDispatcher& Dispatcher = FUnrealMCPMainDispatcher::Get();
	Dispatcher.Initialize();
	
	UE_LOG_MCP_INFO("Command dispatcher initialized with registered commands");
	UE_LOG_MCP_INFO("Unreal MCP Module startup completed successfully");
}

void FUnrealMCPModule::ShutdownModule()
{
	UE_LOG_MCP_INFO("Unreal MCP Module shutting down");
	
	// Shutdown the command dispatcher and registry
	FUnrealMCPMainDispatcher& Dispatcher = FUnrealMCPMainDispatcher::Get();
	Dispatcher.Shutdown();
	
	UE_LOG_MCP_INFO("Command dispatcher shut down and commands unregistered");
	
	// Shutdown the ObjectPoolManager
	FObjectPoolManager& PoolManager = FObjectPoolManager::Get();
	PoolManager.Shutdown();
	
	UE_LOG_MCP_INFO("ObjectPoolManager shut down and pools cleaned up");
	
	UE_LOG_MCP_INFO("Unreal MCP Module has shut down");
	
	// Shutdown the MCP logging system last
	FMCPLogger::Shutdown();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealMCPModule, UnrealMCP) 