// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
using System.IO;

public class XimmerseInput : ModuleRules
{
	private string ModulePath
	{
		get { return ModuleDirectory; }
	}

	private string ThirdPartyPath
	{
		get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
	}

	private string BinariesPath
	{
		get { return Path.GetFullPath(Path.Combine(ModulePath, "../../Binaries/")); }
	}

	public XimmerseInput(TargetInfo Target)
	{
		PublicIncludePaths.AddRange(
		    new string[]
		{
			"XimmerseInput/Public",
		}
		);

		PrivateIncludePaths.AddRange(
		    new string[]
		{
			"XimmerseInput/Private",
		}
		);

		PrivateDependencyModuleNames.AddRange(
		    new string[]
		{
			"Core",
			"CoreUObject",
			"InputCore",
			"Engine",
			"InputDevice",
		}
		);

		bool isLibrarySupported = false;

		PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "Ximmerse/include"));

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			isLibrarySupported = true;
			PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Ximmerse/libs/x64", "xdevice.lib"));
			PublicDelayLoadDLLs.Add(Path.Combine(ThirdPartyPath, "Ximmerse/libs/x64", "xdevice.dll"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			isLibrarySupported = true;
			PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Ximmerse/libs/Android"));
		}

		Definitions.Add(string.Format("XIMMERSE_INPUT_SUPPORTED_PLATFORMS={0}", isLibrarySupported ? 1 : 0));
		//Add DLL for packaging
	}
}
}
