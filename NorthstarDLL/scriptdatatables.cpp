#include "pch.h"
#include "scriptdatatables.h"
#include "squirrel.h"
#include "rpakfilesystem.h"
#include "convar.h"
#include "filesystem.h"
#include <iostream>
#include <sstream>
#include <map>
#include <fstream>

struct ColumnInfo
{
	char* name;
	int type;
	int offset;
};

struct DataTable
{
	int columnAmount;
	int rowAmount;
	ColumnInfo* columnInfo;
	long long data; // actually data pointer
	int rowInfo;
};

ConVar* Cvar_ns_prefer_datatable_from_disk;
void datatableReleaseHook(void*, int size);
const long long customDatatableTypeId = 0xFFFCFFFC12345678;
const long long vanillaDatatableTypeId = 0xFFF7FFF700000004;
void* (*getDataTableStructure)(HSquirrelVM* sqvm);
std::string DataTableToString(DataTable* datatable);

struct csvData
{
	char* name;
	std::vector<std::vector<char*>> dataPointers;
	char* fullData = 0;
	std::vector<char*> columnNames;
};

std::unordered_map<std::string, csvData*> cacheMap;

void StringToVector(char* string, float* vector)
{
	int length = 0;
	while (string[length])
	{
		if ((string[length] == '<') || (string[length] == '>'))
			string[length] = '\0';
		length++;
	}

	int startOfFloat = 1;
	int currentIndex = 1;
	while (string[currentIndex] && (string[currentIndex] != ','))
		currentIndex++;
	string[currentIndex] = '\0';
	vector[0] = std::stof(&string[startOfFloat]);
	startOfFloat = ++currentIndex;
	while (string[currentIndex] && (string[currentIndex] != ','))
		currentIndex++;
	string[currentIndex] = '\0';
	vector[1] = std::stof(&string[startOfFloat]);
	startOfFloat = ++currentIndex;
	while (string[currentIndex] && (string[currentIndex] != ','))
		currentIndex++;
	string[currentIndex] = '\0';
	vector[2] = std::stof(&string[startOfFloat]);
	startOfFloat = ++currentIndex;
}

