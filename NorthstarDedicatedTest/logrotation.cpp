#include "pch.h"
#include <filesystem>
#include <string>
#include <time.h>
#include <iostream>
#include <regex>
#include "configurables.h"

namespace fs = std::filesystem;
using namespace std;

const int seconds_in_day = 86400;

double getLogAge(const fs::path path)
{
	time_t logtime;
	time_t currenttime;
	struct tm timeinfo = {0};
	int year, month, day;
	string filename = path.filename().string();
	// Check if log has a correct name
	if (regex_search(filename, regex("nslog([0-9]{2}-){2}[0-9]{4}\ [0-9]{2}(-[0-9]{2}){2}.txt")))
	{
		// Log naming before 1.4.0, which released on January 06 2022
		year = stoi(filename.substr(11, 4));
		month = stoi(filename.substr(8, 2));
		day = stoi(filename.substr(5, 2));
	}
	else if (regex_search(filename, regex("nsdump([0-9]{2}-){2}[0-9]{4}\ [0-9]{2}(-[0-9]{2}){2}.dmp")))
	{
		// Dumps
		year = stoi(filename.substr(12, 4));
		month = stoi(filename.substr(9, 2));
		day = stoi(filename.substr(6, 2));
	}
	else if (regex_search(filename, regex("nslog[0-9]{4}(-[0-9]{2}){2}\ [0-9]{2}(-[0-9]{2}){2}.txt")))
	{
		// Log naming after 1.4.0
		year = stoi(filename.substr(5, 4));
		month = stoi(filename.substr(10, 2));
		day = stoi(filename.substr(13, 2));
	}
	else
	{
		// Can't get log date
		return 0;
	}

	// Only reads days
	timeinfo.tm_year = year - 1900;
	timeinfo.tm_mon = month - 1;
	timeinfo.tm_mday = day;

	logtime = mktime(&timeinfo);
	time(&currenttime);
	return difftime(currenttime, logtime);
}

void logRotation(int days = 7)
{
	string path = GetNorthstarPrefix() + "/logs";
	struct stat info;
	// Check if we can access log directory
	if (stat(path.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR))
	{
		return;
	}
	for (const auto& entry : fs::directory_iterator(path))
	{
		fs::path link = entry.path();
		string extension = link.extension().string();
		if (extension == ".txt" || extension == ".dmp")
		{
			if (getLogAge(link) > seconds_in_day * days)
			{
				remove(link);
			}
		}
	}
}