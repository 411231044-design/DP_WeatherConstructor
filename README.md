# DP_SandstormPort — движковые песчаные бури на любой карте (DayZ 1.30+)

Переносит движковую песчаную бурю (`SandstormController`, механика DLC-карты Badlands)
на любую другую карту — проверено на Черногорке, ванильным клиентом, сервер 1.30.164.200.

**Клиентам не нужно ничего.** Мод подключается на сервере ТОЛЬКО как `-servermod`:
маршрут, частицы и состояние фронта реплицируются движком.

Исследование движковой части (разбор бинаря, карта крашей, поля объекта) — в
[KR_GRAFTED, RESEARCH/dayz/weather.md](https://github.com/KRdayzmodding/KR_GRAFTED/blob/main/RESEARCH/dayz/weather.md).

## Как это работает

Движок читает маршрут из мирового конфига:

```cpp
class CfgWorlds
{
    class ChernarusPlus          // класс вашего мира (Livonia — Enoch и т.д.)
    {
        class Weather
        {
            class Sandstorm
            {
                startPosition[] = {2000.0, 0.0, 2300.0};    // откуда (X, Y, Z)
                endPosition[]   = {13000.0, 0.0, 2600.0};   // куда
                particlePath    = "Graphics/Particles/dust_vehicle";  // ОБЯЗАТЕЛЬНО, см. ниже
                sandstormIntensity = 1.0;
            };
        };
    };
};
```

Сама буря запускается из миссии (образец-цикл — [example/init.c](example/init.c)):
`GetGame().GetWeather().GetSandstorm().Start(gatherTime, false)` плюс погодная обвязка
(ветер по направлению фронта, overcast ≥ 0.75, ветер ≥ 18, дождь 0).

## Сборка

Нужен DayZ Tools (AddonBuilder):

```powershell
.\build.ps1            # соберёт @DP_SandstormPort\addons\DP_SandstormPort.pbo
```

## Установка (только сервер!)

```
-servermod=@DP_SandstormPort
```

Координаты маршрута — на свой вкус: фронт едет от `startPosition` к `endPosition`
со скоростью до ~20 м/с, «укрытия» (здания/техника/подземка) гасят эффект клиентским
контроллером движка.

## Важно (проверено живыми прогонами)

- **НЕ подключать мод на клиенте** (`-mod`): любая секция `Sandstorm` в CfgWorlds
  роняет клиент ещё в интро-сцене меню (ACCESS_VIOLATION), даже пустая.
- **`particlePath` обязателен**: дефолтный путь `Graphics/Particles/sandstorm/sandstorm`
  существует только в DLC Badlands; без явной замены ванильный клиент падает при входе
  в стену бури (0x400F0 «Game restart required»). Проверенная замена —
  `Graphics/Particles/dust_vehicle` (есть в базовой установке).
- Звуковых наборов буры (`SandStorm_*_SoundSet`) в базовой установке нет — буря беззвучна
  (движок пишет warning, не фатально).
- На серверах с тяжёлым модсетом мерж этой секции может валить сервер на парсинге
  конфигов (замерено на сборке @CF+@test+@server) — при падении биcектить модсет.
- Клиентский локальный маршрут остаётся вырожденным: визуал реплицируется сервером
  корректно, но клиентские проверки «укрытий» работают приближённо.

## Проверено

DayZ 1.30.164.200, выделенный сервер Windows, Черногорка, ванильный клиент:
фронт ходит по маршруту циклами (~10 мин проход + пауза), клиент видит бурю без модов,
крашей нет. Детали экспериментов — в указанном выше research-документе.

## Настройка

`DarkProjectConfig/SandstormPort/config.json` в профиле сервера (паттерн
DP_AdminMenu): создаётся с дефолтами при первом старте, правится и подхватывается
после перезапуска. Пример — [example/config.json](example/config.json).

| Секция | Ключ | Дефолт | Что |
|---|---|---|---|
| (корень) | `Enabled` | `true` | **главный выключатель всей механики** (первая настройка) |
| `AutoStart` | `Enabled` | `false` | постоянный цикл бурь — по умолчанию ВЫКЛЮЧЕН |
| | `GatherTimeSeconds` / `CooldownSeconds` | 30 / 60 | разгон магнитуды / пауза между проходами |
| `RandomStart` | `Enabled` | `true` | случайный старт при условиях, как на Nasdara |
| | `ChancePercent` | 8 | шанс на проверке (у Nasdara — 8%) |
| | `MinOvercast` | 0.75 | буря возможна только при пасмурной погоде |
| | `CheckIntervalMinutes` | 10 | как часто бросок |
| `Damage` | `Enabled` / `HealthPerSecond` / `ShockPerSecond` / `MinIntensity` | true / 0.6 / 0.25 / 0.2 | прямой урон в буре |
| `Logging` | `LogHits` / `Heartbeat` | true / true | строки урона и статус в script-лог |

Маршрут фронта (`startPosition`/`endPosition`, частицы) задаётся в `config.cpp`
аддона — его читает сам движок из CfgWorlds, скриптового сеттера маршрута у бурь
нет. При необходимости позиции можно вынести и в JSON: движковые поля маршрута
умеет писать graft-плагин (см. разбор в KR_GRAFTED, RESEARCH/dayz/weather.md) —
не реализовано, чтобы не плодить сущности.

## Важно (проверено живыми прогонами)
