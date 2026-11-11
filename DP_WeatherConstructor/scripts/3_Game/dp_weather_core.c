// DP_WeatherConstructor (3_Game): холод вьюги.
// modded WorldData обязан жить в том же модуле, что и сам WorldData (3_Game):
// наследование между модулями можно, моддинг — нет. Значение дельты сеттит
// 4_World-движок мода (DPWeatherCold.Set) при переходах вьюги.

class DPWeatherCold
{
	static float s_delta = 0;

	static void Set(float d)
	{
		s_delta = d;
	}

	static float Get()
	{
		return s_delta;
	}
}

modded class WorldData
{
	override float GetTemperature(Object object, EEnvironmentTemperatureComponent properties)
	{
		float result = super.GetTemperature(object, properties);
		return result + DPWeatherCold.Get();
	}
}
