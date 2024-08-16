using System;
using System.IO;

namespace UnrealBuildTool.Rules
{
    public class BlastEditor : ModuleRules
    {
        public BlastEditor(ReadOnlyTargetRules Target) : base(Target)
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
                    "Blast",
                    "UnrealEd"
                }
            );

            PrivateDependencyModuleNames.AddRange(
                new string[] {
                    "Core",
                    "CoreUObject",
                    "AssetTools",
                    "Engine",
                    "RenderCore",
                    "Renderer",
                    "PropertyEditor",
                    "XmlParser",
                    "Slate",
                    "ContentBrowser",
                    "Projects",
                    "SlateCore",
                    "MainFrame",
                    "InputCore",
                    "EditorStyle",
                    "LevelEditor",
                    "JsonUtilities",
                    "PhysicsUtilities",
                    "Json",
                    "RHI",
                    "SkeletalMeshUtilitiesCommon",
                }
            );

            //Don't add to PrivateDependencyModuleNames since we only use IBlastMeshEditorModule and we can't have a circular depdency on Linux
            PrivateIncludePathModuleNames.AddRange(
                new string[] {
                    "BlastMeshEditor",
                }
            );


            string[] BlastLibs =
            {
                 "NvBlastExtAssetUtils",
                 "NvBlastExtAuthoring",
            };

            Blast.SetupModuleBlastSupport(this, BlastLibs);

            AddEngineThirdPartyPrivateStaticDependencies(Target, "FBX");

        }
    }
}