# Пример: селектор ChatTemplater на VipModular-лимитах

## Содержание

- [Что делает плагин](#что-делает-плагин)
- [Код (иллюстративно)](#код-иллюстративно)
- [Соответствующий конфиг VipModular (иллюстративно)](#соответствующий-конфиг-vipmodular-иллюстративно)
- [На что смотреть при переносе](#на-что-смотреть-при-переносе)

> **Иллюстративный материал.** Это пример, а не гарантированный API. Точные
> сигнатуры VipModular (`VipM_Modules_Register`, `VipM_Modules_AddParams`,
> `VipM_Modules_GetParams`, `VipM_Params_ExecuteLimitsList`) и способ извлечь
> ключ шаблона из результата нужно **сверить в .inc самого VipModular и
> ParamsController** перед сборкой. Здесь показан только скелет логики и место
> вызова форварда ChatTemplater.

## Что делает плагин

- Регистрирует свой VipModular-модуль с параметрами `Say` и `SayTeam` (ключи
  шаблонов) и списком `Limits` (условия доступа).
- На каждый запрос шаблона (`CT_OnRequestTemplate`) читает параметры игрока:
  - если параметров нет или лимиты не выполнены, это не VIP, возвращает
    `PLUGIN_CONTINUE` (ядро берёт встроенный селектор);
  - если условия выполнены, ставит ключ шаблона через `CT_SetRequestedTemplate`
    и возвращает `PLUGIN_HANDLED`.

## Код (иллюстративно)

```pawn
#include <amxmodx>
#include <ChatTemplater>
#include <VipModular>

new const MODULE_NAME[] = "ChatTemplaterSelector";

public plugin_init()
{
    register_plugin("ChatTemplater Selector", "1.0", "author");
}

// Регистрация модуля и его параметров. Точный стиль объявления параметров
// (VipM_Modules_AddParams / AddParamsEx + PCParam) сверьте в .inc VipModular.
public VipM_OnInitModules()
{
    VipM_Modules_Register(MODULE_NAME, false);

    // Say     : ShortString  - ключ шаблона для общего чата
    // SayTeam : ShortString  - ключ шаблона для командного чата
    // Limits  : VipM-Limits  - условия доступа
    VipM_Modules_AddParams(MODULE_NAME, "Say");
    VipM_Modules_AddParams(MODULE_NAME, "SayTeam");
    VipM_Modules_AddParams(MODULE_NAME, "Limits");
}

public CT_OnRequestTemplate(const playerIndex, const E_CTChannel:channel)
{
    new userId = get_user_userid(playerIndex);

    // Параметры модуля для конкретного игрока.
    new Trie:params = VipM_Modules_GetParams(MODULE_NAME, userId);
    if (params == Invalid_Trie)
        return PLUGIN_CONTINUE;

    // Проверка доступа: не выполнились лимиты - это не VIP, отдаём встроенному.
    if (!VipM_Params_ExecuteLimitsList(params, "Limits", userId, Limit_Exec_AND))
        return PLUGIN_CONTINUE;

    // Извлечение ключа шаблона под нужный канал.
    // Способ чтения значения из params зависит от VipModular (см. его .inc);
    // ниже - представительный вариант через Trie.
    new templateKey[64];

    switch (channel)
    {
        case CTChannel_Say:
        {
            if (!TrieGetString(params, "Say", templateKey, charsmax(templateKey))
                || !templateKey[0])
                return PLUGIN_CONTINUE;
        }
        case CTChannel_SayTeam:
        {
            if (!TrieGetString(params, "SayTeam", templateKey, charsmax(templateKey))
                || !templateKey[0])
                return PLUGIN_CONTINUE;
        }
    }

    // Ключ может не существовать среди шаблонов: тогда Invalid_Template,
    // и сообщение уйдёт как есть. Поэтому можно либо проверить CT_FindTemplate,
    // либо явно откатиться на встроенный результат.
    if (CT_FindTemplate(templateKey) == Invalid_Template)
    {
        CT_SetRequestedTemplate(CT_GetBuiltinTemplate(playerIndex, channel));
        return PLUGIN_HANDLED;
    }

    // Захват решения.
    CT_SetRequestedTemplate(templateKey);
    return PLUGIN_HANDLED;
}
```

## Соответствующий конфиг VipModular (иллюстративно)

Привилегия, дающая VIP-шаблон с 3-го раунда:

```jsonc
{
    "Access": [
        {
            "Limit": "Flags",
            "Flags": "t"
        }
    ],
    "Modules": [
        {
            "Module": "ChatTemplaterSelector",
            "Limits": [
                {
                    "Limit": "Round",
                    "Min": 3
                }
            ],
            "Say": "vip_say",
            "SayTeam": "vip_sayteam"
        }
    ]
}
```

Не-VIP (флаг `t` отсутствует) не проходит `Access`, модуль не выдаётся,
`VipM_Modules_GetParams` вернёт пусто, и селектор вернёт `PLUGIN_CONTINUE`.

Нужен шаблон под состояние (мёртв/наблюдатель)? Состояние селектор определяет
сам: `is_user_alive(playerIndex)`, `get_user_team(playerIndex)` — форвард
передаёт только канал.

## На что смотреть при переносе

- Проверить реальные сигнатуры в `VipModular.inc` и `ParamsController.inc`.
- Проверить, как именно `VipM_Modules_GetParams` отдаёт значения (Trie или
  другой тип хендлера).
- Проверить константу режима проверки лимитов (`Limit_Exec_AND` и её соседи).
- Убедиться, что плагин грузится после ChatTemplater и VipModular.
- Следить, что решение захватывает только этот селектор.
