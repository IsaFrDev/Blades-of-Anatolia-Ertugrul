// Copyright — ACRPG. Gorka Games "UE5 RPG Tutorial Series" (Blueprint) C++ portida.
// Unreal Engine 5.3 / 5.4 / 5.5 uchun mo'ljallangan.

using UnrealBuildTool;

public class ACRPG : ModuleRules
{
	public ACRPG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// IWYU (Include What You Use) — UE5 uslubi.
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",

			// Ep.78 — Enhanced Input (klaviatura + geympad bitta tizimda)
			"EnhancedInput",

			// Ep.20-29, 57-59 — AI: Behavior Tree, Blackboard, Perception
			"AIModule",
			"GameplayTasks",
			"NavigationSystem",

			// Ep.3, 4 — Motion Warping (vault va assassination'da nishonga "yopishish")
			"MotionWarping",

			// Ep.8, 15-19, 64, 73, 76 — UMG interfeys
			"UMG",
			"Slate",
			"SlateCore",

			// Ep.11, 77 — Niagara VFX (qon, level-up effekti)
			"Niagara",

			// Ep.25, 26, 40 — Control Rig / IK (Foot IK, Head Look At)
			"ControlRig",
			"AnimGraphRuntime",

			// Ep.55 — Physical Material orqali qadam tovushlari
			"PhysicsCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ApplicationCore",
			"RenderCore",
		});

		// Ep.71 — Intro sekvensiya (Level Sequence) dan o'ynatish
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
		}

		PublicIncludePaths.AddRange(new string[] { "ACRPG/Public" });
		PrivateIncludePaths.AddRange(new string[] { "ACRPG/Private" });
	}
}
