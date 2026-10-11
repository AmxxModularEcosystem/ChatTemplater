// ChatTemplater — template registry: key -> compiled template, config loader,
// custom "ChatTemplate" parameter type and the {chat:text} placeholder group.

#if defined _ChatTemplater_Registry_included
    #endinput
#endif
#define _ChatTemplater_Registry_included

#include <amxmodx>
#include <json>
#include <ChatTemplater>

new const CHAT_TEMPLATE_TYPE_NAME[] = "ChatTemplate";
new const TEMPLATES_DIR[] = "/ChatTemplater/Templates";

// Current (already sanitized) chat message, read by the {chat:text} callback.
new PendingChatText[PARAM_CHAT_MESSAGE_MAX_LEN];

static Trie:TemplateKeys = Invalid_Trie;       // key -> T_Template (1-based)
static Array:TemplateRaw = Invalid_Array;      // slot -> raw template string
static Array:TemplateHandles = Invalid_Array;  // slot -> T_PHTemplate

static T_PHGroup:ChatGroup = Invalid_PHGroup;
static T_PHGroup:PlayerGroup = Invalid_PHGroup;

static bool:GroupsRegistered = false;

CTRegistry_Init() {
    CTRegistry_EnsureStorage();
}

static CTRegistry_EnsureStorage() {
    if (TemplateKeys != Invalid_Trie) {
        return;
    }

    TemplateKeys = TrieCreate();
    TemplateRaw = ArrayCreate(PARAM_VALUE_MAX_LEN, 1);
    TemplateHandles = ArrayCreate(1, 1);
}

CTRegistry_OnRegisterTypes() {
    ParamsController_RegSimpleType(CHAT_TEMPLATE_TYPE_NAME, "@CT_OnReadChatTemplate");
}

CTRegistry_OnRegisterGroups() {
    CTRegistry_EnsureStorage();

    ChatGroup = PCPH_RegisterGroup("chat");
    PCPH_Group_RegKey(ChatGroup, "text", "@CT_OnChatText");

    // The player group is registered by ParamsController itself; reuse it so
    // {p:name}, {p:team-name}, ... resolve against the context we push.
    PlayerGroup = PCPH_FindGroup(DEFAULT_PH_PLAYER_GROUP_PREFIX);

    GroupsRegistered = true;
}

// Turns a raw template string into a compiled placeholder template handle.
// Configs are loaded from ParamsController_OnInited, once every plugin has
// registered its placeholder groups/keys.
bool:@CT_OnReadChatTemplate(const JSON:jValue, const Trie:tParams, const key[], const tag[]) {
    #pragma unused tParams
    #pragma unused key
    #pragma unused tag

    new buffer[PARAM_VALUE_MAX_LEN];
    PCSingle_String(jValue, buffer, charsmax(buffer));

    // In Pawn source '^' is the escape character, so the two-character search
    // strings are written "^^1"/"^^3"/"^^4" while the replacements "^1"/"^3"/"^4"
    // are the single colour control bytes 0x01/0x03/0x04.
    replace_all(buffer, charsmax(buffer), "^^1", "^1");
    replace_all(buffer, charsmax(buffer), "^^3", "^3");
    replace_all(buffer, charsmax(buffer), "^^4", "^4");

    return ParamsController_SetCell(_:PCPH_CompileTemplate(buffer));
}

@CT_OnChatText(const key[], const contextKey[]) {
    #pragma unused key
    #pragma unused contextKey

    PCPH_Cb_Set(PendingChatText);
}

CTRegistry_Load() {
    // Templates registered through the native before the placeholder groups were
    // ready are compiled now that everything has been registered.
    CTRegistry_FinalizePending();

    new dirPath[PLATFORM_MAX_PATH];
    PCPath_MakePath(TEMPLATES_DIR, dirPath, charsmax(dirPath));

    if (!dir_exists(dirPath)) {
        log_amx("Templates directory not found: %s", dirPath);
        return;
    }

    new FileType:fileType;
    new fileName[PLATFORM_MAX_PATH];
    new dirHandle = open_dir(dirPath, fileName, charsmax(fileName), fileType);
    if (!dirHandle) {
        log_amx("Failed to open templates directory: %s", dirPath);
        return;
    }

    do {
        if (fileType == FileType_File && CTRegistry_IsJsonFileName(fileName)) {
            new filePath[PLATFORM_MAX_PATH];
            formatex(filePath, charsmax(filePath), "%s/%s", dirPath, fileName);
            CTRegistry_LoadFile(filePath);
        }
    } while (next_file(dirHandle, fileName, charsmax(fileName), fileType));

    close_dir(dirHandle);
}

