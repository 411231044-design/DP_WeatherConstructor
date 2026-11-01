// DP_SandstormPort: игровой слой песчаной бури (серверный скрипт мода).
//
// Конфиг: DarkProjectConfig/SandstormPort/config.json в профиле сервера, создаётся
// с дефолтами при первом старте (паттерн DP_AdminMenu). Маршрут фронта задаётся в
// config.cpp аддона (CfgWorlds >> Weather >> Sandstorm) — его читает сам движок.

class DP_SandstormAutoStartCfg
{
	bool Enabled = false;            // автостарт по дефолту ВЫКЛЮЧЕН
	float GatherTimeSeconds = 30;    // разгон магнитуды, с
	int CooldownSeconds = 60;        // пауза между проходами, с
}

class DP_SandstormRandomStartCfg
{
	bool Enabled = true;             // случайный старт при условиях (как на Nasdara)
	float ChancePercent = 8;         // шанс на проверке (у Nasdara — 8%)
	float MinOvercast = 0.75;        // буря возможна только при пасмурной погоде
	int CheckIntervalMinutes = 10;   // как часто бросок
}

class DP_SandstormDamageCfg
{
	bool Enabled = true;             // прямой урон здоровью/шоку в буре
	float HealthPerSecond = 0.6;     // хп/с при интенсивности 1 без защиты
	float ShockPerSecond = 0.25;     // шок/с
	float MinIntensity = 0.2;        // ниже — урона нет
}

class DP_SandstormLoggingCfg
{
	bool LogHits = true;             // строки урона в script-лог
	bool Heartbeat = true;           // статус раз в минуту и первые три тика
}

class DP_SandstormConfig
{
	bool Enabled = true;             // ГЛАВНЫЙ ВЫКЛЮЧАТЕЛЬ: вся механика мода
	ref DP_SandstormAutoStartCfg AutoStart = new DP_SandstormAutoStartCfg();
	ref DP_SandstormRandomStartCfg RandomStart = new DP_SandstormRandomStartCfg();
	ref DP_SandstormDamageCfg Damage = new DP_SandstormDamageCfg();
	ref DP_SandstormLoggingCfg Logging = new DP_SandstormLoggingCfg();
}

class DPSandstormEffects
{
	static const string CONFIG_ROOT_DIR = "$profile:DarkProjectConfig";
	static const string CONFIG_DIR = "$profile:DarkProjectConfig/SandstormPort";
	static const string CONFIG_FILE = "$profile:DarkProjectConfig/SandstormPort/config.json";

	static const float TICK_SEC = 3.0;
	static const int TICK_MS = 3000;
	static const int CLEANUP_EVERY = 20;

	static ref DP_SandstormConfig s_config = null;
	static int s_cooldownTicks = 0;
	static int s_cycle = 0;
	static int s_tickCount = 0;

	static DP_SandstormConfig Config()
	{
		if (s_config)
		{
			return s_config;
		}

		MakeDirectory(CONFIG_ROOT_DIR);
		MakeDirectory(CONFIG_DIR);
		s_config = new DP_SandstormConfig();

		if (!FileExist(CONFIG_FILE))
		{
			JsonFileLoader<DP_SandstormConfig>.JsonSaveFile(CONFIG_FILE, s_config);
			Print("[DP_SANDSTORM] создан " + CONFIG_FILE + " — правь и перезапусти сервер");
			return s_config;
		}

		JsonFileLoader<DP_SandstormConfig>.JsonLoadFile(CONFIG_FILE, s_config);
		Print("[DP_SANDSTORM] config загружен: механика=" + AsInt(s_config.Enabled) + " автоStart=" + AsInt(s_config.AutoStart.Enabled) + " случайныйStart=" + AsInt(s_config.RandomStart.Enabled) + " урон=" + AsInt(s_config.Damage.Enabled));
		return s_config;
	}

	static int AsInt(bool b)
	{
		if (b)
		{
			return 1;
		}
		return 0;
	}

	static void Tick()
	{
		s_tickCount++;
		DP_SandstormConfig cfg = Config();

		// ГЛАВНЫЙ ВЫКЛЮЧАТЕЛЬ: без него мод полностью молчит
		if (!cfg.Enabled)
		{
			return;
		}

		Weather weather = g_Game.GetWeather();
		Sandstorm storm = null;
		if (weather)
		{
			storm = weather.GetSandstorm();
		}

		if (cfg.Logging.Heartbeat && (s_tickCount <= 3 || s_tickCount % 20 == 0))
		{
			int haveStorm = 0;
			int active = 0;
			int scripted = 0;
			string pos = "none";
			if (storm)
			{
				haveStorm = 1;
				active = AsInt(storm.IsActive());
				pos = storm.GetPosition().ToString();
				ScriptedSandstormController hbssc = ScriptedSandstormController.Cast(storm);
				scripted = AsInt(hbssc != null);
			}
			Print("[DP_SANDSTORM] hb tick=" + s_tickCount + " weather=" + AsInt(weather != null) + " storm=" + haveStorm + " scripted=" + scripted + " active=" + active + " pos=" + pos);
		}

		if (!weather || !storm)
		{
			return;
		}
		ScriptedSandstormController ssc = ScriptedSandstormController.Cast(storm);

		// чистка данных отключившихся: серверная ветка OnUpdate её не делает
		if (ssc && s_tickCount % CLEANUP_EVERY == 0)
		{
			ssc.CheckConnectedPlayers();
		}

		AutoStartCycle(cfg, weather, storm);
		RandomStartCheck(cfg, weather, storm);

		if (!storm.IsActive() || !cfg.Damage.Enabled)
		{
			return;
		}

		array<Man> players = new array<Man>();
		g_Game.GetPlayers(players);
		for (int i = 0; i < players.Count(); i++)
		{
			PlayerBase pb = PlayerBase.Cast(players.Get(i));
			if (!pb || !pb.IsAlive() || pb.IsUnconscious())
			{
				continue;
			}

			float intensity = storm.GetIntensity(pb.GetPosition());
			if (intensity <= 0.0)
			{
				continue;
			}
			// укрытия: ванильные фейды (крыша/машина/подземка/глубина)
			if (ssc)
			{
				intensity = ssc.GetIntensityForPlayer(pb);
			}
			if (intensity <= cfg.Damage.MinIntensity)
			{
				continue;
			}
			// защита лица: очки/маска от пыли (как в ванильном модификаторе)
			float protection = GetFaceProtection(pb);
			intensity = intensity - protection;
			if (intensity <= 0.0)
			{
				continue;
			}

			float dmg = cfg.Damage.HealthPerSecond * intensity * TICK_SEC;
			float shock = cfg.Damage.ShockPerSecond * intensity * TICK_SEC;
			pb.DecreaseHealth("", "Health", dmg);
			pb.DecreaseHealth("", "Shock", shock);

			if (cfg.Logging.LogHits)
			{
				string pname = "unknown";
				if (pb.GetIdentity())
				{
					pname = pb.GetIdentity().GetName();
				}
				Print("[DP_SANDSTORM] cycle " + s_cycle + " hit " + pname + " intensity=" + intensity + " hp-" + dmg + " shock-" + shock);
			}
		}
	}

