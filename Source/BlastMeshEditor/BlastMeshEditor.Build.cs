// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using System;
using System.IO;

namespace UnrealBuildTool.Rules
{
    public class BlastMeshEditor : ModuleRules
    {
        public BlastMeshEditor(ReadOnlyTargetRules Target) : base(Target)
        {
            PrivateIncludePaths.AddRange(
                new string[] {
                    Path.GetFullPath(Path.Combine(PluginDirectory, "Libraries", "include")),
                    Path.GetFullPath(Path.Combine(PluginDirectory, "Libraries", "include", "blast-sdk", "lowlevel")),
                    Path.GetFullPath(Path.Combine(PluginDirectory, "Libraries", "include", "blast-sdk", "globals")),
                    Path.GetFullPath(Path.Combine(PluginDirectory, "Libraries", "include", "blast-sdk", "extensions", "authoringCommon")),
                    Path.GetFullPath(Path.Combine(PluginDirectory, "Libraries", "include", "blast-sdk", "shared", "NvFoundation"))
                }
            );

            PublicDependencyModuleNames.AddRange(
                new string[] {
                "Engine"
                }
            );

            PrivateDependencyModuleNames.AddRange(
                new string[] {
                "Blast",
                "BlastEditor",
                "Core",
                "CoreUObject",
                "InputCore",
                "RenderCore",
                "PhysicsCore",
                "Slate",
                "SlateCore",
                "EditorStyle",
                "UnrealEd",
                "Projects",
                "DesktopPlatform",
                "AdvancedPreviewScene",
                "AssetTools",
                "LevelEditor",
                "MeshMergeUtilities",
                "RawMesh",
                "MeshUtilitiesCommon",
                "MeshDescription",
                "StaticMeshDescription",
                "MeshDescriptionOperations",
                "RHI",
                "SkeletalMeshUtilitiesCommon",
                "ImageCore",
                }
            );

            DynamicallyLoadedModuleNames.Add("PropertyEditor");

            string[] BlastLibs =
            {
                 "NvBlastExtAuthoring",
            };

            Blast.SetupModuleBlastSupport(this, BlastLibs);

            AddEngineThirdPartyPrivateStaticDependencies(Target, "FBX");
        }
    }
}