// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UE_FightKatana : ModuleRules
{
	public UE_FightKatana(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
