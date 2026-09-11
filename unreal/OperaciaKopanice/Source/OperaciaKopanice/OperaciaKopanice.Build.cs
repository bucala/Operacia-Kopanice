using UnrealBuildTool;

public class OperaciaKopanice : ModuleRules
{
	public OperaciaKopanice(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"AIModule",
			"GameplayTasks",
			"NavigationSystem",
			"Landscape",
			"PhysicsCore",
			"PCG"
		});
	}
}
