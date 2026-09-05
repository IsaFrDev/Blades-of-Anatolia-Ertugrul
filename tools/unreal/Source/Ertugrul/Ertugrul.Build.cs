// Blades of Anatolia: Ertugrul — asosiy o'yin moduli
using UnrealBuildTool;

public class Ertugrul : ModuleRules
{
	public Ertugrul(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore",
			"EnhancedInput", "AIModule", "NavigationSystem",
			"GameplayTasks", "UMG", "Slate", "SlateCore",
			"Json", "JsonUtilities", "ProceduralMeshComponent", "AssetRegistry", "ImageWrapper"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
