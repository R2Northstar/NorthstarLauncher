#include "../interface.h"
#include "ILogging.h"
#include "../../plugins.h"

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
		case LogLevel::INFO:
		default:
			spdLevel = spdlog::level::level_enum::info;
			break;
		}

		g_pPluginManager->GetPlugin(handle)->logger->log(spdLevel, msg);
	}
};

EXPOSE_SINGLE_INTERFACE(CLogging, ILogging, LOGGING_VERSION);
