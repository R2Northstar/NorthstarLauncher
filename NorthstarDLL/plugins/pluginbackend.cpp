#include "pluginbackend.h"
#include "plugin_abi.h"
#include "server/serverpresence.h"
#include "masterserver/masterserver.h"
#include "squirrel/squirrel.h"
#include "plugins.h"
#include "pluginmanager.h"

#include "core/convar/concommand.h"

#include <filesystem>

AUTOHOOK_INIT()

PluginCommunicationHandler* g_pPluginCommunicationhandler;

static PluginDataRequest storedRequest {PluginDataRequestType::END, (PluginRespondDataCallable) nullptr};

