#include <string>
#include "pch.h"
#include "configurables.h"

std::string GetNorthstarPrefix() { return NORTHSTAR_FOLDER_PREFIX; }

void ParseConfigurables()
{
	char* clachar = strstr(GetCommandLineA(), "-profile=");
	if (clachar)
	{
		std::string cla = clachar;
		if (strncmp(cla.substr(9, 1).c_str(), "\"", 1))
		{
			int space = cla.find(" ");
			std::string dirname = cla.substr(9, space - 9);
			NORTHSTAR_FOLDER_PREFIX = dirname;
		}
		else
		{
			std::string quote = "\"";
			int quote1 = cla.find(quote);
			int quote2 = (cla.substr(quote1 + 1)).find(quote);
			std::string dirname = cla.substr(quote1 + 1, quote2);
			NORTHSTAR_FOLDER_PREFIX = dirname;
		}
	}
	else
	{
		NORTHSTAR_FOLDER_PREFIX = "R2Northstar";
	}
}
