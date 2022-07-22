#pragma once
void InitialiseClientSaveFiles(HMODULE baseAddress);
void InitialiseServerSaveFiles(HMODULE baseAddress);
bool ContainsNonASCIIChars(std::string str);
bool saveFilesEnabled = true;