static bool:CTRegistry_IsJsonFileName(const name[]) {
    new len = strlen(name);
    return (len > 5) && equali(name[len - 5], ".json");
}

static CTRegistry_FinalizePending() {
    for (new slot = 0, count = ArraySize(TemplateHandles); slot < count; ++slot) {
        if (T_PHTemplate:ArrayGetCell(TemplateHandles, slot) != Invalid_PHTemplate) {
            continue;
        }

        new raw[PARAM_VALUE_MAX_LEN];
        ArrayGetString(TemplateRaw, slot, raw, charsmax(raw));

        if (raw[0] != EOS) {
            ArraySetCell(TemplateHandles, slot, _:PCPH_CompileTemplate(raw));
        }
    }
}

static CTRegistry_LoadFile(const path[]) {
    new JSON:root = PCJson_ParseFile(path);

    if (!json_is_array(root)) {
        PCJson_LogForFile(root, "ERROR", "Template file must contain a JSON array.");
        PCJson_Free(root);
        return;
    }

    for (new i = 0, count = json_array_get_count(root); i < count; ++i) {
        new JSON:element = json_array_get_value(root, i);

        if (!json_is_object(element)) {
            PCJson_LogForFile(element, "ERROR", "Template entry #%d must be a JSON object.", i);
            json_free(element);
            continue;
        }

        CTRegistry_LoadEntry(element);
        json_free(element);
    }

    PCJson_Free(root);
}

static CTRegistry_LoadEntry(const JSON:element) {
    new key[PARAM_SHORT_STRING_MAX_LEN];
    PCSingle_ObjString(element, "Key", key, charsmax(key));

    new raw[PARAM_VALUE_MAX_LEN];
    PCSingle_ObjString(element, "Template", raw, charsmax(raw));

    if (key[0] == EOS || raw[0] == EOS) {
        PCJson_LogForFile(element, "ERROR", "Template entry must have non-empty 'Key' and 'Template' fields.");
        return;
    }

    new T_PHTemplate:handler = T_PHTemplate:PCSingle_ObjCell(element, "Template", CHAT_TEMPLATE_TYPE_NAME);
    CTRegistry_Register(key, raw, handler);
}

T_Template:CTRegistry_Register(const key[], const raw[], T_PHTemplate:handler) {
    CTRegistry_EnsureStorage();

    if (key[0] == EOS) {
        log_amx("Cannot register a template with an empty key.");
        return Invalid_Template;
    }

    // Compile late if the caller could not (registered before the PH groups
    // were ready); otherwise CTRegistry_FinalizePending() picks it up later.
    if (handler == Invalid_PHTemplate && raw[0] != EOS && GroupsRegistered) {
        handler = PCPH_CompileTemplate(raw);
    }

    new existing;
    if (TrieGetCell(TemplateKeys, key, existing)) {
        new slot = existing - 1;
        new T_PHTemplate:previous = T_PHTemplate:ArrayGetCell(TemplateHandles, slot);
        if (previous != Invalid_PHTemplate) {
            PCPH_DestroyTemplate(previous);
        }

        ArraySetString(TemplateRaw, slot, raw);
        ArraySetCell(TemplateHandles, slot, _:handler);
        return T_Template:existing;
    }

    new slot = ArrayPushString(TemplateRaw, raw);
    ArrayPushCell(TemplateHandles, _:handler);
    TrieSetCell(TemplateKeys, key, slot + 1);

    return T_Template:(slot + 1);
}

T_Template:CTRegistry_Find(const key[]) {
    if (TemplateKeys == Invalid_Trie) {
        return Invalid_Template;
    }

    new slot;
    if (!TrieGetCell(TemplateKeys, key, slot)) {
        return Invalid_Template;
    }

    return T_Template:slot;
}

T_PHTemplate:CTRegistry_Handle(const T_Template:template) {
    if (TemplateHandles == Invalid_Array || template == Invalid_Template) {
        return Invalid_PHTemplate;
    }

    return T_PHTemplate:ArrayGetCell(TemplateHandles, _:template - 1);
}

bool:CTRegistry_GetRaw(const T_Template:template, out[], const outLen) {
    if (TemplateRaw == Invalid_Array || template == Invalid_Template) {
        out[0] = EOS;
        return false;
    }

    ArrayGetString(TemplateRaw, _:template - 1, out, outLen);
    return true;
}

T_PHGroup:CTRegistry_PlayerGroup() {
    return PlayerGroup;
}
