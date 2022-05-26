#pragma once
#include "pch.h"
#include <rapidjson/rapidjson.h>

#define NSQIO_DIRECTORY "NSQIO/data"

/*
enum NSQFieldType
{
	NSQFIELD_INVALID = -1,

	NSQFIELD_INT,
	NSQFIELD_FLOAT,
	NSQFIELD_STRING,

	NSQFIELD_COUNT
};

struct NSQField
{
	// FORMAT: type=name=val
	const static char FIELD_DELIM = '='; // Seperates fields in a NSQField

	NSQFieldType type = NSQFIELD_INVALID;
	std::string name;

	std::string val_str;
	INT64 val_int;
	double val_float;

	bool IsValid() { return type != NSQFIELD_INVALID; }
	bool ReadFromLine(std::string line);
	std::string WriteToLine();
};

struct NSQFile
{
	std::unordered_map<std::string, NSQField> fields;

	bool HasField(std::string name) { return fields.find(name) != fields.end(); }

	NSQField Get(std::string name) {
		auto itr = fields.find(name);
		return (itr == fields.end()) ? NSQField() : itr->second;
	}

	bool Set(NSQField field) {
		fields.insert({field.name, field});
		return field.IsValid();
	}

	bool Remove(std::string name) {
		auto itr = fields.find(name);
		if (itr == fields.end())
		{
			return false;
		}
		else
		{
			fields.erase(itr);
			return true;
		}
	}

	void Save(std::string fileName);
	bool Load(std::string fileName);
};
*/

namespace NSQIO
{
	rapidjson_document LoadJSON(std::string namePath);

	// Returns false if invalid path
	bool SaveJSON(std::string namePath, const rapidjson_document& document);
} // namespace NSQIO