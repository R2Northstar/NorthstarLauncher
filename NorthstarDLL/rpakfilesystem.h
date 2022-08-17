#pragma once

class PakLoadManager
{
  public:
	void LoadPakSync(const char* path);
	void LoadPakAsync(const char* path, bool bMarkForUnload);
	void UnloadPaks();
	void* LoadFile(const char* path); //this is a guess

  private:
	std::vector<int> m_pakHandlesToUnload;
};

extern PakLoadManager* g_pPakLoadManager;
