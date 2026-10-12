// ChatTemplater — template selector: the built-in selector (Selector.json) and
// the CT_OnRequestTemplate dispatch used by external selectors.

#if defined _ChatTemplater_Selector_included
    #endinput
#endif
#define _ChatTemplater_Selector_included

#include <amxmodx>
#include <json>
#include <ChatTemplater>

new const SELECTOR_FILE[] = "/ChatTemplater/Selector.json";

// Built-in selector keys. Each is an optional entry of Selector.json; a missing
// entry falls back to the channel base key (see CTSelector_Builtin).
new const KEY_SAY[] = "Say";
new const KEY_SAY_DEAD[] = "SayDead";
new const KEY_SAY_SPEC[] = "SaySpec";
new const KEY_SAY_TEAM[] = "SayTeam";
new const KEY_SAY_TEAM_T[] = "SayTeamT";
new const KEY_SAY_TEAM_CT[] = "SayTeamCT";
new const KEY_SAY_TEAM_DEAD[] = "SayTeamDead";
new const KEY_SAY_TEAM_T_DEAD[] = "SayTeamTDead";
new const KEY_SAY_TEAM_CT_DEAD[] = "SayTeamCTDead";
new const KEY_SAY_TEAM_SPEC[] = "SayTeamSpec";

// Player state the built-in selector resolves on its own: only the chat channel
// is handed to it, everything player-related is looked up here.
enum E_CTPlayerState {
    CTState_Alive = 0,
    CTState_Dead,
    CTState_Spectator,
};

enum E_CTTeamSide {
    CTTeam_None = 0,
    CTTeam_Terrorist,
    CTTeam_CounterTerrorist,
};

static RequestForward = INVALID_HANDLE;
static RequestedKey[PARAM_SHORT_STRING_MAX_LEN];
static Trie:BuiltinKeys = Invalid_Trie;

CTSelector_Init() {
    if (RequestForward != INVALID_HANDLE) {
        return;
    }

    // ET_STOP + two cells. Handlers read the args and signal an override by
    // calling CT_SetRequestedTemplate(); the forward's return value is unused.
    RequestForward = CreateMultiForward("CT_OnRequestTemplate", ET_STOP, FP_CELL, FP_CELL);
}

T_Template:CTSelector_Request(const playerIndex, const E_CTChannel:channel) {
    RequestedKey[0] = EOS;

    if (RequestForward != INVALID_HANDLE) {
        new result;
        ExecuteForward(RequestForward, result, playerIndex, _:channel);
    }

    if (RequestedKey[0] != EOS) {
        return CTRegistry_Find(RequestedKey);
    }

    return CTSelector_Builtin(playerIndex, channel);
}

// Picks a template from Selector.json for the player's channel and state. The
// most specific configured key wins; a missing one falls back:
//   Say:     SayDead -> Say,  SaySpec -> Say
//   SayTeam: <side> -> <state> -> SayTeam -> the Say chain for the same state
// For a dead Terrorist that is: SayTeamTDead -> SayTeamDead -> SayTeam ->
// SayDead -> Say.
T_Template:CTSelector_Builtin(const playerIndex, const E_CTChannel:channel) {
    new E_CTPlayerState:playerState;
    new E_CTTeamSide:teamSide;
    CTSelector_ResolvePlayer(playerIndex, playerState, teamSide);

    if (channel == CTChannel_Say) {
        return CTSelector_BuiltinSay(playerState);
    }

    new T_Template:template = CTSelector_BuiltinSayTeam(playerState, teamSide);
    if (template == Invalid_Template) {
        // A server that configures only the Say side still styles every message.
        template = CTSelector_BuiltinSay(playerState);
    }

    return template;
}

static T_Template:CTSelector_BuiltinSay(const E_CTPlayerState:playerState) {
    new T_Template:template = Invalid_Template;

    if (playerState == CTState_Dead) {
        template = CTSelector_Lookup(KEY_SAY_DEAD);
    } else if (playerState == CTState_Spectator) {
        template = CTSelector_Lookup(KEY_SAY_SPEC);
    }

    if (template == Invalid_Template) {
        template = CTSelector_Lookup(KEY_SAY);
    }

    return template;
}

