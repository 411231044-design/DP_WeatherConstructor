class CfgPatches
{
	class KR_SandstormPort
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Worlds_Chernarusplus_World"};
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
