// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MCPGameProjectEditorTarget : TargetRules
{
	public MCPGameProjectEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		bOverrideBuildEnvironment = true;
		ExtraModuleNames.Add("MCPGameProject");
	}
}
