// DP_WeatherConstructor: движковые песчаные бури и вьюги на любой карте (сервер).
//
// Две независимые подсистемы с общим тик-движком и общим мастер-выключателем:
//  - Песчаная буря (SandstormController, механика Badlands): автостарт/случайный
//    старт, прямой урон с ванильными укрытиями и защитой лица.
//  - Вьюга (механика Сахала, thundersnow): снегопад+ветер+мгла, снимает
//    скриптовый замок снега у Черноруси, холод — вычетом из GetTemperature мира,
//    после метели — «замело»: лёгкий снег и приземная дымка, затем таяние.
// Конфиг: DarkProjectConfig/WeatherConstructor/config.json в профиле сервера
// (паттерн DP_AdminMenu, создаётся с дефолтами). Маршрут песчаной бури — в
// config.cpp аддона (CfgWorlds >> Weather >> Sandstorm), его читает сам движок.

// ── Конфиг: песчаная буря ──

class DPW_AutoStartCfg
{
	bool Enabled = false;            // автостарт по дефолту ВЫКЛЮЧЕН
	float GatherTimeSeconds = 30;    // разгон магнитуды, с
	int CooldownSeconds = 60;        // пауза между проходами, с
}

class DPW_RandomStartCfg
{
	bool Enabled = true;             // случайный старт при условиях (как на Nasdara)
	float ChancePercent = 8;         // шанс на проверке
	float MinOvercast = 0.75;        // буря возможна только при пасмурной погоде
	int CheckIntervalMinutes = 10;   // как часто бросок
}

class DPW_SandstormDamageCfg
{
	bool Enabled = true;             // прямой урон здоровью/шоку в буре
	float HealthPerSecond = 0.6;     // хп/с при интенсивности 1 без защиты
	float ShockPerSecond = 0.25;     // шок/с
	float MinIntensity = 0.2;        // ниже — урона нет
}

// ── Конфиг: вьюга ──

class DPW_BlizzardRandomCfg
{
	bool Enabled = true;
	float ChancePercent = 10;
	float MinOvercast = 0.7;
	int CheckIntervalMinutes = 15;
}

class DPW_BlizzardCfg
{
	bool Enabled = true;                 // подсистема вьюги
	ref DPW_BlizzardRandomCfg RandomStart = new DPW_BlizzardRandomCfg();

	float GatherMinutes = 3;             // разгон явлений
	float DurationMinutes = 12;          // держится метель
	float MeltMinutes = 20;              // «тает»: лёгкий снег + приземная дымка

	float SnowfallIntensity = 0.9;       // плотность снегопада в метель
	float SnowfallThreshold = 0.5;       // окно облачности для снега (мин)
	float WindMaximum = 20;              // ветер, м/с
	float VolFogStormDensity = 0.3;      // мгла метели (видимость)
	float VolFogMeltDensity = 0.1;       // «замело» после метели
	float MeltHeightDensity = 0.02;      // приземная дымка после
	float TemperatureDelta = -15;        // вычет из температуры мира в метель
	bool Thundersnow = true;             // молнии при облачности ~1
}

// ── Конфиг: журнал и корень ──

class DPW_LoggingCfg
{
	bool LogHits = true;             // строки урона в script-лог
	bool Heartbeat = true;           // статус раз в минуту и первые три тика
}

class DPW_SandstormCfg
{
	bool Enabled = true;             // ВЫКЛЮЧАТЕЛЬ песчаной бури (отдельно от вьюги)
	ref DPW_AutoStartCfg AutoStart = new DPW_AutoStartCfg();
	ref DPW_RandomStartCfg RandomStart = new DPW_RandomStartCfg();
	ref DPW_SandstormDamageCfg Damage = new DPW_SandstormDamageCfg();
}

class DP_WeatherConfig
{
	bool Enabled = true;             // ГЛАВНЫЙ ВЫКЛЮЧАТЕЛЬ: вся механика мода
	ref DPW_SandstormCfg Sand = new DPW_SandstormCfg();
	ref DPW_BlizzardCfg Blizzard = new DPW_BlizzardCfg();
	ref DPW_LoggingCfg Logging = new DPW_LoggingCfg();
}

// ── Тик-движок ──

class DPWeatherEffects
{
	static const string CONFIG_ROOT_DIR = "$profile:DarkProjectConfig";
	static const string CONFIG_DIR = "$profile:DarkProjectConfig/WeatherConstructor";
	static const string CONFIG_FILE = "$profile:DarkProjectConfig/WeatherConstructor/config.json";

