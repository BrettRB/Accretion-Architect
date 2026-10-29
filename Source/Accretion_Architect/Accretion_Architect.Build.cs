// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Accretion_Architect : ModuleRules
{
	public Accretion_Architect(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "Slate", "SlateCore" });

		// Allow includes relative to the module root, e.g. "Components/AccretionMassComponent.h".
		PublicIncludePaths.Add(ModuleDirectory);

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
