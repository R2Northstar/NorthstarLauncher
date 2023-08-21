class ModDownloader
{
	private:
		rapidjson_document authorizedMods;
		std::vector<std::string> downloadingMods {};
		void FetchModsListFromAPI();

		fs::path FetchModFromDistantStore(std::string modName);
		bool IsModLegit(fs::path modPath, std::string expectedChecksum);
		void ExtractMod(fs::path modPath);

	public:
		ModDownloader();
		bool DownloadMod(std::string modName, std::string modVersion);
		bool IsModAuthorized(std::string modName, std::string modVersion);
		std::string GetModInstallProgress(std::string modName); // TODO return a struct
		void CancelDownload(std::string modName);
};
