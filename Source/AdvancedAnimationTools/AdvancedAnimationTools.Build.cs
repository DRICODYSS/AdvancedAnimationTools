// Copyright DRICODYSS. All Rights Reserved.

using UnrealBuildTool;

public class AdvancedAnimationTools : ModuleRules
{
	public AdvancedAnimationTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"AdvancedPreviewScene",
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Engine",
				"ToolWidgets",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"AssetRegistry",
				"AnimationBlueprintLibrary",
				"IKRig",
				"IKRigEditor",
				"LevelEditor",
				"ToolMenus",
				"PropertyEditor",
				"InteractiveToolsFramework",
				"PythonScriptPlugin",
				"Projects",
			}
		);
	}
}
