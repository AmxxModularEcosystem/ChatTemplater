// ChatTemplater — native registration entry point (called from plugin_natives).

#if defined _ChatTemplater_API_General_included
    #endinput
#endif
#define _ChatTemplater_API_General_included

#include <amxmodx>
#include <ChatTemplater>

#include "ChatTemplater/API/Registry"
#include "ChatTemplater/API/Selector"

API_Init() {
    API_Registry_Register();
    API_Selector_Register();
}
