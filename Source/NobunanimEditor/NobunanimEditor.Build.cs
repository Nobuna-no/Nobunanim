// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NobunanimEditor : ModuleRules
{
	public NobunanimEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );

        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
            );


        PublicDependencyModuleNames.AddRange(new string[]
        {
              "AnimGraph",
        });

        PrivateDependencyModuleNames.AddRange(new string[] 
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "AnimGraph",
            "BlueprintGraph",
            "Persona",
            "UnrealEd",
            "AnimGraph",
            "AnimGraphRuntime",
            "SlateCore",
            "Nobunanim"
        });

   //     PrivateDependencyModuleNames.AddRange(
			//new string[]
   //         {
   //             "Projects",
   //             "InputCore",
   //             "UnrealEd",
   //             "LevelEditor",
   //             "CoreUObject",
   //             "Engine",
   //             "Slate",
   //             "SlateCore",
   //             "ImageWrapper",
   //             "AssetRegistry",
   //             "PropertyEditor",
   //             "AdvancedPreviewScene",

			//	// ... add private dependencies that you statically link with here ...	
			//}
			//);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
