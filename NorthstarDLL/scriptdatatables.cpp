#include "pch.h"
#include "scriptdatatables.h"
#include "squirrel.h"
#include "rpakfilesystem.h"
#include "convar.h"
#include "filesystem.h"
#include <iostream>
#include <sstream>
#include <map>

ConVar *Cvar_ns_force_datatable_from_disk;
void datatableReleaseHook(void*, int size);
const long long customDatatableTypeId = 0xFFFCFFFC12345678;
const long long vanillaDatatableTypeId = 0xFFF7FFF700000004;

struct csvData
{
	std::vector<std::vector<char*>> dataPointers;
	char* fullData = 0;
	std::vector<char*> columnNames;
};

std::unordered_map<std::string,csvData*> cacheMap;

void StringToVector(char * string, float* vector) 
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
	SquirrelManager<context> *sqManager = GetSquirrelManager<context>();
	sqManager->getasset(sqvm, 2, &assetName);
	SQRESULT result = SQRESULT_ERROR;
	if (strncmp(assetName,"datatable/",10)!=0)
	{
		spdlog::error("Asset \"{}\" doesn't start with \"datatable/\"", assetName);
	}
	else if (g_pPakLoadManager->FileExists(assetName)&&(!Cvar_ns_force_datatable_from_disk->GetBool())) 
	{
		spdlog::info("Load Datatable {} from rpak",assetName);
		result = sqManager->m_funcOriginals["GetDataTable"](sqvm);
	}
	else 
	{
		char assetPath[250];
		snprintf(assetPath, 250, "scripts/%s", assetName);
		if (cacheMap.count(assetName))
		{
			spdlog::info("Loaded custom Datatable {} from cache", assetName);
			csvData** dataPointer = (csvData**)sqManager->createuserdata(sqvm, sizeof(csvData*));
			*dataPointer = cacheMap[assetName];
			sqManager->setuserdatatypeid(sqvm, -1, customDatatableTypeId);
			//sqvm->_stack[sqvm->_top -1]._VAL.asUserdata->releaseHook = datatableReleaseHook;
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
					if (csvFullData[i+1] == '\"')
					{

						currentLine.push_back(&csvFullData[i+2]);
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
						currentLine.push_back(&csvFullData[i+1]);
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
			
			
			csvData** dataPointer = (csvData**)sqManager->createuserdata(sqvm, sizeof(csvData*));
			sqManager->setuserdatatypeid(sqvm, -1, customDatatableTypeId);
			
			*dataPointer = data;
			// vm->_stack[vm->_top -1]._VAL.asUserdata->releaseHook = datatableReleaseHook;
			cacheMap[assetName] = data;
			spdlog::info("Loaded custom Datatable from file at {} with pointer {}", assetPath,(void*)data);
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
	// spdlog::info("start getDatatableColumnByName");
	SquirrelManager<context>* sqManager = GetSquirrelManager<context>();
	csvData** dataPointer;
	long long typeId;
	sqManager->getuserdata(sqvm, 2, (void**) &dataPointer, &typeId);
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
	sqManager->pushinteger(sqvm,col );
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
	SquirrelManager<context> *sqManager = GetSquirrelManager<context>();
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
		spdlog::info(
			"row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
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
		spdlog::info(
			"row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
		return SQRESULT_ERROR;
	}
	sqManager->pushasset(sqvm, data->dataPointers[row][col], 1);
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
		spdlog::info(
			"row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
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
		spdlog::info(
			"row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
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
		spdlog::info(
			"row {} and col {} are outside of range row {} and col {}", row, col, data->dataPointers.size(), data->columnNames.size());
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
	sqManager->pushvector(sqvm,vector);
	
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
	const char *stringValue = sqManager->getstring(sqvm, 3);


	

	for (int i = 0; i < data->dataPointers.size(); i++)
	{
		if (!strcmp(data->dataPointers[i][col],stringValue))
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
	char* stringValue;
	sqManager->getasset(sqvm, 3,&stringValue);

	for (int i = 0; i < data.pointerData.size(); i++)
	{
		if (!strcmp(data->dataPointers[i][col]))
		{
			sqManager->pushinteger(sqvm, i);
			return SQRESULT_NOTNULL;
		}
	}
	sqManager->pushinteger(sqvm, -1);
	return SQRESULT_NOTNULL;
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
	g_pServerSquirrel->AddFuncOverride("GetDataTableVector", GetDataTableVector < ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableRowMatchingStringValue",GetDataTableRowMatchingStringValue<ScriptContext::SERVER>);
	g_pServerSquirrel->AddFuncOverride("GetDataTableRowMatchingAssetValue", GetDataTableRowMatchingAssetValue<ScriptContext::SERVER>);

	Cvar_ns_force_datatable_from_disk = new ConVar("ns_force_datatable_from_disk", "0", FCVAR_NONE, "whether datatables are only loaded from disk");
}

