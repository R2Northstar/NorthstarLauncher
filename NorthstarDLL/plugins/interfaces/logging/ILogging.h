#ifndef ILOGGING_H
#define ILOGGING_H

#define LOGGING_VERSION "Logging001"

/*
 * Maybe this should just be log() since all others are derived from that
 **/
class ILogging
{
	public:
	virtual void log(int handle, spdlog::level::level_enum level, char* msg) = 0;
	virtual void trace(int handle, char* msg) = 0;
	virtual void debug(int handle, char* msg) = 0;
	virtual void info(int handle, char* msg) = 0;
	virtual void warn(int handle, char* msg) = 0;
	virtual void error(int handle, char* msg) = 0;
	virtual void critical(int handle, char* msg) = 0;
};

#endif
