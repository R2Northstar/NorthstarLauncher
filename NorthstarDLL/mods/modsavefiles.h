#pragma once
bool ContainsInvalidChars(std::string str);

class SaveFileManager
{
  public:
	template <ScriptContext context> void SaveFileAsync(fs::path file, std::string content);
	template <ScriptContext context> int LoadFileAsync(fs::path file);
	template <ScriptContext context> void DeleteFileAsync(fs::path file);
	std::map<fs::path, std::mutex> mutexMap;

  private:
	int m_iLastRequestHandle = 0;
};
