// DP_WeatherConstructor: подключение тик-движка (Mission-модуль).
modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		Print("[DP_WEATHER] mission hook: OnInit");
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DPWeatherEffects.Tick, DPWeatherEffects.TICK_MS, true);
	}
}
