// Copyright Epic Games, Inc. All Rights Reserved.

// This is now an Editor-only module. It will not be included in packaged builds.

using UnrealBuildTool;

public class UnrealMCP : ModuleRules
{
	public UnrealMCP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		// Use IWYUSupport instead of the deprecated bEnforceIWYU in UE5.5
		IWYUSupport = IWYUSupport.Full;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				"Runtime/AdvancedWidgets/Public"
			}
		);

		// Add NiagaraEditor private includes for static switch node access
		string NiagaraEditorPrivatePath = System.IO.Path.GetFullPath(System.IO.Path.Combine(EngineDirectory, "Plugins", "FX", "Niagara", "Source", "NiagaraEditor", "Private"));
		if (System.IO.Directory.Exists(NiagaraEditorPrivatePath))
		{
			PrivateIncludePaths.Add(NiagaraEditorPrivatePath);
		}

		// Add Niagara Internal includes for stateless module access (FNiagaraDistributionFloat, UNiagaraStatelessModule, etc.)
		string NiagaraInternalPath = System.IO.Path.GetFullPath(System.IO.Path.Combine(EngineDirectory, "Plugins", "FX", "Niagara", "Source", "Niagara", "Internal"));
		if (System.IO.Directory.Exists(NiagaraInternalPath))
		{
			PrivateIncludePaths.Add(NiagaraInternalPath);
		}

		// Add NiagaraShader Internal includes for stateless simulation shader (required by NiagaraStatelessEmitter.h)
		string NiagaraShaderInternalPath = System.IO.Path.GetFullPath(System.IO.Path.Combine(EngineDirectory, "Plugins", "FX", "Niagara", "Source", "NiagaraShader", "Internal"));
		if (System.IO.Directory.Exists(NiagaraShaderInternalPath))
		{
			PrivateIncludePaths.Add(NiagaraShaderInternalPath);
		}

		// Add MetasoundEditor private includes for FGraphBuilder access
		string MetasoundEditorPrivatePath = System.IO.Path.GetFullPath(System.IO.Path.Combine(EngineDirectory, "Plugins", "Runtime", "Metasound", "Source", "MetasoundEditor", "Private"));
		if (System.IO.Directory.Exists(MetasoundEditorPrivatePath))
		{
			PrivateIncludePaths.Add(MetasoundEditorPrivatePath);
		}

		// PCGEditor private headers not included — ReconstructGraph() is not exported.
		// Instead using close+reopen approach via UAssetEditorSubsystem.
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"EnhancedInput",
				"Networking",
				"Sockets",
				"HTTP",
				"Json",
				"JsonUtilities",
				"DeveloperSettings",
				"EditorScriptingUtilities",
				"AssetTools"
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"EditorScriptingUtilities",
				"EditorSubsystem",
				"Slate",
				"SlateCore",
				"UMG",
				"AdvancedWidgets",
				"Kismet",
				"KismetCompiler",
				"BlueprintGraph",
				"Projects",
				"AssetRegistry",
				"UMGEditor",
				"InputCore",
				"EnhancedInput",
				"InputBlueprintNodes",
				"ToolMenus",
				"CoreUObject",
				"EditorStyle",
				"AssetTools",
				"StructUtils",
				"PropertyEditor",
				"BlueprintEditorLibrary",
				"SubobjectDataInterface",  // For UE 5.7 USubobjectDataSubsystem API
				"ImageWrapper",            // For PNG/JPEG compression
				"RenderCore",              // For render targets
				"RHI",                     // For reading pixels from GPU
				"NavigationSystem",        // For ANavMeshBoundsVolume
				"AnimGraph",               // For Animation Blueprint nodes
				"AnimationBlueprintLibrary", // For Animation Blueprint utilities
				"GameplayTags"             // For FGameplayTag in animation variables
			}
		);
		
		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
					"PropertyEditor",      // For widget property editing
					"ToolMenus",           // For editor UI
					"BlueprintEditorLibrary", // For Blueprint utilities
					"MaterialEditor",      // For Material Editor refresh notifications
					"EditorFramework",     // For FToolkitManager (finding open Material Editors)
					// Niagara VFX support
					"Niagara",             // Core Niagara runtime
					"NiagaraCore",         // Niagara core types
					"NiagaraEditor",       // Niagara editor utilities (factories, graph utilities)
					"NiagaraShader",       // Niagara shader compilation
					// MetaSound audio support
					"MetasoundEngine",     // UMetaSoundSource, builders
					"MetasoundFrontend",   // FMetaSoundFrontendDocumentBuilder
					"MetasoundGraphCore",  // Core graph types
					"MetasoundEditor",      // Editor-specific utilities
					// StateTree AI support
					"StateTreeModule",         // Core StateTree runtime
					"StateTreeEditorModule",   // StateTree editor utilities (factories, compilation)
					"PropertyBindingUtils",    // Property binding path utilities
					// PCG (Procedural Content Generation) support
					"PCG"                      // Core PCG runtime (graphs, nodes, elements)
				}
			);
		}
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
} 