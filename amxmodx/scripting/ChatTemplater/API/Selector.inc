// ChatTemplater — selector natives.

#if defined _ChatTemplater_API_Selector_included
    #endinput
#endif
#define _ChatTemplater_API_Selector_included

#include <amxmodx>
#include <ChatTemplater>

API_Selector_Register() {
    register_native("CT_SetRequestedTemplate", "@CT_SetRequestedTemplate");
    register_native("CT_GetBuiltinTemplate", "@CT_GetBuiltinTemplate");
}

@CT_SetRequestedTemplate() {
    enum { Arg_Key = 1 }

    new key[PARAM_SHORT_STRING_MAX_LEN];
    get_string(Arg_Key, key, charsmax(key));

    CTSelector_SetRequestedKey(key);
}

@CT_GetBuiltinTemplate() {
    enum { Arg_PlayerIndex = 1, Arg_Channel }

    new playerIndex = get_param(Arg_PlayerIndex);
    new channel = get_param(Arg_Channel);

    return _:CTSelector_Builtin(playerIndex, E_CTChannel:channel);
}