	static const float TICK_SEC = 3.0;
	static const int TICK_MS = 3000;
	static const int CLEANUP_EVERY = 20;

	// вьюга: 0 — тихо, 1 — метель, 2 — таяние
	static const int BLIZZARD_IDLE = 0;
	static const int BLIZZARD_STORM = 1;
	static const int BLIZZARD_MELT = 2;

	static ref DP_WeatherConfig s_config = null;
	static int s_cooldownTicks = 0;
	static int s_cycle = 0;
	static int s_tickCount = 0;
	static int s_blizzardState = 0;
	static int s_blizzardPhaseTicks = 0;
	static int s_blizzardCooldownTicks = 0;
	static int s_blizzardCycle = 0;
	static bool s_sandStoppedByDisable = false;

	static DP_WeatherConfig Config()
	{
		if (s_config)
		{
			return s_config;
		}

		MakeDirectory(CONFIG_ROOT_DIR);
		MakeDirectory(CONFIG_DIR);
		s_config = new DP_WeatherConfig();

		if (!FileExist(CONFIG_FILE))
		{
			JsonFileLoader<DP_WeatherConfig>.JsonSaveFile(CONFIG_FILE, s_config);
			Print("[DP_WEATHER] создан " + CONFIG_FILE + " — правь и перезапусти сервер");
			return s_config;
		}

		JsonFileLoader<DP_WeatherConfig>.JsonLoadFile(CONFIG_FILE, s_config);
		Print("[DP_WEATHER] config загружен: механика=" + AsInt(s_config.Enabled) + " буря=" + AsInt(s_config.Sand.Enabled) + " вьюга=" + AsInt(s_config.Blizzard.Enabled));
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
		DP_WeatherConfig cfg = Config();

		// ГЛАВНЫЙ ВЫКЛЮЧАТЕЛЬ: без него мод полностью молчит
		if (!cfg.Enabled)
		{
			return;
		}

		Weather weather = g_Game.GetWeather();

		if (cfg.Logging.Heartbeat && (s_tickCount <= 3 || s_tickCount % 20 == 0))
		{
			Print(HbLine(weather));
		}

		if (!weather)
		{
			return;
		}

		if (cfg.Blizzard.Enabled)
		{
			BlizzardTick(cfg, weather);
		}
		if (cfg.Sand.Enabled)
		{
			SandstormTick(cfg, weather);
		}		else
		{
			StopSandstormIfDisabled(weather);
		}
	}

	static string HbLine(Weather weather)
	{
		int haveStorm = 0;
		int active = 0;
		int scripted = 0;
		string pos = "none";
		Sandstorm storm = null;
		if (weather)
		{
			storm = weather.GetSandstorm();
		}
		if (storm)
		{
			haveStorm = 1;
			active = AsInt(storm.IsActive());
			pos = storm.GetPosition().ToString();
			ScriptedSandstormController hbssc = ScriptedSandstormController.Cast(storm);
			scripted = AsInt(hbssc != null);
		}
		string snow = "0";
		if (weather)
		{
			snow = weather.GetSnowfall().GetActual().ToString();
		}
		return "[DP_WEATHER] hb tick=" + s_tickCount + " буря=" + active + " scripted=" + scripted + " вьюга=" + s_blizzardState + " снег=" + snow + " pos=" + pos;
	}

	// Выключенная в конфиге подсистема глушит и уже идущую бурю — включая
	// восстановленную из прошлого сеанса (иначе выключатель не действует
	// до конца прохода).
	static void StopSandstormIfDisabled(Weather weather)
	{
		Sandstorm storm = weather.GetSandstorm();
		if (!storm)
		{
			return;
		}
		if (storm.IsActive() && !s_sandStoppedByDisable)
		{
			storm.Stop(10, false);
			Print("[DP_WEATHER] подсистема бурь выключена в конфиге — останавливаю активную бурю");
		}
		s_sandStoppedByDisable = storm.IsActive();
	}

	// ── Подсистема: вьюга ─────────────────────────────────────────────────────

