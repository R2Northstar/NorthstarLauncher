#include "plugins/interfaces/interface.h"
#include "ILogging.h"
#include "plugins/plugins.h"

class CLogging : public ILogging
{
  public:
	void log(int handle, LogLevel level, char* msg)
	{
		spdlog::level::level_enum spdLevel;

		switch (level)
		{
		case LogLevel::WARN:
			spdLevel = spdlog::level::level_enum::warn;
			break;
		case LogLevel::ERR:
			spdLevel = spdlog::level::level_enum::err;
			break;
		default:
			NS::log::PLUGINSYS->warn("Attempted to log with invalid level {}. Defaulting to info", (int)level);
		case LogLevel::INFO:
			spdLevel = spdlog::level::level_enum::info;
			break;
		}

		std::optional<Plugin*> plugin = g_pPluginManager->GetPlugin(handle);
		if (plugin)
		{
			(*plugin)->Log(spdLevel, msg);
		}
		else
		{
			NS::log::PLUGINSYS->warn("Attempted to log message '{}' with invalid plugin handle {}", msg, handle);
		}
	}
};

EXPOSE_SINGLE_INTERFACE(CLogging, ILogging, LOGGING_VERSION);
