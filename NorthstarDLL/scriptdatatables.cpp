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

ConVar* Cvar_ns_prefere_datatable_from_disk;
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
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	sqManager->getasset(sqvm, 2, &assetName);
	SQRESULT result = SQRESULT_ERROR;
	if (strncmp(assetName, "datatable/", 10) != 0)
	{
		spdlog::error("Asset \"{}\" doesn't start with \"datatable/\"", assetName);
	}
	else if ((!Cvar_ns_prefere_datatable_from_disk->GetBool()) && g_pPakLoadManager->FileExists(assetName) )
	{
		//spdlog::info("Load Datatable {} from rpak", assetName);
		result = sqManager->m_funcOriginals["GetDataTable"](sqvm);
	}
	else
	{
		char assetPath[250];
		snprintf(assetPath, 250, "scripts/%s", assetName);
		if (cacheMap.count(assetName))
		{
			//spdlog::info("Loaded custom Datatable {} from cache", assetName);
			csvData** dataPointer = (csvData**)sqManager->createuserdata(sqvm, sizeof(csvData*));
			*dataPointer = cacheMap[assetName];
			sqManager->setuserdatatypeid(sqvm, -1, customDatatableTypeId);
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
			csvData** dataPointer = (csvData**)sqManager->createuserdata(sqvm, sizeof(csvData*));
			sqManager->setuserdatatypeid(sqvm, -1, customDatatableTypeId);

			*dataPointer = data;
			// vm->_stack[vm->_top -1]._VAL.asUserdata->releaseHook = datatableReleaseHook;
			cacheMap[assetName] = data;
			//spdlog::info("Loaded custom Datatable from file at {} with pointer {}", assetPath, (void*)data);

			result = SQRESULT_NOTNULL;
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
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableColumnByName"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	//spdlog::info("GetColumnByName form datatable with pointer {}",(void*)data);
	const char* searchName = sqManager->getstring(sqvm, 2);
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
	sqManager->pushinteger(sqvm, col);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDatatabeRowCount(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableRowCount");
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{

		return sqManager->m_funcOriginals["GetDatatableRowCount"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	sqManager->pushinteger(sqvm, data->dataPointers.size());
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableString(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableString");
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableString"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	int row = sqManager->getinteger(sqvm, 2);
	int col = sqManager->getinteger(sqvm, 3);
	if (row >= data->dataPointers.size() || col >= data->dataPointers[row].size())
	{
		spdlog::info( "row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}

	sqManager->pushstring(sqvm, data->dataPointers[row][col], -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableAsset(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableAsset");
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableAsset"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	int row = sqManager->getinteger(sqvm, 2);
	int col = sqManager->getinteger(sqvm, 3);
	if (row >= data->dataPointers.size() || col >= data->dataPointers[row].size())
	{
		spdlog::info( "row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}
	char* asset = data->dataPointers[row][col];
	sqManager->pushasset(sqvm, asset, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableInt(HSquirrelVM* sqvm)
{
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableInt"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	int row = sqManager->getinteger(sqvm, 2);
	int col = sqManager->getinteger(sqvm, 3);
	if (row >= data->dataPointers.size() || col >= data->dataPointers[row].size())
	{
		spdlog::info( "row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}
	sqManager->pushinteger(sqvm, std::stoi(data->dataPointers[row][col]));
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableFloat(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableFloat");
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableFloat"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	int row = sqManager->getinteger(sqvm, 2);
	int col = sqManager->getinteger(sqvm, 3);
	if (row >= data->dataPointers.size() || col >= data->dataPointers[row].size())
	{
		spdlog::info( "row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}
	sqManager->pushfloat(sqvm, std::stof(data->dataPointers[row][col]));
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableBool(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableBool");
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableBool"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	int row = sqManager->getinteger(sqvm, 2);
	int col = sqManager->getinteger(sqvm, 3);
	if (row >= data->dataPointers.size() || col >= data->dataPointers[row].size())
	{
		spdlog::info( "row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}
	sqManager->pushbool(sqvm, std::stoi(data->dataPointers[row][col]));
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableVector(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableVector");
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableVector"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}
	float vector[3];

	int row = sqManager->getinteger(sqvm, 2);
	int col = sqManager->getinteger(sqvm, 3);
	if (row >= data->dataPointers.size() || col >= data->dataPointers[row].size())
	{
		spdlog::info(
			"row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}

	StringToVector(data->dataPointers[row][col], vector);
	sqManager->pushvector(sqvm, vector);

	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowMatchingStringValue(HSquirrelVM* sqvm)
{
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableRowMatchingStringValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = sqManager->getinteger(sqvm, 2);
	const char* stringValue = sqManager->getstring(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{
		if (!strcmp(data->dataPointers[i][col], stringValue))
		{
			sqManager->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	sqManager->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowMatchingAssetValue(HSquirrelVM* sqvm)
{
	//spdlog::info("start getDatatableRowMatchingAsset");
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableRowMatchingAssetValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = sqManager->getinteger(sqvm, 2);
	const char* stringValue;
	sqManager->getasset(sqvm, 3, &stringValue);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{
		if (!strcmp(data->dataPointers[i][col], stringValue))
		{
			sqManager->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	sqManager->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowMatchingFloatValue(HSquirrelVM* sqvm)
{
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableRowMatchingFloatValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = sqManager->getinteger(sqvm, 2);
	float compareValue = sqManager->getfloat(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{

		if (compareValue == std::stof(data->dataPointers[i][col]))
		{
			sqManager->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	sqManager->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowMatchingIntValue(HSquirrelVM* sqvm)
{
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableRowMatchingIntValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = sqManager->getinteger(sqvm, 2);
	int compareValue = sqManager->getinteger(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{

		if (compareValue == std::stoi(data->dataPointers[i][col]))
		{
			sqManager->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	sqManager->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowGreaterThanOrEqualToIntValue(HSquirrelVM* sqvm)
{
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableRowGreaterThanOrEqualToIntValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = sqManager->getinteger(sqvm, 2);
	int compareValue = sqManager->getinteger(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{

		if (compareValue >= std::stoi(data->dataPointers[i][col]))
		{
			sqManager->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	sqManager->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowLessThanOrEqualToIntValue(HSquirrelVM* sqvm)
{
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableRowLessThanOrEqualToIntValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = sqManager->getinteger(sqvm, 2);
	int compareValue = sqManager->getinteger(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{

		if (compareValue <= std::stoi(data->dataPointers[i][col]))
		{
			sqManager->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	sqManager->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowGreaterThanOrEqualToFloatValue(HSquirrelVM* sqvm)
{
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableRowGreaterThanOrEqualToFloatValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = sqManager->getinteger(sqvm, 2);
	float compareValue = sqManager->getfloat(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{

		if (compareValue >= std::stof(data->dataPointers[i][col]))
		{
			sqManager->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	sqManager->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowLessThanOrEqualToFloatValue(HSquirrelVM* sqvm)
{
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableRowLessThanOrEqualToFloatValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = sqManager->getinteger(sqvm, 2);
	int compareValue = sqManager->getfloat(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{

		if (compareValue <= std::stof(data->dataPointers[i][col]))
		{
			sqManager->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	sqManager->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT GetDataTableRowMatchingVectorValue(HSquirrelVM* sqvm)
{
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**)&dataPointer, &typeId);
	csvData* data = *dataPointer;
	if (typeId == vanillaDatatableTypeId)
	{
		return sqManager->m_funcOriginals["GetDataTableRowMatchingVectorValue"](sqvm);
	}

	if (typeId != customDatatableTypeId)
	{
		return SQRESULT_ERROR;
	}

	int col = sqManager->getinteger(sqvm, 2);
	float* compareValue = sqManager->getvector(sqvm, 3);

	for (int i = 0; i < data->dataPointers.size(); i++)
	{
		float dataTableVector[3]; 
		StringToVector(data->dataPointers[i][col],dataTableVector);
		if ((dataTableVector[0] == compareValue[0]) && (dataTableVector[1] == compareValue[1]) && (dataTableVector[2] == compareValue[2]))
		{
			sqManager->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	sqManager->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT DumpDataTable(HSquirrelVM* sqvm)
{
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();

	DataTable* datatable = (DataTable*)getDataTableStructure(sqvm);
	if (datatable == 0)
	{
		spdlog::info("datatable not loaded");
		sqManager->pushinteger(sqvm, 1);
		return SQRESULT_NOTNULL;
	}
	//spdlog::info("Datatable size row = {} col = {}", datatable->rowAmount, datatable->columnAmount);
	// std::string header = std::string(datatable->columnInfo[0].name);

	spdlog::info(DataTableToString(datatable));

	return SQRESULT_NULL;
}

template <ScriptContext context> SQRESULT DumpDataTableToFile(HSquirrelVM* sqvm)
{
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();

	DataTable* datatable = (DataTable*)getDataTableStructure(sqvm);
	if (datatable == 0)
	{
		spdlog::info("datatable not loaded");
		sqManager->pushinteger(sqvm, 1);
		return SQRESULT_NOTNULL;
	}
	//spdlog::info("Datatable size row = {} col = {}", datatable->rowAmount, datatable->columnAmount);
	// std::string header = std::string(datatable->columnInfo[0].name);
	const char* pathName = sqManager->getstring(sqvm, 2);
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

ON_DLL_LOAD_RELIESON("server.dll", ServerScriptDatatables, ServerSquirrel, (CModule module))
{
	g_pServerSquirrel->AddFuncOverride("GetDataTable", GetDatatable<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableColumnByName", GetDatatabeColumnByName<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDatatableRowCount", GetDatatabeRowCount<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableString", GetDataTableString<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableInt", GetDataTableInt<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableFloat", GetDataTableFloat<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableBool", GetDataTableBool<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableAsset", GetDataTableAsset<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableVector", GetDataTableVector<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableRowMatchingStringValue", GetDataTableRowMatchingStringValue<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableRowMatchingAssetValue", GetDataTableRowMatchingAssetValue<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableRowMatchingFloatValue", GetDataTableRowMatchingFloatValue<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableRowMatchingIntValue", GetDataTableRowMatchingIntValue<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableRowMatchingVectorValue", GetDataTableRowMatchingVectorValue<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableRowLessThanOrEqualToFloatValue", GetDataTableRowLessThanOrEqualToFloatValue<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride(
		"GetDataTableRowGreaterThanOrEqualToFloatValue", GetDataTableRowGreaterThanOrEqualToFloatValue<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride(
		"GetDataTableRowLessThanOrEqualToIntValue", GetDataTableRowLessThanOrEqualToIntValue<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride(
		"GetDataTableRowGreaterThanOrEqualToFloatValue", GetDataTableRowGreaterThanOrEqualToIntValue<ScriptContext::SERVER>);

	g_pServerSquirrel->AddFuncRegistration(
		"void", "DumpDataTable", "var", "Dumps rpak datatable contents to console", DumpDataTable<ScriptContext::SERVER>);
	// g_pServerSquirrel->AddFuncRegistration( "void", "DumpDataTableToFile", "var,string", "Dumps datatable contents to console", DumpDataTableToFile<ScriptContext::SERVER>); 



		g_pClientSquirrel->AddFuncOverride("GetDataTable", GetDatatable<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDataTableColumnByName", GetDatatabeColumnByName<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDatatableRowCount", GetDatatabeRowCount<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDataTableString", GetDataTableString<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDataTableInt", GetDataTableInt<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDataTableFloat", GetDataTableFloat<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDataTableBool", GetDataTableBool<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDataTableAsset", GetDataTableAsset<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDataTableVector", GetDataTableVector<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDataTableRowMatchingStringValue", GetDataTableRowMatchingStringValue<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDataTableRowMatchingAssetValue", GetDataTableRowMatchingAssetValue<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDataTableRowMatchingFloatValue", GetDataTableRowMatchingFloatValue<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDataTableRowMatchingIntValue", GetDataTableRowMatchingIntValue<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride("GetDataTableRowMatchingVectorValue", GetDataTableRowMatchingVectorValue<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride(
		"GetDataTableRowLessThanOrEqualToFloatValue", GetDataTableRowLessThanOrEqualToFloatValue<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride(
		"GetDataTableRowGreaterThanOrEqualToFloatValue", GetDataTableRowGreaterThanOrEqualToFloatValue<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride(
		"GetDataTableRowLessThanOrEqualToIntValue", GetDataTableRowLessThanOrEqualToIntValue<ScriptContext::CLIENT>);
	g_pClientSquirrel->AddFuncOverride(
		"GetDataTableRowGreaterThanOrEqualToFloatValue", GetDataTableRowGreaterThanOrEqualToIntValue<ScriptContext::CLIENT>);


	g_pUISquirrel->AddFuncOverride("GetDataTable", GetDatatable<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDataTableColumnByName", GetDatatabeColumnByName<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDatatableRowCount", GetDatatabeRowCount<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDataTableString", GetDataTableString<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDataTableInt", GetDataTableInt<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDataTableFloat", GetDataTableFloat<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDataTableBool", GetDataTableBool<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDataTableAsset", GetDataTableAsset<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDataTableVector", GetDataTableVector<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDataTableRowMatchingStringValue", GetDataTableRowMatchingStringValue<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDataTableRowMatchingAssetValue", GetDataTableRowMatchingAssetValue<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDataTableRowMatchingFloatValue", GetDataTableRowMatchingFloatValue<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDataTableRowMatchingIntValue", GetDataTableRowMatchingIntValue<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride("GetDataTableRowMatchingVectorValue", GetDataTableRowMatchingVectorValue<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride(
		"GetDataTableRowLessThanOrEqualToFloatValue", GetDataTableRowLessThanOrEqualToFloatValue<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride(
		"GetDataTableRowGreaterThanOrEqualToFloatValue", GetDataTableRowGreaterThanOrEqualToFloatValue<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride(
		"GetDataTableRowLessThanOrEqualToIntValue", GetDataTableRowLessThanOrEqualToIntValue<ScriptContext::UI>);
	g_pUISquirrel->AddFuncOverride(
		"GetDataTableRowGreaterThanOrEqualToFloatValue", GetDataTableRowGreaterThanOrEqualToIntValue<ScriptContext::UI>);






	Cvar_ns_prefere_datatable_from_disk =
		new ConVar("ns_prefere_datatable_from_disk", "0", FCVAR_NONE, "whether datatables are only loaded from disk");

	getDataTableStructure = module.Offset(0x1250f0).As<void* (*)(HSquirrelVM*)>();
}
