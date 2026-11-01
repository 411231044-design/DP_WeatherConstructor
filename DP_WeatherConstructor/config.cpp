class CfgPatches
{
	class DP_WeatherConstructor
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Scripts", "DZ_Worlds_Chernarusplus_World"};
	};
};

class CfgMods
{
	class DP_WeatherConstructor
	{
		dir = "DP_WeatherConstructor";
		picture = "";
		action = "";
		hideName = 1;
		hidePicture = 1;
		name = "DP Weather Constructor";
		credits = "KitTysh";
		author = "KitTysh";
		authorID = "0";
		version = "0.2.0";
		extra = 0;
		type = "servermod";

		dependencies[] = {"World", "Mission"};

		class defs
		{
			class worldScriptModule
			{
				value = "";
				files[] = {"DP_WeatherConstructor/scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"DP_WeatherConstructor/scripts/5_Mission"};
			};
		};
	};
};

// Маршрут песчаной бури (движковая секция, читается при загрузке мира).
// particlePath ОБЯЗАТЕЛЬНО существующая частица: дефолтный путь — файл DLC.
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
