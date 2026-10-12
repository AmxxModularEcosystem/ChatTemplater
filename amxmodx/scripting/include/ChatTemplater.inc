// ChatTemplater — public API.
//
// A chat message templater: every SayText message is re-rendered through a
// per-player placeholder template looked up in the template registry. The
// registry (key -> template) and the selector (which key a player uses for
// Say/SayTeam) both live in the core plugin; a separate plugin may replace the
// selector through the CT_OnRequestTemplate forward.

#if defined _ChatTemplater_included
    #endinput
#endif
#define _ChatTemplater_included

#include <amxmodx>
#include <ParamsController>

// Chat channels the selector can answer for.
enum E_CTChannel {
    CTChannel_Say = 0,
    CTChannel_SayTeam,
};

// Opaque handle of a registered template (0 = invalid).
enum T_Template {
    Invalid_Template = 0,
};

/**
 * Registers a template under a key.
 *
 * @param key       Unique template key (up to PARAM_SHORT_STRING_MAX_LEN cells).
 * @param template  Raw template string with placeholders, e.g. "{p:name}: {chat:text}".
 *
 * @note  The template is compiled into a placeholder template, so this should be
 *        called after ParamsController has registered the placeholder groups
 *        (i.e. from ParamsController_OnInited or later).
 *
 * @return  Handle of the registered template, or Invalid_Template on failure.
 */
native T_Template:CT_RegisterTemplate(const key[], const template[]);

/**
 * Looks a template up by key.
 *
 * @param key  Template key.
 *
 * @return  Handle of the template, or Invalid_Template if not registered.
 */
native T_Template:CT_FindTemplate(const key[]);

/**
 * Copies the raw template string of a registered template into a buffer.
 *
 * @param template  Template handle.
 * @param out       Output buffer.
 * @param len       Size of the output buffer.
 *
 * @return  true on success, false if the handle is invalid.
 */
native bool:CT_GetTemplateString(const T_Template:template, out[], const len);

/**
 * Called for every chat message before it is rendered. A plugin may override
 * the template used for this message by calling CT_SetRequestedTemplate().
 *
 * @note  Only the chat channel is passed, because the channel is a property of
 *        the command (say vs say_team), not of the player. Anything else about
 *        the sender — alive/dead/spectator, team — is the selector's own
 *        business: resolve it from playerIndex if you need it.
 *
 * @param playerIndex  Player whose message is being rendered.
 * @param channel      Chat channel (Say or SayTeam).
 *
 * @noreturn
 */
forward CT_OnRequestTemplate(const playerIndex, const E_CTChannel:channel);

/**
 * Sets the template key to use for the message currently being processed.
 *
 * @note  Only meaningful inside the CT_OnRequestTemplate forward. An empty key
 *        (the default) makes the core fall back to the built-in selector.
 *
 * @param key  Template key.
 *
 * @noreturn
 */
native CT_SetRequestedTemplate(const key[]);

/**
 * Returns the template the built-in selector would use for a player/channel.
 *
 * @note  The built-in selector resolves the sender's state (alive/dead/
 *        spectator) and team side from playerIndex itself, so this is the
 *        built-in answer for the player's current state. With no message
 *        context (e.g. a direct call) it still works, using the player's state
 *        as it is right now.
 *
 * @param playerIndex  Player index.
 * @param channel      Chat channel.
 *
 * @return  Template handle, or Invalid_Template if none is configured.
 */
native T_Template:CT_GetBuiltinTemplate(const playerIndex, const E_CTChannel:channel);