template <ScriptContext context> SQRESULT GetDatatable(HSquirrelVM* sqvm)
{
	const char* assetName;
	g_pSquirrel<context>->getasset(sqvm, 2, &assetName);
	SQRESULT result = SQRESULT_ERROR;
	if (strncmp(assetName, "datatable/", 10) != 0)
	{
		spdlog::error("Asset \"{}\" doesn't start with \"datatable/\"", assetName);
	}
	else if ((!Cvar_ns_prefer_datatable_from_disk->GetBool()) && g_pPakLoadManager->LoadFile(assetName) )
	{
		//spdlog::info("Load Datatable {} from rpak", assetName);
		result = g_pSquirrel<context>->m_funcOriginals["GetDataTable"](sqvm);
	}
	else
	{
		char assetPath[250];
		snprintf(assetPath, 250, "scripts/%s", assetName);
		if (cacheMap.count(assetName))
		{
			//spdlog::info("Loaded custom Datatable {} from cache", assetName);
			csvData** dataPointer = (csvData**)g_pSquirrel<context>->createuserdata(sqvm, sizeof(csvData*));
			*dataPointer = cacheMap[assetName];
			g_pSquirrel<context>->setuserdatatypeid(sqvm, -1, customDatatableTypeId);
			// sqvm->_stack[sqvm->_top -1]._VAL.asUserdata->releaseHook = datatableReleaseHook;
			result = SQRESULT_NOTNULL;
		}
		else if ((*R2::g_pFilesystem)->m_vtable2->FileExists(&(*R2::g_pFilesystem)->m_vtable2, assetPath, "GAME"))
		{

			std::string dataFile = R2::ReadVPKFile(assetPath);
			if (dataFile.size() == 0)
				return SQRESULT_ERROR;
			char* csvFullData = (char*)malloc(dataFile.size());
			memcpy(csvFullData, dataFile.c_str(), dataFile.size());

			csvData* data = new csvData();
			data->fullData = csvFullData;

			std::vector<char*> currentLine;
			int startIndex = 0;
			if (csvFullData[0] == '\"')
			{
				currentLine.push_back(&csvFullData[1]);
				startIndex = 1;
				int i = 1;
				while (csvFullData[i] != '\"')
					i++;
				csvFullData[i] = '\0';
			}
			else
			{
				currentLine.push_back(csvFullData);
			}

			bool firstLine = true;
			for (int i = startIndex; i < dataFile.size(); i++)
			{
				if (csvFullData[i] == ',')
				{
					if (csvFullData[i + 1] == '\"')
					{

						currentLine.push_back(&csvFullData[i + 2]);
						csvFullData[i] = '\0';
						csvFullData[i + 1] = '\0';
						while (true)
						{
							if ((csvFullData[i] == '\n') || (csvFullData[i] == '\r'))
								return SQRESULT_ERROR;
							if (csvFullData[i] == '\"')
								break;
							i++;
						}
						csvFullData[i] = '\0';
					}
					else
					{
						currentLine.push_back(&csvFullData[i + 1]);
						csvFullData[i] = '\0';
					}
				}
				if ((csvFullData[i] == '\n') || (csvFullData[i] == '\r'))
				{
					csvFullData[i] = '\0';
					if ((csvFullData[i + 1] == '\n') || (csvFullData[i + 1] == '\r'))
					{
						i++;
						csvFullData[i] = '\0';
					}
					if (firstLine)
					{
						data->columnNames = currentLine;
						firstLine = false;
					}
					else
					{
						data->dataPointers.push_back(currentLine);
					}

					currentLine.clear();
					if (i + 1 >= dataFile.size())
						break;
					if (csvFullData[i + 1] == '\"')
					{

						currentLine.push_back(&csvFullData[i + 2]);
						csvFullData[i] = '\0';
						csvFullData[i + 1] = '\0';
						while (true)
						{
							if ((csvFullData[i] == '\n') || (csvFullData[i] == '\r'))
								return SQRESULT_ERROR;
							if (csvFullData[i] == '\"')
								break;
							i++;
						}
						csvFullData[i] = '\0';
					}
					else
					{
						currentLine.push_back(&csvFullData[i + 1]);
						csvFullData[i] = '\0';
					}
				}
			}
			if (currentLine.size() != 0)
			{
				if (firstLine)
				{
					data->columnNames = currentLine;
				}
				else
				{
					data->dataPointers.push_back(currentLine);
				}
			}
			data->name = (char*)malloc(256);

			strncpy(data->name, assetName, 256);
			csvData** dataPointer = (csvData**)g_pSquirrel<context>->createuserdata(sqvm, sizeof(csvData*));
			g_pSquirrel<context>->setuserdatatypeid(sqvm, -1, customDatatableTypeId);

			*dataPointer = data;
			// vm->_stack[vm->_top -1]._VAL.asUserdata->releaseHook = datatableReleaseHook;
			cacheMap[assetName] = data;
			//spdlog::info("Loaded custom Datatable from file at {} with pointer {}", assetPath, (void*)data);

			result = SQRESULT_NOTNULL;
		}
		else if (Cvar_ns_prefer_datatable_from_disk->GetBool()&&g_pPakLoadManager->LoadFile(assetName))
		{
			result = g_pSquirrel<context>->m_funcOriginals["GetDataTable"](sqvm);
		}
		else
		{
			spdlog::error("Datatable {} not found", assetPath);
		}
	}
	return result;
}

