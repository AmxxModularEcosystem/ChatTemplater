// ChatTemplater — plugin glue.

#include <amxmodx>
#include <ChatTemplater>

#include "ChatTemplater/Registry.inc"
#include "ChatTemplater/Selector.inc"
#include "ChatTemplater/Core.inc"
#include "ChatTemplater/API/General.inc"

public stock const PluginName[] = "ChatTemplater";
public stock const PluginVersion[] = "1.0.0";
public stock const PluginAuthor[] = "AmxxModularEcosystem";
public stock const PluginDescription[] = "Chat message templater built on ParamsController placeholders.";

public plugin_init() {
    // ParamsController_Init() synchronously fires its registration and init
    // forwards, which our ParamsController_*/PCPH_* handlers below service.
    ParamsController_Init();

    CTRegistry_Init();
    CTSelector_Init();
    CTCore_Init();
}

public plugin_natives() {
    API_Init();
}

public ParamsController_OnRegisterTypes() {
    CTRegistry_OnRegisterTypes();
}

public PCPH_OnRegisterGroups() {
    CTRegistry_OnRegisterGroups();
}

public ParamsController_OnInited() {
    CTRegistry_Load();
    CTSelector_Load();
}
