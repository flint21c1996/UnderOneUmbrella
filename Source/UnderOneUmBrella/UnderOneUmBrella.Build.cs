// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnderOneUmBrella : ModuleRules
{
	public UnderOneUmBrella(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] { ModuleDirectory });

		bool bWithUOUPuzzleCheats =
			Target.Configuration != UnrealTargetConfiguration.Shipping ||
			Target.Name.Equals("UnderOneUmBrellaInternal", System.StringComparison.OrdinalIgnoreCase);
		PublicDefinitions.Add($"UOU_WITH_PUZZLE_CHEATS={(bWithUOUPuzzleCheats ? 1 : 0)}");

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",
			"Niagara",
			"UMG",
			"LevelSequence",
			"MovieScene"
        });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"RenderCore",
			"RHI",
			"Slate",
			"SlateCore"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd"
			});
		}
	}
}
