// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnderOneUmBrella : ModuleRules
{
	public UnderOneUmBrella(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] { ModuleDirectory });

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
