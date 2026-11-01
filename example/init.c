// DP_SandstormPort — drop-in сервермод: миссии ничего не нужно.
// Маршрут в CfgWorlds >> Weather >> Sandstorm, скрипты регистрирует CfgMods >> defs.
void main()
{
	Hive ce = CreateHive();
	if (ce)
	{
		ce.InitOffline();
	}
}

class CustomMission: MissionServer
{
}

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}