	// Постоянный цикл бурь (по умолчанию выключен).
	static void AutoStartCycle(DP_SandstormConfig cfg, Weather weather, Sandstorm storm)
	{
		if (!cfg.AutoStart.Enabled)
		{
			return;
		}
		int cooldownTicks = cfg.AutoStart.CooldownSeconds / TICK_SEC;
		if (storm.IsActive())
		{
			s_cooldownTicks = cooldownTicks;
			return;
		}
		if (s_cooldownTicks > 0)
		{
			s_cooldownTicks = s_cooldownTicks - 1;
			return;
		}
		StartStorm(cfg, weather, storm, cfg.AutoStart.GatherTimeSeconds);
		s_cooldownTicks = cooldownTicks;
	}

	// Случайный старт при условиях — как на Nasdara: время от времени, при
	// пасмурной погоде, бросок кубика; шанс и порог настраиваются.
	static void RandomStartCheck(DP_SandstormConfig cfg, Weather weather, Sandstorm storm)
	{
		if (!cfg.RandomStart.Enabled)
		{
			return;
		}
		if (storm.IsActive())
		{
			return;
		}
		if (s_cooldownTicks > 0)
		{
			s_cooldownTicks = s_cooldownTicks - 1;
			return;
		}
		int intervalTicks = (cfg.RandomStart.CheckIntervalMinutes * 60) / TICK_SEC;
		if (intervalTicks < 1)
		{
			intervalTicks = 1;
		}
		if (s_tickCount % intervalTicks != 0)
		{
			return;
		}
		float overcast = weather.GetOvercast().GetActual();
		if (overcast < cfg.RandomStart.MinOvercast)
		{
			return;
		}
		float roll = Math.RandomFloat01() * 100.0;
		if (roll >= cfg.RandomStart.ChancePercent)
		{
			return;
		}
		Print("[DP_SANDSTORM] случайный старт: бросок=" + roll + " шанс=" + cfg.RandomStart.ChancePercent + " overcast=" + overcast);
		StartStorm(cfg, weather, storm, cfg.AutoStart.GatherTimeSeconds);
		s_cooldownTicks = (cfg.AutoStart.CooldownSeconds / TICK_SEC);
	}

	// Старт бури с погодной обвязкой (ветер по направлению фронта, как dev-консоль).
	static void StartStorm(DP_SandstormConfig cfg, Weather weather, Sandstorm storm, float gatherTime)
	{
		bool ok = storm.Start(gatherTime, false);
		s_cycle = s_cycle + 1;
		Print("[DP_SANDSTORM] cycle " + s_cycle + " Start ok=" + AsInt(ok) + " pos=" + storm.GetPosition() + " dir=" + storm.GetDirection());
		if (ok)
		{
			float angle = weather.WindDirectionToAngle(storm.GetDirection());
			float holdTime = 99999;
			weather.GetWindDirection().Set(angle, 30, holdTime);
			weather.GetOvercast().Set(0.9, 30, holdTime);
			weather.GetWindMagnitude().Set(20.0, 30, holdTime);
			weather.GetRain().Set(0, 30, holdTime);
		}
	}

	// Максимальная защита от твёрдых частиц по очкам/маскам игрока.
	static float GetFaceProtection(PlayerBase pb)
	{
		CachedEquipmentStorageQuery query = new CachedEquipmentStorageQuery();
		query.m_Category = ECachedEquipmentItemCategory.EYEWEAR | ECachedEquipmentItemCategory.MASK;
		query.m_Placement = ECachedEquipmentPlacement.ATTACHMENT;
		query.m_MaximumDepth = 2;

		array<Entity> items = pb.GetCachedEquipment().GetEntitiesByCategory(query);
		for (int i = 0; i < items.Count(); i++)
		{
			ItemBase item = ItemBase.Cast(items.Get(i));
			if (item)
			{
				float lvl = item.GetProtectionLevel(DEF_DUST_PARTICLE_BREATH);
				if (lvl > 0.0)
				{
					return lvl;
				}
			}
		}
		return 0.0;
	}
}
