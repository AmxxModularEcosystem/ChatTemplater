// ChatTemplater — registry natives.

#if defined _ChatTemplater_API_Registry_included
    #endinput
#endif
#define _ChatTemplater_API_Registry_included

#include <amxmodx>
#include <ChatTemplater>

API_Registry_Register() {
    register_native("CT_RegisterTemplate", "@CT_RegisterTemplate");
    register_native("CT_FindTemplate", "@CT_FindTemplate");
    register_native("CT_GetTemplateString", "@CT_GetTemplateString");
}

@CT_RegisterTemplate() {
    enum { Arg_Key = 1, Arg_Template }

    new key[PARAM_SHORT_STRING_MAX_LEN];
    get_string(Arg_Key, key, charsmax(key));

    new raw[PARAM_VALUE_MAX_LEN];
    get_string(Arg_Template, raw, charsmax(raw));

    return _:CTRegistry_Register(key, raw, Invalid_PHTemplate);
}

@CT_FindTemplate() {
    enum { Arg_Key = 1 }

    new key[PARAM_SHORT_STRING_MAX_LEN];
    get_string(Arg_Key, key, charsmax(key));

    return _:CTRegistry_Find(key);
}

bool:@CT_GetTemplateString() {
    enum { Arg_Template = 1, Arg_Out, Arg_Len }

    new T_Template:template = T_Template:get_param(Arg_Template);
    new len = get_param(Arg_Len);

    new out[PARAM_VALUE_MAX_LEN];
    new bool:found = CTRegistry_GetRaw(template, out, charsmax(out));

    set_string(Arg_Out, out, len);
    return found;
}
