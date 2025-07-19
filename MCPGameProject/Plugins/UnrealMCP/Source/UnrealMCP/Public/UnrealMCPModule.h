#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Main module class for the UnrealMCP plugin
 * 
 * The UnrealMCP plugin provides a comprehensive Model Context Protocol (MCP) integration
 * for Unreal Engine 5.6, enabling AI assistants to control Unreal Engine through
 * natural language commands via a layered architecture.
 * 
 * Key Features:
 * - Command-based architecture with individual command classes
 * - Service layer for business logic separation
 * - Factory pattern for type-safe object creation
 * - Comprehensive error handling and validation
 * - Performance optimizations with caching and object pooling
 * - Thread-safe operations where required
 * 
 * Architecture Layers:
 * 1. Command Layer: Individual commands implementing IUnrealMCPCommand
 * 2. Service Layer: Business logic services (Blueprint, Component, Property, etc.)
 * 3. Factory Layer: Type-safe object creation (Component, Widget factories)
 * 4. Utility Layer: Validation, error handling, logging, common utilities
 * 
 * The module handles initialization and shutdown of all plugin systems including:
 * - Command registry setup
 * - Service initialization
 * - Factory type registration
 * - TCP server startup for MCP communication
 * - Logging system configuration
 */
class UNREALMCP_API FUnrealMCPModule : public IModuleInterface
{
public:
	/**
	 * Called when the module is loaded during engine startup
	 * Initializes all plugin systems and starts the MCP server
	 */
	virtual void StartupModule() override;
	
	/**
	 * Called when the module is unloaded during engine shutdown
	 * Cleans up resources and stops the MCP server
	 */
	virtual void ShutdownModule() override;

	/**
	 * Get the singleton instance of the UnrealMCP module
	 * @return Reference to the loaded module instance
	 * @note This will load the module if it's not already loaded
	 */
	static inline FUnrealMCPModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FUnrealMCPModule>("UnrealMCP");
	}

	/**
	 * Check if the UnrealMCP module is currently loaded
	 * @return true if the module is loaded and available
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("UnrealMCP");
	}

private:
	/**
	 * Initialize the command registry with all available commands
	 * Registers commands from all categories (Blueprint, Editor, Project, etc.)
	 */
	void InitializeCommandRegistry();
	
	/**
	 * Initialize all service layer components
	 * Sets up service instances and their dependencies
	 */
	void InitializeServices();
	
	/**
	 * Initialize factory systems
	 * Registers default component and widget types
	 */
	void InitializeFactories();
	
	/**
	 * Start the MCP TCP server
	 * Begins listening for MCP client connections
	 */
	void StartMCPServer();
	
	/**
	 * Stop the MCP TCP server
	 * Closes all client connections and stops listening
	 */
	void StopMCPServer();
	
	/**
	 * Clean up all plugin resources
	 * Called during shutdown to ensure proper cleanup
	 */
	void CleanupResources();
}; 