	static void BlizzardTick(DP_WeatherConfig cfg, Weather weather)
	{
		if (!cfg.Blizzard.Enabled)
		{
			return;
		}
		s_blizzardPhaseTicks = s_blizzardPhaseTicks + 1;

		if (s_blizzardState == BLIZZARD_STORM)
		{
			UnlockSnow(weather, cfg);
			int holdTicks = (cfg.Blizzard.DurationMinutes * 60) / TICK_SEC;
			if (s_blizzardPhaseTicks >= holdTicks)
			{
				BlizzardMelt(cfg, weather);
			}
			return;
		}

		if (s_blizzardState == BLIZZARD_MELT)
		{
			UnlockSnow(weather, cfg);
			int meltTicks = (cfg.Blizzard.MeltMinutes * 60) / TICK_SEC;
			if (s_blizzardPhaseTicks >= meltTicks)
			{
				BlizzardEnd(weather);
			}
			return;
		}

		// тихо: планировщик случайного старта
		if (s_blizzardCooldownTicks > 0)
		{
			s_blizzardCooldownTicks = s_blizzardCooldownTicks - 1;
			return;
		}
		int intervalTicks = (cfg.Blizzard.RandomStart.CheckIntervalMinutes * 60) / TICK_SEC;
		if (intervalTicks < 1)
		{
			intervalTicks = 1;
		}
		if (s_tickCount % intervalTicks != 0)
		{
			return;
		}
		float overcast = weather.GetOvercast().GetActual();
		if (overcast < cfg.Blizzard.RandomStart.MinOvercast)
		{
			return;
		}
		float roll = Math.RandomFloat01() * 100.0;
		if (roll >= cfg.Blizzard.RandomStart.ChancePercent)
		{
			return;
		}
		Print("[DP_WEATHER] вьюга: случайный старт, бросок=" + roll + " шанс=" + cfg.Blizzard.RandomStart.ChancePercent + " overcast=" + overcast);
		BlizzardStart(cfg, weather);
	}

	static void UnlockSnow(Weather weather, DP_WeatherConfig cfg)
	{
		// Чернорусь замыкает снег в veto-хуке на каждой смене облачности — снимаем
		weather.GetSnowfall().SetLimits(0, 1);
		weather.SetSnowfallThresholds(cfg.Blizzard.SnowfallThreshold, 1.0, 60);
	}

	static void BlizzardStart(DP_WeatherConfig cfg, Weather weather)
	{
		DPWeatherCold.Set(cfg.Blizzard.TemperatureDelta);
		s_blizzardCycle = s_blizzardCycle + 1;
		s_blizzardState = BLIZZARD_STORM;
		s_blizzardPhaseTicks = 0;

		float t = cfg.Blizzard.GatherMinutes * 60;
		weather.SetWindMaximumSpeed(cfg.Blizzard.WindMaximum);
		UnlockSnow(weather, cfg);
		weather.GetOvercast().Set(0.95, t, 99999);
		weather.GetSnowfall().Set(cfg.Blizzard.SnowfallIntensity, t, 99999);
		weather.GetWindMagnitude().Set(cfg.Blizzard.WindMaximum, t, 99999);
		weather.GetRain().Set(0, t, 99999);
		weather.SetDynVolFogDistanceDensity(cfg.Blizzard.VolFogStormDensity, t);
		if (cfg.Blizzard.Thundersnow)
		{
			weather.SetStorm(1.0, 0.98, 45);
		}
		Print("[DP_WEATHER] вьюга #" + s_blizzardCycle + " началась: снег=" + cfg.Blizzard.SnowfallIntensity + " ветер=" + cfg.Blizzard.WindMaximum + " мгла=" + cfg.Blizzard.VolFogStormDensity + " холод=" + cfg.Blizzard.TemperatureDelta);
	}

	static void BlizzardMelt(DP_WeatherConfig cfg, Weather weather)
	{
		DPWeatherCold.Set(cfg.Blizzard.TemperatureDelta * 0.5);
		s_blizzardState = BLIZZARD_MELT;
		s_blizzardPhaseTicks = 0;

		float t = cfg.Blizzard.MeltMinutes * 60;
		weather.GetSnowfall().Set(0.25, t, 99999);
		weather.SetDynVolFogDistanceDensity(cfg.Blizzard.VolFogMeltDensity, t);
		weather.SetDynVolFogHeightDensity(cfg.Blizzard.MeltHeightDensity, t);
		Print("[DP_WEATHER] вьюга #" + s_blizzardCycle + " стихла — замело, тает " + cfg.Blizzard.MeltMinutes + " мин");
	}

