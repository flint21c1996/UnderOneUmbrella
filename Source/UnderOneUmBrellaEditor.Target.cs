// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnderOneUmBrellaEditorTarget : TargetRules
{
	public UnderOneUmBrellaEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("UnderOneUmBrella");
		ExtraModuleNames.Add("UnderOneUmBrellaEditor");
		ExtraModuleNames.Add("UnderOneUmBrellaDevTools");
	}
}
