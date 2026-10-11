# ChatTemplater

Шаблонизатор сообщений чата для AMX Mod X (Counter-Strike 1.6 / GoldSrc).
Каждое сообщение в чат (`say`, `say_team` и их dead/spectator-варианты)
перерисовывается через шаблон отправителя, собранный из плейсхолдеров
[ParamsController](https://github.com/AmxxModularEcosystem/ParamsController).
Шаблоны поддерживают цвета чата (`^1` / `^3` / `^4`).

Состоит из двух компонентов — **реестра шаблонов** и **селектора шаблонов** —
поставляемых одним плагином. Селектор можно подменить другим плагином через
публичный форвард (например, селектором на VipModular-лимитах).

## Возможности

- Перехват всех сообщений `SayText` и подмена на шаблон отправителя.
- Плейсхолдеры ParamsController (`{p:name}`, `{map}`, ...) + текст игрока `{chat:text}`.
- Цвета чата `^1` / `^3` / `^4`.
- Шаблоны в нескольких JSON-файлах (папка `Templates/`).
- Встроенный селектор: глобальная пара ключей `Say` / `SayTeam`.
- Подменяемый селектор (свой API для расширенных реализаций).

## Требования

- AMX Mod X 1.10+.
- [ParamsController](https://github.com/AmxxModularEcosystem/ParamsController) `1.5.0`
  — как зависимость сборки и как отдельный плагин на сервере.

## Установка

Готовый архив (см. релизы) распаковать в корень сервера (`addons/amxmodx/...`),
добавить `ChatTemplater.amxx` в `addons/amxmodx/configs/plugins.ini`.
На сервере также должен быть установлен `ParamsController.amxx`.

## Конфигурация

```
addons/amxmodx/configs/ChatTemplater/
├── Templates/
│   └── Example.json
└── Selector.json
```

### Шаблоны — `Templates/*.json`

Каждый файл — JSON-массив объектов:

```json
[
    {
        "Key": "default_say",
        "Template": "^3{p:name}^1: ^4{chat:text}",
        "Description": "Публичный чат"
    },
    {
        "Key": "default_sayteam",
        "Template": "^3({p:team-name}) {p:name}^1: ^4{chat:text}",
        "Description": "Командный чат"
    }
]
```

- `Key` — уникальный ключ шаблона (на него ссылается селектор).
- `Template` — строка шаблона с плейсхолдерами и цветами.
- `Description` — необязательное описание.

Файлы читаются через `PCJson` и обрабатываются одинаково: шаблонов может быть
сколько угодно, важно лишь чтобы ключи не повторялись.

### Встроенный селектор — `Selector.json`

```json
{
    "Say": "default_say",
    "SayTeam": "default_sayteam"
}
```

Глобальная (для всех игроков) пара ключей: для общего и командного чата.

### Плейсхолдеры

- Текст игрока: `{chat:text}`.
- Глобальная группа: `{map}`, `{time}`, `{date}`, `{server-name}`,
  `{players-count}`, `{nextmap}`, `{uptime}`, ...
- Группа игрока: `{p:name}`, `{p:team-name}`, `{p:health}`, `{p:armor}`,
  `{p:frags}`, `{p:ping}`, `{p:alive}`, `{p:index}`, ...

Полный список даёт группа плейсхолдеров ParamsController.

### Цвета

`^1` — обычный (белый), `^3` — цвет команды, `^4` — зелёный. Сырые байты цвета
писать не нужно: тип параметра `ChatTemplate` сам превращает `^`-коды в коды
движка и компилирует шаблон.

## Свой селектор (минимальный пример)

Селектор решает, какой шаблон использовать для пары (игрок, канал). Решение
захватывается публичным форвардом `CT_OnRequestTemplate`:

```pawn
#include <amxmodx>
#include <ChatTemplater>

public plugin_init()
{
    register_plugin("My CT Selector", "1.0", "author");
}

public CT_OnRequestTemplate(const playerIndex, const E_CTChannel:channel)
{
    // Не VIP — пусть решает встроенный селектор.
    if (!(get_user_flags(playerIndex) & ADMIN_LEVEL_H))
        return PLUGIN_CONTINUE;

    if (channel == CTChannel_Say)
        CT_SetRequestedTemplate("vip_say");
    else
        CT_SetRequestedTemplate("vip_sayteam");

    // Захватить решение и остановить цепочку селекторов.
    return PLUGIN_HANDLED;
}
```

- `CT_SetRequestedTemplate(key)` + `return PLUGIN_HANDLED` — использовать свой шаблон.
- `return PLUGIN_CONTINUE` — отдать встроенному селектору.
- `CT_GetBuiltinTemplate(playerIndex, channel)` — явный откат к встроенному результату.

Полный пример (селектор по VipModular-лимитам) — в скилле
`skills/selector-swap/references/example-vipmodular.md`.

## Публичный API

```pawn
enum E_CTChannel
{
    CTChannel_Say = 0,
    CTChannel_SayTeam,
};

enum T_Template
{
    Invalid_Template = 0,
};

// Реестр
native T_Template:CT_RegisterTemplate(const key[], const template[]);
native T_Template:CT_FindTemplate(const key[]);
native bool:CT_GetTemplateString(const T_Template:template, out[], const len);

// Подмена селектора
forward CT_OnRequestTemplate(const playerIndex, const E_CTChannel:channel);
native CT_SetRequestedTemplate(const key[]);
native T_Template:CT_GetBuiltinTemplate(const playerIndex, const E_CTChannel:channel);
```

## Сборка

```bash
amxb build
```

Зависимости и параметры сборки заданы в `amxbuild.yml`. Компиляция — последним
доступным компилятором AMXX.

## Скиллы для агентов

- `chat-templater-template-config` — настройка шаблонов и встроенного селектора.
- `chat-templater-selector-swap` — подмена селектора (пример на VipModular-лимитах).
