#pragma once

class PakLoadManager
{
  public:
	void LoadPakSync(const char* path);
	void LoadPakAsync(const char* path, bool bMarkForUnload);
	void UnloadPaks();
	long long FileExists(const char* path);

  private:
	std::vector<int> m_pakHandlesToUnload;
};

extern PakLoadManager* g_pPakLoadManager;
