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

static RequestForward = INVALID_HANDLE;
static RequestedKey[PARAM_SHORT_STRING_MAX_LEN];
static BuiltinSayKey[PARAM_SHORT_STRING_MAX_LEN];
static BuiltinSayTeamKey[PARAM_SHORT_STRING_MAX_LEN];

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

T_Template:CTSelector_Builtin(const playerIndex, const E_CTChannel:channel) {
    #pragma unused playerIndex

    new key[PARAM_SHORT_STRING_MAX_LEN];
    if (channel == CTChannel_Say) {
        copy(key, charsmax(key), BuiltinSayKey);
    } else {
        copy(key, charsmax(key), BuiltinSayTeamKey);
    }

    if (key[0] == EOS) {
        return Invalid_Template;
    }

    return CTRegistry_Find(key);
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

    PCSingle_ObjString(root, "Say", BuiltinSayKey, charsmax(BuiltinSayKey));
    PCSingle_ObjString(root, "SayTeam", BuiltinSayTeamKey, charsmax(BuiltinSayTeamKey));

    PCJson_Free(root);
}
