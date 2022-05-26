#include "pch.h"

#include <fstream>

#include "NSQIO.h"
#include "NSQApi.h"
#include <rapidjson/fwd.h>
#include <rapidjson/writer.h>
#include "configurables.h"

#if 0 // Old code
const static std::pair<std::string, std::string> NSQField_StrReplacements[] = {
	{"\\n", "\n"},
	{"\\r", "\r"},
};

void StrReplaceAll(std::string& str, const std::string& from, const std::string& to)
{
	if (from.empty())
		return;
	size_t curStartPos = 0;
	while ((curStartPos = str.find(from, curStartPos)) != std::string::npos)
	{
		str.replace(curStartPos, from.length(), to);
		curStartPos += to.length();
	}
}

bool NSQField::ReadFromLine(std::string line)
{
	std::string parts[3]{};
	for (int i = 0; i < 2; i++)
	{
		size_t nextDelimPos = line.find(FIELD_DELIM);
		if (nextDelimPos == std::string::npos)
		{
			return false; // Not enough parts
		}
		parts[i] = line.substr(0, nextDelimPos);
		if (parts[i].empty())
			return false; // No parts should be empty, except value (can hold empty strings)

		line = line.erase(0, nextDelimPos + 1);
	}
	parts[2] = line; // Last part (value) can include delimiters, we don't care

	auto type = (NSQFieldType)(parts[0][0] - '0');
	if (type < 0 || type >= NSQFIELD_COUNT)
		return false; // Invalid type

	NSQField out;
	out.type = type;
	out.name = parts[1];

	{ // Parse value
		std::string valStr = parts[2];
		if (type == NSQFIELD_STRING)
		{
			// Do restore replacements
			for (auto& pair : NSQField_StrReplacements)
			{
				StrReplaceAll(valStr, pair.first, pair.second);
			}

			out.val_str = valStr;
		}
		else if (type == NSQFIELD_INT || type == NSQFIELD_FLOAT)
		{
			bool isFloat = type == NSQFIELD_FLOAT;
			try
			{
				if (isFloat)
				{
					out.val_float = std::stod(valStr);
				}
				else
				{
					out.val_int = std::stoll(valStr);
				}
			}
			catch (std::exception& e)
			{
				return false; // Invalid number value
			}
		}
		else
		{
			assert(false); // Unimplemented type
		}
	}

	*this = out;
	return true;
}

std::string NSQField::WriteToLine()
{
	std::stringstream out;
	out << type << FIELD_DELIM << name << FIELD_DELIM;

	switch (type)
	{
	case NSQFIELD_INVALID:
		return "";
	case NSQFIELD_INT:
		out << val_int;
		break;
	case NSQFIELD_FLOAT:
		out << val_float;
		break;
	case NSQFIELD_STRING:
	{

		std::string writeOut = val_str;
		for (auto& pair : NSQField_StrReplacements)
			StrReplaceAll(writeOut, pair.second, pair.first);
		out << writeOut;
	}
		break;
	default : assert(false); // Unimplemented
	}

	return out.str();
}

void NSQFile::Save(std::string fileName) {}
bool NSQFile::Load(std::string fileName) {}
#endif

bool IsJSONPathBad(std::string namePath)
{
	if (namePath.empty())
		return true;

	if (namePath.find("..") != std::string::npos)
		return true; // No escaping the folder!

	return false;
}

rapidjson_document NSQIO::LoadJSON(std::string namePath)
{
	DBLOG("NSQIO::LoadJSON \"{}\"", namePath);

	if (IsJSONPathBad(namePath))
		return rapidjson_document();

	rapidjson_document out;
	out.SetObject();

	std::string path = GetNorthstarPrefix() + "/" + NSQIO_DIRECTORY + "/" + namePath + ".json";
	std::ifstream inStream = std::ifstream(path);

	DBLOG("\tPath: {}", path);

	if (!inStream.good())
	{
		DBLOG("\tFailed to open file stream.");
		return out;
	}

	std::stringstream stream;
	stream << inStream.rdbuf();

	out.Parse(stream.str().c_str());

	DBLOG("Loaded JSON from {}, member count: {}", path, out.MemberCount());
	return out;
}

bool NSQIO::SaveJSON(std::string namePath, const rapidjson_document& document)
{
	DBLOG("NSQIO::SaveJSON \"{}\"", namePath);

	if (IsJSONPathBad(namePath))
		return false;

	std::string basePath = GetNorthstarPrefix() + "/" + NSQIO_DIRECTORY;
	if (!std::filesystem::exists(basePath))
	{
		try
		{
			std::filesystem::create_directories(basePath);
		}
		catch (std::exception& e)
		{
			DBLOG("\tFailed to create directory \"{}\": {}", basePath, e.what());
			return false;
		}
	}

	std::string path = basePath + "/" + namePath + ".json";
	DBLOG("\tPath: {}", path);

	std::ofstream outStream = std::ofstream(path);

	if (!outStream.good())
	{
		DBLOG("\tFailed to write, bad stream.");
		return false;
	}

	rapidjson::StringBuffer s;
	rapidjson::Writer<rapidjson::StringBuffer> writer(s);
	if (document.Accept(writer))
	{
		outStream << s.GetString();
		return true;
	}
	else
	{
		DBLOG("\tFailed to write, document won't accept writer.");
		return false;
	}
}