	static void BlizzardEnd(Weather weather)
	{
		DPWeatherCold.Set(0);
		s_blizzardState = BLIZZARD_IDLE;
		s_blizzardPhaseTicks = 0;

		weather.GetSnowfall().Set(0, 120, 0);
		weather.GetSnowfall().SetLimits(0, 0);
		weather.SetDynVolFogDistanceDensity(0.001, 120);
		weather.SetDynVolFogHeightDensity(0, 120);
		Print("[DP_WEATHER] вьюга #" + s_blizzardCycle + " закончилась, снег замкнут обратно");
	}

	// Вычет из температуры мира: полный в метель, половина в таяние.
	static float BlizzardTempDelta()
	{
		if (!s_config || !s_config.Blizzard || !s_config.Enabled)
		{
			return 0;
		}
		if (s_blizzardState == BLIZZARD_STORM)
		{
			return s_config.Blizzard.TemperatureDelta;
		}
		if (s_blizzardState == BLIZZARD_MELT)
		{
			return s_config.Blizzard.TemperatureDelta * 0.5;
		}
		return 0;
	}

	// ── Подсистема: песчаная буря ─────────────────────────────────────────────

	static void SandstormTick(DP_WeatherConfig cfg, Weather weather)
	{
		Sandstorm storm = weather.GetSandstorm();
		if (!storm)
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

		if (!storm.IsActive() || !cfg.Sand.Damage.Enabled)
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
			if (intensity <= cfg.Sand.Damage.MinIntensity)
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

			float dmg = cfg.Sand.Damage.HealthPerSecond * intensity * TICK_SEC;
			float shock = cfg.Sand.Damage.ShockPerSecond * intensity * TICK_SEC;
			pb.DecreaseHealth("", "Health", dmg);
			pb.DecreaseHealth("", "Shock", shock);

			if (cfg.Logging.LogHits)
			{
				string pname = "unknown";
				if (pb.GetIdentity())
				{
					pname = pb.GetIdentity().GetName();
				}
				Print("[DP_WEATHER] cycle " + s_cycle + " hit " + pname + " intensity=" + intensity + " hp-" + dmg + " shock-" + shock);
			}
		}
	}

	// Постоянный цикл бурь (по умолчанию выключен).
	static void AutoStartCycle(DP_WeatherConfig cfg, Weather weather, Sandstorm storm)
	{
		if (!cfg.Sand.AutoStart.Enabled)
		{
			return;
		}
		int cooldownTicks = cfg.Sand.AutoStart.CooldownSeconds / TICK_SEC;
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
		StartStorm(cfg, weather, storm, cfg.Sand.AutoStart.GatherTimeSeconds);
		s_cooldownTicks = cooldownTicks;
	}

	// Случайный старт при условиях — как на Nasdara.
	static void RandomStartCheck(DP_WeatherConfig cfg, Weather weather, Sandstorm storm)
	{
		if (!cfg.Sand.RandomStart.Enabled)
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
		int intervalTicks = (cfg.Sand.RandomStart.CheckIntervalMinutes * 60) / TICK_SEC;
		if (intervalTicks < 1)
		{
			intervalTicks = 1;
		}
		if (s_tickCount % intervalTicks != 0)
		{
			return;
		}
		float overcast = weather.GetOvercast().GetActual();
		if (overcast < cfg.Sand.RandomStart.MinOvercast)
		{
			return;
		}
		float roll = Math.RandomFloat01() * 100.0;
		if (roll >= cfg.Sand.RandomStart.ChancePercent)
		{
			return;
		}
		Print("[DP_WEATHER] буря: случайный старт, бросок=" + roll + " шанс=" + cfg.Sand.RandomStart.ChancePercent + " overcast=" + overcast);
		StartStorm(cfg, weather, storm, cfg.Sand.AutoStart.GatherTimeSeconds);
		s_cooldownTicks = (cfg.Sand.AutoStart.CooldownSeconds / TICK_SEC);
	}

	// Старт песчаной бури с погодной обвязкой (ветер по направлению фронта).
	static void StartStorm(DP_WeatherConfig cfg, Weather weather, Sandstorm storm, float gatherTime)
	{
		bool ok = storm.Start(gatherTime, false);
		s_cycle = s_cycle + 1;
		Print("[DP_WEATHER] cycle " + s_cycle + " Start ok=" + AsInt(ok) + " pos=" + storm.GetPosition() + " dir=" + storm.GetDirection());
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