template <ScriptContext context> SQRESULT GetDatatabeColumnByName(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableColumnByName");
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableColumnByName"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	//spdlog::info("GetColumnByName form datatable with pointer {}",(void*)data);
	const char* searchName = g_pSquirrel<context>->getstring(sqvm, 2);
	int col = 0;
	for (auto colName : data->columnNames)
	{
		if (!strcmp(colName, searchName))
			break;

		col++;
	}
	if (col == data->columnNames.size())
		col = -1;
	//spdlog::info("Datatable CoulumnName {} in column {}", std::string(searchName), col);
	g_pSquirrel<context>->pushinteger(sqvm, col);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDatatabeRowCount(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableRowCount");
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{

		return g_pSquirrel<context>->m_funcOriginals["GetDatatableRowCount"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	g_pSquirrel<context>->pushinteger(sqvm, data->dataPointers.size());
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableString(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableString");
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableString"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	int row = g_pSquirrel<context>->getinteger(sqvm, 2);
	int col = g_pSquirrel<context>->getinteger(sqvm, 3);
	if (row >= data->dataPointers.size() || col >= data->dataPointers[row].size())
	{
		spdlog::info( "row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}

	g_pSquirrel<context>->pushstring(sqvm, data->dataPointers[row][col], -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableAsset(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableAsset");
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableAsset"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	int row = g_pSquirrel<context>->getinteger(sqvm, 2);
	int col = g_pSquirrel<context>->getinteger(sqvm, 3);
	if (row >= data->dataPointers.size() || col >= data->dataPointers[row].size())
	{
		spdlog::info( "row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}
	char* asset = data->dataPointers[row][col];
	g_pSquirrel<context>->pushasset(sqvm, asset, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableInt(HSquirrelVM* sqvm)
{
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableInt"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	int row = g_pSquirrel<context>->getinteger(sqvm, 2);
	int col = g_pSquirrel<context>->getinteger(sqvm, 3);
	if (row >= data->dataPointers.size() || col >= data->dataPointers[row].size())
	{
		spdlog::info( "row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}
	g_pSquirrel<context>->pushinteger(sqvm, std::stoi(data->dataPointers[row][col]));
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableFloat(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableFloat");
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableFloat"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	int row = g_pSquirrel<context>->getinteger(sqvm, 2);
	int col = g_pSquirrel<context>->getinteger(sqvm, 3);
	if (row >= data->dataPointers.size() || col >= data->dataPointers[row].size())
	{
		spdlog::info( "row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}
	g_pSquirrel<context>->pushfloat(sqvm, std::stof(data->dataPointers[row][col]));
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableBool(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableBool");
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableBool"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	int row = g_pSquirrel<context>->getinteger(sqvm, 2);
	int col = g_pSquirrel<context>->getinteger(sqvm, 3);
	if (row >= data->dataPointers.size() || col >= data->dataPointers[row].size())
	{
		spdlog::info( "row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}
	g_pSquirrel<context>->pushbool(sqvm, std::stoi(data->dataPointers[row][col]));
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableVector(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableVector");
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableVector"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	float vector[3];

	int row = g_pSquirrel<context>->getinteger(sqvm, 2);
	int col = g_pSquirrel<context>->getinteger(sqvm, 3);
	if (row >= data->dataPointers.size() || col >= data->dataPointers[row].size())
	{
		spdlog::info(
			"row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}

	StringToVector(data->dataPointers[row][col], vector);
	g_pSquirrel<context>->pushvector(sqvm, vector);

	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowMatchingStringValue(HSquirrelVM* sqvm)
{
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableRowMatchingStringValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = g_pSquirrel<context>->getinteger(sqvm, 2);
	const char* stringValue = g_pSquirrel<context>->getstring(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{
		if (!strcmp(data->dataPointers[i][col], stringValue))
		{
			g_pSquirrel<context>->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	g_pSquirrel<context>->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowMatchingAssetValue(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableRowMatchingAsset");
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableRowMatchingAssetValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = g_pSquirrel<context>->getinteger(sqvm, 2);
	const char* stringValue;
	g_pSquirrel<context>->getasset(sqvm, 3, &stringValue);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{
		if (!strcmp(data->dataPointers[i][col], stringValue))
		{
			g_pSquirrel<context>->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	g_pSquirrel<context>->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowMatchingFloatValue(HSquirrelVM* sqvm)
{
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableRowMatchingFloatValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = g_pSquirrel<context>->getinteger(sqvm, 2);
	float compareValue = g_pSquirrel<context>->getfloat(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{

		if (compareValue == std::stof(data->dataPointers[i][col]))
		{
			g_pSquirrel<context>->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	g_pSquirrel<context>->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowMatchingIntValue(HSquirrelVM* sqvm)
{
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableRowMatchingIntValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = g_pSquirrel<context>->getinteger(sqvm, 2);
	int compareValue = g_pSquirrel<context>->getinteger(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{

		if (compareValue == std::stoi(data->dataPointers[i][col]))
		{
			g_pSquirrel<context>->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	g_pSquirrel<context>->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowGreaterThanOrEqualToIntValue(HSquirrelVM* sqvm)
{
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableRowGreaterThanOrEqualToIntValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = g_pSquirrel<context>->getinteger(sqvm, 2);
	int compareValue = g_pSquirrel<context>->getinteger(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{

		if (compareValue >= std::stoi(data->dataPointers[i][col]))
		{
			g_pSquirrel<context>->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	g_pSquirrel<context>->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowLessThanOrEqualToIntValue(HSquirrelVM* sqvm)
{
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableRowLessThanOrEqualToIntValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = g_pSquirrel<context>->getinteger(sqvm, 2);
	int compareValue = g_pSquirrel<context>->getinteger(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{

		if (compareValue <= std::stoi(data->dataPointers[i][col]))
		{
			g_pSquirrel<context>->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	g_pSquirrel<context>->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowGreaterThanOrEqualToFloatValue(HSquirrelVM* sqvm)
{
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableRowGreaterThanOrEqualToFloatValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = g_pSquirrel<context>->getinteger(sqvm, 2);
	float compareValue = g_pSquirrel<context>->getfloat(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{

		if (compareValue >= std::stof(data->dataPointers[i][col]))
		{
			g_pSquirrel<context>->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	g_pSquirrel<context>->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowLessThanOrEqualToFloatValue(HSquirrelVM* sqvm)
{
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableRowLessThanOrEqualToFloatValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = g_pSquirrel<context>->getinteger(sqvm, 2);
	int compareValue = g_pSquirrel<context>->getfloat(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{

		if (compareValue <= std::stof(data->dataPointers[i][col]))
		{
			g_pSquirrel<context>->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	g_pSquirrel<context>->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowMatchingVectorValue(HSquirrelVM* sqvm)
{
	csvData** dataPointer;
	long long typeId;
	g_pSquirrel<context>->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return g_pSquirrel<context>->m_funcOriginals["GetDataTableRowMatchingVectorValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = g_pSquirrel<context>->getinteger(sqvm, 2);
	float* compareValue = g_pSquirrel<context>->getvector(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{
		float dataTableVector[3]; 
		StringToVector(data->dataPointers[i][col],dataTableVector);
		if ((dataTableVector[0] == compareValue[0]) && (dataTableVector[1] == compareValue[1]) && (dataTableVector[2] == compareValue[2]))
		{
			g_pSquirrel<context>->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	g_pSquirrel<context>->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT DumpDataTable(HSquirrelVM* sqvm)
{

	DataTable* datatable = (DataTable*)getDataTableStructure(sqvm);
	if (datatable == 0)
	{
		spdlog::info("datatable not loaded");
		g_pSquirrel<context>->pushinteger(sqvm, 1);
		return SQRESULT_NOTNULL;
	}
	//spdlog::info("Datatable size row = {} col = {}", datatable->rowAmount, datatable->columnAmount);
	// std::string header = std::string(datatable->columnInfo[0].name);

	spdlog::info(DataTableToString(datatable));

	return SQRESULT_NULL;
}

template <ScriptContext context> SQRESULT DumpDataTableToFile(HSquirrelVM* sqvm)
{
	DataTable* datatable = (DataTable*)getDataTableStructure(sqvm);
	if (datatable == 0)
	{
		spdlog::info("datatable not loaded");
		g_pSquirrel<context>->pushinteger(sqvm, 1);
		return SQRESULT_NOTNULL;
	}
	//spdlog::info("Datatable size row = {} col = {}", datatable->rowAmount, datatable->columnAmount);
	// std::string header = std::string(datatable->columnInfo[0].name);
	const char* pathName = g_pSquirrel<context>->getstring(sqvm, 2);
	std::ofstream ofs(pathName);
	std::string data = DataTableToString(datatable);
	ofs.write(data.c_str(), data.size());
	ofs.close();
	return SQRESULT_NULL;
}

std::string DataTableToString(DataTable* datatable)
{
	std::string line = std::string(datatable->columnInfo[0].name);
	for (int col = 1; col < datatable->columnAmount; col++)
	{
		ColumnInfo* colInfo = &datatable->columnInfo[col];
		line += "," + std::string(colInfo->name);
	}
	line += std::string("\n");
	for (int row = 0; row < datatable->rowAmount; row++)
	{

		bool seperator = false;
		for (int col = 0; col < datatable->columnAmount; col++)
		{
			if (seperator)
			{
				line += std::string(",");
			}
			seperator = true;
			ColumnInfo* colInfo = &datatable->columnInfo[col];
			switch (colInfo->type)
			{
			case 0:
			{
				bool input = *((bool*)(datatable->data + colInfo->offset + row * datatable->rowInfo));
				if (input)
				{
					line += std::string("1");
				}
				else
				{
					line += std::string("0");
				}
				break;
			}
			case 1:
			{
				int input = *((int*)(datatable->data + colInfo->offset + row * datatable->rowInfo));
				line += std::to_string(input);
				break;
			}
			case 2:
			{
				float input = *((float*)(datatable->data + colInfo->offset + row * datatable->rowInfo));
				line += std::to_string(input);
				break;
			}
			case 3:
			{
				float* input = ((float*)(datatable->data + colInfo->offset + row * datatable->rowInfo));
				char string[256];
				snprintf(string, 256, "\"<%f,%f,%f>\"", input[0], input[1], input[2]);
				line += std::string(string);
				break;
			}

			case 4:
			case 5:
			case 6:
			{
				char* string = *((char**)(datatable->data + colInfo->offset + row * datatable->rowInfo));
				line += "\"" + std::string(string) + "\"";
				break;
			}
			}
		}
		line += std::string("\n");
	}
	return line;
}

void datatableReleaseHook(void* d, int size)
{
	csvData* data = *(csvData**)d;
	free(data->fullData);
	delete data;
}


template <ScriptContext context> void RegisterDataTableFunctions() 
{
	g_pSquirrel<context>->AddFuncOverride("GetDataTable", GetDatatable<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDataTableColumnByName", GetDatatabeColumnByName<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDatatableRowCount", GetDatatabeRowCount<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDataTableString", GetDataTableString<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDataTableInt", GetDataTableInt<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDataTableFloat", GetDataTableFloat<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDataTableBool", GetDataTableBool<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDataTableAsset", GetDataTableAsset<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDataTableVector", GetDataTableVector<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDataTableRowMatchingStringValue", GetDataTableRowMatchingStringValue<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDataTableRowMatchingAssetValue", GetDataTableRowMatchingAssetValue<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDataTableRowMatchingFloatValue", GetDataTableRowMatchingFloatValue<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDataTableRowMatchingIntValue", GetDataTableRowMatchingIntValue<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride("GetDataTableRowMatchingVectorValue", GetDataTableRowMatchingVectorValue<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride(
		"GetDataTableRowLessThanOrEqualToFloatValue", GetDataTableRowLessThanOrEqualToFloatValue<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride(
		"GetDataTableRowGreaterThanOrEqualToFloatValue", GetDataTableRowGreaterThanOrEqualToFloatValue<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride(
		"GetDataTableRowLessThanOrEqualToIntValue", GetDataTableRowLessThanOrEqualToIntValue<ScriptContext::SERVER>);
	g_pSquirrel<context>->AddFuncOverride(
		"GetDataTableRowGreaterThanOrEqualToFloatValue", GetDataTableRowGreaterThanOrEqualToIntValue<ScriptContext::SERVER>);


}

ON_DLL_LOAD_RELIESON("server.dll", ServerScriptDatatables, ServerSquirrel, (CModule module))
{
	RegisterDataTableFunctions<ScriptContext::SERVER>();
	g_pSquirrel<ScriptContext::SERVER>->AddFuncRegistration(
		"void", "DumpDataTable", "var", "Dumps rpak datatable contents to console", DumpDataTable<ScriptContext::SERVER>);
	// g_pSquirrel<ScriptContext::SERVER>->AddFuncRegistration( "void", "DumpDataTableToFile", "var,string", "Dumps datatable contents to console",
	// DumpDataTableToFile<ScriptContext::SERVER>);



	getDataTableStructure = module.Offset(0x1250f0).As<void* (*)(HSquirrelVM*)>();
}


ON_DLL_LOAD_RELIESON("client.dll", ClientScriptDatatables, ClientSquirrrel, (CModule module)) 
{

	RegisterDataTableFunctions<ScriptContext::CLIENT>();
	RegisterDataTableFunctions<ScriptContext::UI>();
}

ON_DLL_LOAD_RELIESON("engine.dll", GeneralScriptDataTables, ConCommand, (CModule module)) 
{
	Cvar_ns_prefer_datatable_from_disk =
		new ConVar("ns_prefer_datatable_from_disk", "0", FCVAR_NONE, "whether datatables from disk overwrite rpak datatables");
}