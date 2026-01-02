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
				"RHI"                      // For reading pixels from GPU
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
					"EditorFramework"      // For FToolkitManager (finding open Material Editors)
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