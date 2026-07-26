using UnrealBuildTool;

public class Runik : ModuleRules
{
	public Runik(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"ApplicationCore",
			"RenderCore",
			"RHI",
			"ImageWrapper",
			"Json",
			"Projects",
			"AudioMixer"
		});
	}
}
