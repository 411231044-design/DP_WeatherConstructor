class CfgPatches
{
	class DP_SandstormPort
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Scripts", "DZ_Worlds_Chernarusplus_World"};
	};
};

class CfgMods
{
	class DP_SandstormPort
	{
		dir = "DP_SandstormPort";
		picture = "";
		action = "";
		hideName = 1;
		hidePicture = 1;
		name = "DP Sandstorm Port";
		credits = "KitTysh";
		author = "KitTysh";
		authorID = "0";
		version = "0.1.0";
		extra = 0;
		type = "servermod";

		dependencies[] = {"World", "Mission"};

		class defs
		{
			class worldScriptModule
			{
				value = "";
				files[] = {"DP_SandstormPort/scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"DP_SandstormPort/scripts/5_Mission"};
			};
		};
	};
};

class CfgWorlds
{
	class ChernarusPlus
	{
		class Weather
		{
			class Sandstorm
			{
				startPosition[] = {2000.0, 0.0, 2300.0};
				endPosition[] = {13000.0, 0.0, 2600.0};
				particlePath = "Graphics/Particles/dust_vehicle";
				sandstormIntensity = 1.0;
			};
		};
	};
};
