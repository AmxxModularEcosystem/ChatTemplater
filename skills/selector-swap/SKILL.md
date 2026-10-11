---
name: chat-templater-selector-swap
description: >-
  Как заменить встроенный селектор шаблонов ChatTemplater на свой, например
  селектор по VipModular-лимитам для каждого игрока отдельно. Форвард
  CT_OnRequestTemplate, захват решения через CT_SetRequestedTemplate +
  PLUGIN_HANDLED, отказ в пользу встроенного через PLUGIN_CONTINUE, откат
  CT_GetBuiltinTemplate, поиск CT_FindTemplate, каналы E_CTChannel
  (CTChannel_Say / CTChannel_SayTeam) и T_Template. Использовать для подмены
  селектора шаблонов чата и выдачи игрокам разных шаблонов.
  Триггеры: подмена селектора, замена селектора, селектор шаблонов, selector,
  chat template selector, custom selector, CT_OnRequestTemplate,
  CT_SetRequestedTemplate, ChatTemplater selector, VipModular selector,
  per-player template, шаблон чата на игрока. Use when replacing ChatTemplater's
  built-in template selector with a custom one (e.g. a VipModular per-player
  selector). Do NOT use for configuring templates and Selector.json
  (use chat-templater-template-config), for building (amxb-migration), or for
  Pawn style (amxx-pawn-style).
---

# ChatTemplater: подмена селектора шаблонов

Как перехватить выбор шаблона чата и отдавать свой результат для каждого
игрока, а не брать глобальную пару ключей из встроенного селектора.

## Коротко

1. `#include <ChatTemplater>`.
2. Реализовать `public CT_OnRequestTemplate(const playerIndex, const E_CTChannel:channel)`.
3. Захват: `CT_SetRequestedTemplate("<key>")` и `return PLUGIN_HANDLED`.
   Отказ (не VIP): `return PLUGIN_CONTINUE`, ядро берёт встроенный селектор.

## Когда применять / When to use

- Нужно, чтобы разные игроки получали разные шаблоны чата (по правам, по
  VipModular-лимитам, по рангу и т.п.).
- Нужно полностью заменить встроенный селектор, а не настраивать его.
- Нужно точечно переопределять шаблон только для части игроков, остальным
  оставить встроенный выбор.

Смежные скиллы:

| Задача | Скилл |
|---|---|
| Настройка самих шаблонов и `Selector.json` | `chat-templater-template-config` |
| Сборка и зависимости | `amxb-migration` |
| Стиль Pawn (имена, теги, форварды) | `amxx-pawn-style` |

## Как устроен встроенный селектор

ChatTemplater несёт встроенный селектор: это **глобальная** пара ключей шаблонов
из `addons/amxmodx/configs/ChatTemplater/Selector.json`:

```json
{
    "Say": "<key>",
    "SayTeam": "<key>"
}
```

Пара применяется ко всем игрокам без разбора.

Все 8 токенов CS SayText сводятся ядром к двум логическим каналам:

| Канал | Какие токены покрывает |
|---|---|
| `CTChannel_Say` | All, AllDead, AllSpec |
| `CTChannel_SayTeam` | T, T_Dead, CT, CT_Dead, Spec |

## Как подменить (процесс)

1. Плагин подключает ядро: `#include <ChatTemplater>`.
2. Реализует публичный форвард:
   ```pawn
   public CT_OnRequestTemplate(const playerIndex, const E_CTChannel:channel)
   {
       // ...
   }
   ```
3. Чтобы **захватить** решение: вызвать `CT_SetRequestedTemplate("<templateKey>")`
   и вернуть `PLUGIN_HANDLED`. Это останавливает цепочку селекторов.
   Решение выигрывает **первый** плагин, вернувший `PLUGIN_HANDLED`.
4. Чтобы **отказаться** (например, игрок не VIP): вернуть `PLUGIN_CONTINUE`.
   Тогда ядро падает обратно на встроенный селектор.
5. Для явного отката к встроенному результату вызвать
   `CT_GetBuiltinTemplate(playerIndex, channel)`, вернёт `T_Template`.
6. Ключи шаблонов ищутся через `CT_FindTemplate(key)`. Несуществующий шаблон
   даёт `Invalid_Template`, и сырое сообщение отправляется без изменений.

## API селектора

| Сущность | Вид | Назначение |
|---|---|---|
| `CT_OnRequestTemplate` | `forward CT_OnRequestTemplate(const playerIndex, const E_CTChannel:channel)` | Ядро спрашивает плагин, какой шаблон использовать |
| `CT_SetRequestedTemplate` | натив | Задать ключ выбранного шаблона |
| `CT_GetBuiltinTemplate` | натив, возвращает `T_Template` | Получить встроенный (Selector.json) результат |
| `CT_FindTemplate` | натив, возвращает `T_Template` | Найти шаблон по ключу |
| `E_CTChannel` | enum | `CTChannel_Say`, `CTChannel_SayTeam` |
| `T_Template` | tag | `Invalid_Template` для отсутствующего шаблона |

## Минимальный перехватчик

```pawn
#include <ChatTemplater>

new const SAY_TEMPLATE_KEY[] = "vip_say";

public CT_OnRequestTemplate(const playerIndex, const E_CTChannel:channel)
{
    if (channel != CTChannel_Say)
        return PLUGIN_CONTINUE;

    if (CT_FindTemplate(SAY_TEMPLATE_KEY) == Invalid_Template)
        return PLUGIN_CONTINUE;

    CT_SetRequestedTemplate(SAY_TEMPLATE_KEY);
    return PLUGIN_HANDLED;
}
```

Полный рабочий пример (селектор по VipModular-лимитам для каждого игрока) в
`references/example-vipmodular.md`.

## Ограничения и порядок загрузки

- Захватывать решение должен **только один** внешний селектор. Несколько
  захватчиков дают неопределённый порядок: выигрывает тот, чей форвард
  сработал первым.
- Плагин-селектор грузится **после** ядра ChatTemplater.
- Плагин-селектор должен зависеть от **той же версии ParamsController**, что и
  ядро, иначе возможны расхождения в совместимости API.

## Частые ошибки

- Вернули `PLUGIN_HANDLED`, но не вызвали `CT_SetRequestedTemplate`: шаблон не
  задан, решение захвачено впустую.
- Вызвали `CT_SetRequestedTemplate`, но вернули `PLUGIN_CONTINUE`: решение не
  захвачено, ядро использует встроенный селектор.
- Ключ шаблона неизвестен (`CT_FindTemplate` вернул `Invalid_Template`):
  сообщение уходит как есть.
- Несколько плагинов возвращают `PLUGIN_HANDLED`: победитель не детерминирован.

## Ссылки

| Тема | Файл |
|---|---|
| Полный пример селектора на VipModular-лимитах | `references/example-vipmodular.md` |

## Чек-лист перед завершением

- [ ] Есть `#include <ChatTemplater>`.
- [ ] Реализован `CT_OnRequestTemplate(playerIndex, channel)`.
- [ ] При захвате: вызван `CT_SetRequestedTemplate` и возвращён `PLUGIN_HANDLED`.
- [ ] При отказе: возвращён `PLUGIN_CONTINUE`.
- [ ] Ключ проверен через `CT_FindTemplate` (иначе `Invalid_Template`).
- [ ] Учтён порядок загрузки (после ядра) и единственный захватчик.
