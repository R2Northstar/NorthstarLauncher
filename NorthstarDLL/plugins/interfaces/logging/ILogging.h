#ifndef ILOGGING_H
#define ILOGGING_H

#define LOGGING_VERSION "NSLogging001"

enum class LogLevel : int
{
	INFO = 0,
	WARN,
	ERR,
};

class ILogging
{
  public:
	virtual void log(int handle, LogLevel level, char* msg) = 0;
};

#endif
