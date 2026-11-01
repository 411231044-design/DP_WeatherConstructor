// Sandstorm port probe for Chernarus, looping edition.
// Starts a sandstorm sweep along the south coast, restarts when it fades out.
class SandstormProbe
{
	static bool s_booted = false;
	static int  s_cooldown = 0;
	static int  s_cycle = 0;
	static int  s_print = 0;

	static int AsInt(bool b)
	{
		if (b)
		{
			return 1;
		}
		return 0;
	}

	static void StartStorm()
	{
		Weather weather = g_Game.GetWeather();
		Sandstorm storm = weather.GetSandstorm();
		bool ok = storm.Start(30.0, false);
		Print("[SANDSTORM] cycle " + s_cycle + " Start ok=" + AsInt(ok));
		if (ok)
		{
			float angle = weather.WindDirectionToAngle(storm.GetDirection());
			weather.GetWindDirection().Set(angle, 30, 99999);
			weather.GetOvercast().Set(0.9, 30, 99999);
			weather.GetWindMagnitude().Set(20.0, 30, 99999);
			weather.GetRain().Set(0, 30, 99999);
			Print("[SANDSTORM] cycle " + s_cycle + " begin pos=" + storm.GetPosition() + " dir=" + storm.GetDirection());
		}
	}

	static void Tick()
	{
		Weather weather = g_Game.GetWeather();
		Sandstorm storm = weather.GetSandstorm();

		if (!s_booted)
		{
			s_booted = true;
			s_cycle = 1;
			StartStorm();
			return;
		}

		if (storm.IsActive())
		{
			s_cooldown = 120;
		}
		else
		{
			s_cooldown = s_cooldown - 1;
			if (s_cooldown <= 0)
			{
				s_cycle = s_cycle + 1;
				StartStorm();
				s_cooldown = 120;
			}
		}

		s_print = s_print + 1;
		if (s_print >= 20)
		{
			s_print = 0;
			Print("[SANDSTORM] c" + s_cycle + " active=" + AsInt(storm.IsActive()) + " pos=" + storm.GetPosition() + " speed=" + storm.GetSpeed() + " mag=" + storm.GetMagnitude());
		}
	}
}

void main()
{
	Hive ce = CreateHive();
	if (ce)
	{
		ce.InitOffline();
	}
	GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SandstormProbe.Tick, 500, true);
}

class CustomMission: MissionServer
{
}

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}
