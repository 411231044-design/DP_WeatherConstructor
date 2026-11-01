// KR_SandstormPort: подключение игрового слоя (Mission-модуль).
modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		Print("[DP_SANDSTORM] mission hook: OnInit");
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DPSandstormEffects.Tick, DPSandstormEffects.TICK_MS, true);
	}
}
