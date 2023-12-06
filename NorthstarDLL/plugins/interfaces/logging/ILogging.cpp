#include "../interface.h"
#include "ILogging.h"
#include "../../plugins.h"

class CLogging : public ILogging
{
	public:
		void log(int handle, spdlog::level::level_enum level, char* msg)
		{
			NS::log::PLUGINSYS->info("attempting to log msg '{}'", msg);
			g_pPluginManager->GetPlugin(handle)->logger->log(level, msg);
		}

		void trace(int handle, char* msg)
		{
			g_pPluginManager->GetPlugin(handle)->logger->trace(msg);
		}

		void debug(int handle, char* msg)
		{
			g_pPluginManager->GetPlugin(handle)->logger->debug(msg);
		}

		void info(int handle, char* msg)
		{
			g_pPluginManager->GetPlugin(handle)->logger->info(msg);
		}

		void warn(int handle, char* msg)
		{
			g_pPluginManager->GetPlugin(handle)->logger->warn(msg);
		}

		void error(int handle, char* msg)
		{
			g_pPluginManager->GetPlugin(handle)->logger->error(msg);
		}

		void critical(int handle, char* msg)
		{
			g_pPluginManager->GetPlugin(handle)->logger->critical(msg);
		}
};

static_assert(sizeof(CLogging) == 8);

EXPOSE_SINGLE_INTERFACE(CLogging, ILogging, LOGGING_VERSION);