static T_Template:CTSelector_BuiltinSayTeam(const E_CTPlayerState:playerState, const E_CTTeamSide:teamSide) {
    new T_Template:template = Invalid_Template;

    if (playerState == CTState_Alive) {
        if (teamSide == CTTeam_Terrorist) {
            template = CTSelector_Lookup(KEY_SAY_TEAM_T);
        } else if (teamSide == CTTeam_CounterTerrorist) {
            template = CTSelector_Lookup(KEY_SAY_TEAM_CT);
        }
    } else if (playerState == CTState_Dead) {
        if (teamSide == CTTeam_Terrorist) {
            template = CTSelector_Lookup(KEY_SAY_TEAM_T_DEAD);
        } else if (teamSide == CTTeam_CounterTerrorist) {
            template = CTSelector_Lookup(KEY_SAY_TEAM_CT_DEAD);
        }

        if (template == Invalid_Template) {
            template = CTSelector_Lookup(KEY_SAY_TEAM_DEAD);
        }
    } else if (playerState == CTState_Spectator) {
        template = CTSelector_Lookup(KEY_SAY_TEAM_SPEC);
    }

    if (template == Invalid_Template) {
        template = CTSelector_Lookup(KEY_SAY_TEAM);
    }

    return template;
}

// Resolves the sender's state from the player itself, so the same logic serves
// both the message hook and CT_GetBuiltinTemplate (which has no message context).
static CTSelector_ResolvePlayer(const playerIndex, &E_CTPlayerState:playerState, &E_CTTeamSide:teamSide) {
    teamSide = CTSelector_TeamSide(playerIndex);

    if (teamSide == CTTeam_None) {
        playerState = CTState_Spectator;
    } else if (is_user_alive(playerIndex)) {
        playerState = CTState_Alive;
    } else {
        playerState = CTState_Dead;
    }
}

// CsTeams: 1 = Terrorist, 2 = Counter-Terrorist; 0 (unassigned) and 3
// (spectator) map to no side. get_user_team() returns -1 for an invalid index.
static E_CTTeamSide:CTSelector_TeamSide(const playerIndex) {
    switch (get_user_team(playerIndex)) {
        case 1: {
            return CTTeam_Terrorist;
        }
        case 2: {
            return CTTeam_CounterTerrorist;
        }
    }

    return CTTeam_None;
}

// Looks a built-in key up in Selector.json and resolves it to a template. An
// absent key or an unregistered template yields Invalid_Template so the caller
// can fall back to a broader key.
static T_Template:CTSelector_Lookup(const key[]) {
    if (BuiltinKeys == Invalid_Trie) {
        return Invalid_Template;
    }

    new templateKey[PARAM_SHORT_STRING_MAX_LEN];
    if (!TrieGetString(BuiltinKeys, key, templateKey, charsmax(templateKey)) || templateKey[0] == EOS) {
        return Invalid_Template;
    }

    return CTRegistry_Find(templateKey);
}

CTSelector_SetRequestedKey(const key[]) {
    copy(RequestedKey, charsmax(RequestedKey), key);
}

CTSelector_Load() {
    new path[PLATFORM_MAX_PATH];
    PCPath_MakePath(SELECTOR_FILE, path, charsmax(path));

    if (!file_exists(path)) {
        log_amx("Selector file not found: %s", path);
        return;
    }

    new JSON:root = PCJson_ParseFile(path);

    if (!json_is_object(root)) {
        PCJson_LogForFile(root, "ERROR", "Selector file must contain a JSON object.");
        PCJson_Free(root);
        return;
    }

    if (BuiltinKeys == Invalid_Trie) {
        BuiltinKeys = TrieCreate();
    } else {
        TrieClear(BuiltinKeys);
    }

    CTSelector_ReadKey(root, KEY_SAY);
    CTSelector_ReadKey(root, KEY_SAY_DEAD);
    CTSelector_ReadKey(root, KEY_SAY_SPEC);
    CTSelector_ReadKey(root, KEY_SAY_TEAM);
    CTSelector_ReadKey(root, KEY_SAY_TEAM_T);
    CTSelector_ReadKey(root, KEY_SAY_TEAM_CT);
    CTSelector_ReadKey(root, KEY_SAY_TEAM_DEAD);
    CTSelector_ReadKey(root, KEY_SAY_TEAM_T_DEAD);
    CTSelector_ReadKey(root, KEY_SAY_TEAM_CT_DEAD);
    CTSelector_ReadKey(root, KEY_SAY_TEAM_SPEC);

    PCJson_Free(root);
}

static CTSelector_ReadKey(const JSON:root, const name[]) {
    new value[PARAM_SHORT_STRING_MAX_LEN];
    PCSingle_ObjString(root, name, value, charsmax(value));

    if (value[0] != EOS) {
        TrieSetString(BuiltinKeys, name, value);
    }
}
