---
name: chat-templater-template-config
description: >-
  Как настраивать шаблоны чата ChatTemplater и его встроенный селектор: файлы
  addons/amxmodx/configs/ChatTemplater/Templates/*.json (массив объектов
  {Key, Template, Description}), Selector.json (Say / SayTeam + уточнения), синтаксис
  шаблона (плейсхолдеры {group:key}, цвета ^1/^3/^4, специальный {chat:text}),
  встроенные группы плейсхолдеров (global и player), поведение при ошибках и
  типовые проблемы. Использовать, когда нужно создать или поправить файл
  шаблона чата или встроенный селектор. Триггеры: шаблон чата, chat template,
  chat template config, configs/ChatTemplater, Templates/, Selector.json,
  ChatTemplate, цвета чата, ^1 ^3 ^4, плейсхолдеры чата, {chat:text},
  {p:name}, {map}. Use when configuring ChatTemplater templates and the
  built-in selector. Do NOT use for replacing the selector in code (use
  chat-templater-selector-swap), or for general ParamsController mechanics
  (use params-usage / param-types).
---

# ChatTemplater: настройка шаблонов и встроенного селектора

Как заполнять файлы шаблонов и глобальный `Selector.json`, какие плейсхолдеры и
цветовые коды доступны, что происходит при ошибке.

## Коротко

Две вещи в конфиге:

1. **Шаблоны.** Каждый файл в `Templates/` это JSON-массив объектов с ключом
   `Key` и строкой `Template`.
2. **Встроенный селектор.** `Selector.json` задаёт ключи шаблонов: базовые
   `Say` / `SayTeam` и необязательные уточнения по состоянию (`SayDead`,
   `SaySpec`, `SayTeamT` / `SayTeamCT`, `SayTeamDead`, ...) с откатом, в
   пределе — `say_team` на `say`.

Строка шаблона собирается из плейсхолдеров ParamsController `{group:key}` и
цветовых кодов чата `^1` / `^3` / `^4`. Плейсхолдер `{chat:text}` подставляет
текст самого игрока.

Замена селектора кодом (per-player) это отдельный скилл:
`chat-templater-selector-swap`.

## Раскладка конфигов

```text
addons/amxmodx/configs/ChatTemplater/
├── Templates/
│   ├── <file>.json        # массив объектов шаблонов
│   └── ...
└── Selector.json          # селектор (Say / SayTeam + уточнения)
```

## Файлы шаблонов

Каждый файл в `Templates/` это **JSON-массив** объектов:

```json
[
    {
        "Key": "vip_say",
        "Template": "{p:name}^1: {chat:text}",
        "Description": "Общий чат для VIP"
    },
    {
        "Key": "vip_sayteam",
        "Template": "(^3TEAM^1) {p:name}: {chat:text}"
    }
]
```

- `Key` (**обязательный**) уникальный ключ шаблона; на него ссылается селектор.
- `Template` (**обязательный**) строка шаблона.
- `Description` (необязательный) короткое описание.

Файлы читаются через `PCJson_ParseFile` (`PCJson` из ParamsController), и
обрабатываются **одинаково**: кладите шаблоны в любое число файлов, важно лишь
чтобы ключи не повторялись.

## Встроенный селектор (Selector.json)

Объект «ключ селектора → `Key` шаблона». Базовые ключи — по одному на канал,
остальные уточняют состояние и **необязательны**:

```json
{
    "Say": "say_default",
    "SayDead": "say_dead",
    "SaySpec": "say_spec",
    "SayTeam": "sayteam_default",
    "SayTeamT": "sayteam_t",
    "SayTeamCT": "sayteam_ct",
    "SayTeamDead": "sayteam_dead",
    "SayTeamTDead": "sayteam_t_dead",
    "SayTeamCTDead": "sayteam_ct_dead",
    "SayTeamSpec": "sayteam_spec"
}
```

| Ключ | Когда применяется | Откат, если не задан |
|---|---|---|
| `Say` | `say`, жив | — (последний рубеж) |
| `SayDead` | `say`, мёртв | `Say` |
| `SaySpec` | `say`, наблюдатель | `Say` |
| `SayTeam` | `say_team`, жив | `Say` |
| `SayTeamT` | `say_team`, жив, Terrorist | `SayTeam` → `Say` |
| `SayTeamCT` | `say_team`, жив, Counter-Terrorist | `SayTeam` → `Say` |
| `SayTeamDead` | `say_team`, мёртв (любая сторона) | `SayTeam` → `SayDead` → `Say` |
| `SayTeamTDead` | `say_team`, мёртв, Terrorist | `SayTeamDead` → `SayTeam` → `SayDead` → `Say` |
| `SayTeamCTDead` | `say_team`, мёртв, Counter-Terrorist | `SayTeamDead` → `SayTeam` → `SayDead` → `Say` |
| `SayTeamSpec` | `say_team`, наблюдатель | `SayTeam` → `SaySpec` → `Say` |

- Значения — это `Key` шаблонов.
- Каждый ключ необязателен, кроме `Say`: незаданный откатывается на более
  общий, затем на базовый ключ канала, а `say_team` — дальше на цепочку `say`
  того же состояния (`SayDead` / `SaySpec` / `Say`).
- Канал (`say` / `say_team`) определяет ядро; состояние (жив / мёртв /
  наблюдатель) и сторону (T / CT) селектор определяет сам по игроку.
- Действует **глобально** для всех игроков.

## Синтаксис шаблона

| Элемент | Что делает |
|---|---|
| `{group:key}` | Плейсхолдер ParamsController (например `{p:name}`) |
| `{key}` | Плейсхолдер глобальной группы (например `{map}`) |
| `{chat:text}` | Текст самого игрока (его сообщение) |
| `^1` | Цвет по умолчанию (белый) |
| `^3` | Цвет команды |
| `^4` | Зелёный |

Кастомный тип параметра `ChatTemplate` преобразует `^1` / `^3` / `^4` в
байтовые коды цвета движка и компилирует шаблон плейсхолдеров. То есть цвета не
нужно писать вручную в виде сырых байтов: достаточно `^`-кодов.

## Встроенные группы плейсхолдеров

Ключевые (полный список даёт ParamsController через свои группы плейсхолдеров):

**Глобальная группа** (вид `{key}`):

| Плейсхолдер | Значение |
|---|---|
| `{map}` | имя карты |
| `{time}` | текущее время |
| `{date}` | текущая дата |
| `{server-name}` | имя сервера |
| `{players-count}` | число игроков |
| `{nextmap}` | следующая карта |
| `{uptime}` | время работы сервера |

**Группа игрока** (префикс `p`, вид `{p:key}`):

| Плейсхолдер | Значение |
|---|---|
| `{p:name}` | имя игрока |
| `{p:team-name}` | название команды |
| `{p:health}` | здоровье |
| `{p:armor}` | броня |
| `{p:frags}` | фраги |
| `{p:ping}` | пинг |
| `{p:alive}` | жив ли игрок |
| `{p:index}` | индекс игрока |

## Примеры

Общий чат с именем и текстом:

```json
{
    "Key": "say_default",
    "Template": "{p:name}^1: {chat:text}"
}
```

Командный чат с цветной меткой команды:

```json
{
    "Key": "sayteam_default",
    "Template": "(^3TEAM^1) {p:name}: {chat:text}"
}
```

Отдельный шаблон по состоянию. Встроенный селектор умеет выбирать шаблон под
живость/спектаторство и сторону через ключи `SayDead`, `SaySpec`, `SayTeamT`,
`SayTeamDead`, ... (см. раздел про `Selector.json`):

```json
{
    "Key": "say_dead",
    "Template": "^1*МЁРТВ* {p:name}^1: {chat:text}"
}
```

```json
{
    "Say": "say_default",
    "SayDead": "say_dead"
}
```

Вывести состояние можно и одним шаблоном, без переключения, через `{p:alive}`
(0/1):

```json
{
    "Key": "say_state",
    "Template": "{p:name}^1 [{p:alive}]: {chat:text}"
}
```

Цветной префикс без `RGB`, только `^`-коды:

```json
{
    "Key": "say_vip",
    "Template": "^4[VIP]^1 {p:name}^1: {chat:text}  (^3{map}^1)"
}
```

Больше готовых примеров в `references/examples.md`.

## Поведение при ошибках

- Пропущен `Key` или `Template` в объекте: элемент шаблона невалиден.
- Уточняющий ключ состояния не задан (или его шаблон не найден): селектор
  откатывается на более общий ключ (`SayTeamTDead` → `SayTeamDead` →
  `SayTeam`), в пределе — на базовый `Say` / `SayTeam`.
- Базовый ключ канала отсутствует или его шаблон не найден
  (`Invalid_Template`): сообщение отправляется без изменений.

## Диагностика

- **Цвета не показываются.** Проверьте `^1` / `^3` / `^4` (не `&`, не `\x`), и
  что строка реально проходит через `ChatTemplate`.
- **Плейсхолдер показан буквально, например `{p:name}`.** Группа или ключ были
  неизвестны на момент компиляции шаблона. Неизвестные плейсхолдеры фиксируются
  как литеральный текст. Проверьте имя группы/ключа и что нужные группы
  зарегистрированы (полный список из ParamsController).
- **Строка обрезана.** Есть предел длины (~190 ячеек); сокращайте шаблон или
  число плейсхолдеров.
- **Шаблон не применился.** Проверьте, что ключ из `Selector.json` совпадает с
  `Key` в `Templates/*.json`.

## Ссылки

| Тема | Файл |
|---|---|
| Полные примеры шаблонов и селектора | `references/examples.md` |

## Чек-лист перед завершением

- [ ] Файлы шаблонов лежат в `addons/amxmodx/configs/ChatTemplater/Templates/`.
- [ ] Каждый файл это JSON-массив объектов с `Key` и `Template`.
- [ ] Ключи `Key` уникальны и не повторяются между файлами.
- [ ] `Selector.json` ссылается на существующие `Key`.
- [ ] Цвета заданы через `^1` / `^3` / `^4`.
- [ ] `{chat:text}` присутствует там, где нужен текст игрока.
- [ ] Плейсхолдеры проверены по доступным группам.
- [ ] Длина строки укладывается в предел (~190 ячеек).
