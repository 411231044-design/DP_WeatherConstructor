// KR_SandstormPort: игровой слой песчаной бури (серверный скрипт мода).
// Автостарт цикла бурь + урон здоровью/шоку; укрытия и защита лица —
// ванильная логика ScriptedSandstormController.GetIntensityForPlayer и
// GetProtectionLevel(DEF_DUST_PARTICLE_BREATH). Настройки — классы
// константы ниже в этом файле.

class DPSandstormEffects
{
	// ── Настройки сервера (перенесены из config.cpp по конвенции мода) ──
	static const bool DAMAGE_ENABLED = true;   // прямой урон здоровью/шоку в буре
	static const float HEALTH_PER_SECOND = 0.6;
	static const float SHOCK_PER_SECOND = 0.25;
	static const float MIN_INTENSITY = 0.2;    // ниже — урона нет
	static const bool AUTO_START = true;       // мод сам крутит цикл бурь
	static const float GATHER_TIME = 30;       // разгон магнитуды, с
	static const int COOLDOWN_SEC = 60;        // пауза между проходами, с

	static const float TICK_SEC = 3.0;
	static const int TICK_MS = 3000;
	static const int CLEANUP_EVERY = 20;

	static int s_cooldownTicks = 0;
	static int s_cycle = 0;
	static int s_tickCount = 0;

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
		Weather weather = g_Game.GetWeather();
		if (!weather)
		{
			return;
		}
		Sandstorm storm = weather.GetSandstorm();
		if (!storm)
		{
			return;
		}
		ScriptedSandstormController ssc = ScriptedSandstormController.Cast(storm);

		// heartbeat: первые три тика и далее каждые 20 — состояние в журнал
		if (s_tickCount <= 3 || s_tickCount % 20 == 0)
		{
			Print("[DP_SANDSTORM] hb tick=" + s_tickCount + " active=" + AsInt(storm.IsActive()) + " pos=" + storm.GetPosition() + " mag=" + storm.GetMagnitude());
		}

		// чистка данных отключившихся: серверная ветка OnUpdate её не делает
		if (ssc && s_tickCount % CLEANUP_EVERY == 0)
		{
			ssc.CheckConnectedPlayers();
		}

		AutoStartCycle(weather, storm);

		if (!storm.IsActive())
		{
			return;
		}

		bool enabled = DAMAGE_ENABLED;
		if (!enabled)
		{
			return;
		}
		float minIntensity = MIN_INTENSITY;
		float hpPerSec = HEALTH_PER_SECOND;
		float shockPerSec = SHOCK_PER_SECOND;

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
			if (intensity <= minIntensity)
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

			float dmg = hpPerSec * intensity * TICK_SEC;
			float shock = shockPerSec * intensity * TICK_SEC;
			pb.DecreaseHealth("", "Health", dmg);
			pb.DecreaseHealth("", "Shock", shock);

			string pname = "unknown";
			if (pb.GetIdentity())
			{
				pname = pb.GetIdentity().GetName();
			}
			Print("[DP_SANDSTORM] cycle " + s_cycle + " hit " + pname + " intensity=" + intensity + " hp-" + dmg + " shock-" + shock);
		}
	}

	// Автостарт: как только буря не активна и прошло ожидание — новый цикл.
	static void AutoStartCycle(Weather weather, Sandstorm storm)
	{
		bool autoStart = AUTO_START;
		if (!autoStart)
		{
			return;
		}
		int cooldownTicks = COOLDOWN_SEC / TICK_SEC;
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

		float gather = GATHER_TIME;
		bool ok = storm.Start(gather, false);
		s_cycle = s_cycle + 1;
		Print("[DP_SANDSTORM] cycle " + s_cycle + " auto Start ok=" + AsInt(ok));
		if (ok)
		{
			float angle = weather.WindDirectionToAngle(storm.GetDirection());
			float holdTime = 99999;
			weather.GetWindDirection().Set(angle, 30, holdTime);
			weather.GetOvercast().Set(0.9, 30, holdTime);
			weather.GetWindMagnitude().Set(20.0, 30, holdTime);
			weather.GetRain().Set(0, 30, holdTime);
		}
		s_cooldownTicks = cooldownTicks;
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
