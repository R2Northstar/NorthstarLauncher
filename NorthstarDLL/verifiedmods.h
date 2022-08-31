#pragma once
#include "memalloc.h"

void _FetchVerifiedModsList();
rapidjson_document GetVerifiedModsList();
void DownloadMod(char* dependencyString);
void InitialiseVerifiedModsScripts(HMODULE baseAddress);
