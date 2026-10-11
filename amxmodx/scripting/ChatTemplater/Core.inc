// ChatTemplater — core component.
//
// Hooks the SayText user message, sanitizes player input, asks the selector for
// a template and re-renders the whole line through ParamsController placeholders.

#if defined _ChatTemplater_Core_included
    #endinput
#endif
#define _ChatTemplater_Core_included

#include <amxmodx>
#include <ChatTemplater>

// Re-entrancy guard: rendering may emit further messages (e.g. from an external
// selector plugin), which must never be re-processed recursively.
static bool:ProcessingSayText = false;

CTCore_Init() {
    register_message(get_user_msgid("SayText"), "@CT_OnSayText");
}

@CT_OnSayText(msgId, msgDest, msgEntity) {
    #pragma unused msgId
    #pragma unused msgDest
    #pragma unused msgEntity

    if (ProcessingSayText) {
        return PLUGIN_CONTINUE;
    }

    // SayText args: 1 = sender, 2 = message name, 3 = name placeholder, 4 = text.
    if (get_msg_args() < 4) {
        return PLUGIN_CONTINUE;
    }

    new token[24];
    get_msg_arg_string(2, token, charsmax(token));

    new E_CTChannel:channel;
    if (!CTCore_ClassifyChannel(token, channel)) {
        return PLUGIN_CONTINUE;
    }

    new playerIndex = get_msg_arg_int(1);
    if (!is_user_connected(playerIndex)) {
        return PLUGIN_CONTINUE;
    }

    new text[PARAM_CHAT_MESSAGE_MAX_LEN + 1];
    get_msg_arg_string(4, text, charsmax(text));
    CTCore_SanitizeText(text, charsmax(text));

    new T_Template:template = CTSelector_Request(playerIndex, channel);
    if (template == Invalid_Template) {
        return PLUGIN_CONTINUE;
    }

    new T_PHTemplate:handler = CTRegistry_Handle(template);
    if (handler == Invalid_PHTemplate) {
        return PLUGIN_CONTINUE;
    }

    copy(PendingChatText, charsmax(PendingChatText), text);

    ProcessingSayText = true;

    new T_PHGroup:playerGroup = CTRegistry_PlayerGroup();
    if (playerGroup != Invalid_PHGroup) {
        PCPH_Group_PushIntContext(playerGroup, playerIndex);
    }

    new out[PARAM_VALUE_MAX_LEN];
    PCPH_FormatTemplate(handler, out, charsmax(out));

    if (playerGroup != Invalid_PHGroup) {
        PCPH_Group_PopContext(playerGroup);
    }

    ProcessingSayText = false;

    // Chat bodies are capped at PARAM_CHAT_MESSAGE_MAX_LEN cells.
    out[PARAM_CHAT_MESSAGE_MAX_LEN - 1] = EOS;

    // '#Spec_PlayerItem' renders a single colour-capable string: put the fully
    // rendered line in arg 3 and blank the template's other argument.
    set_msg_arg_string(2, "#Spec_PlayerItem");
    set_msg_arg_string(3, out);
    set_msg_arg_string(4, "");

    return PLUGIN_CONTINUE;
}

// Maps a SayText token to a chat channel. Returns false for non-chat tokens.
static bool:CTCore_ClassifyChannel(const token[], &E_CTChannel:channel) {
    if (
        equal(token, "#Cstrike_Chat_All")
        || equal(token, "#Cstrike_Chat_AllDead")
        || equal(token, "#Cstrike_Chat_AllSpec")
    ) {
        channel = CTChannel_Say;
        return true;
    }

    if (
        equal(token, "#Cstrike_Chat_T")
        || equal(token, "#Cstrike_Chat_T_Dead")
        || equal(token, "#Cstrike_Chat_CT")
        || equal(token, "#Cstrike_Chat_CT_Dead")
        || equal(token, "#Cstrike_Chat_Spec")
    ) {
        channel = CTChannel_SayTeam;
        return true;
    }

    return false;
}

// Removes forged colour control bytes (0x01..0x04) and '%' (potential
// format-string injection) from player-supplied chat text before rendering.
static CTCore_SanitizeText(text[], const len) {
    new writePos = 0;

    for (new readPos = 0; text[readPos] != EOS && writePos < len; ++readPos) {
        new ch = text[readPos];
        if ((ch >= 0x01 && ch <= 0x04) || ch == '%') {
            continue;
        }

        text[writePos++] = ch;
    }

    text[writePos] = EOS;
}
