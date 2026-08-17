// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnderOneUmBrellaDevTools : ModuleRules
{
	public UnderOneUmBrellaDevTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnderOneUmBrella"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AIModule",
			"InputCore",
			"NavigationSystem",
			"Niagara",
			"RenderCore",
			"RHI",
			"Slate",
			"SlateCore"
		});
	}
}
