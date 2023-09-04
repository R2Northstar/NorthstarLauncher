class ModDownloader
{
  private:
	const char* STORE_URL = "https://gcdn.thunderstore.io/live/repository/packages/";
	const char* MODS_LIST_URL = "https://raw.githubusercontent.com/R2Northstar/VerifiedMods/master/mods.json";
	const char* VERIFICATION_COMMAND = "-disablemodverification";

	struct VerifiedModVersion
	{
		std::string checksum;
	};
	struct VerifiedModDetails
	{
		std::string dependencyPrefix;
		std::unordered_map<std::string, VerifiedModVersion> versions = {};
	};
	std::unordered_map<std::string, VerifiedModDetails> verifiedMods = {};

	/**
	 * Downloads a mod archive from distant store.
	 *
	 * This rebuilds the URI of the mod archive using both a predefined store URI
	 * and the mod dependency string from the `verifiedMods` variabl; fetched
	 * archive is then stored in a temporary location.
	 *
	 * @param modName name of the mod to be downloaded
	 * @param modVersion version of the mod to be downloaded
	 * @returns location of the downloaded archive
	 */
	fs::path FetchModFromDistantStore(std::string modName, std::string modVersion);

	/**
	 * Tells if a mod archive has not been corrupted.
	 *
	 * The mod validation procedure includes computing the SHA256 hash of the final
	 * archive, which is stored in the verified mods list. This hash is used by this
	 * very method to ensure the archive downloaded from the Internet is the exact
	 * same that has been manually verified.
	 *
	 * @param modPath path of the archive to check
	 * @param expectedChecksum checksum the archive should have
	 * @returns whether archive is legit
	 */
	bool IsModLegit(fs::path modPath, std::string expectedChecksum);

	/**
	 * Extracts a mod archive to the game folder.
	 *
	 * This extracts a downloaded mod archive from its original location to the
	 * current game profile, in the remote mods folder.
	 *
	 * @param modPath location of the downloaded archive
	 * @returns nothing
	 */
	void ExtractMod(fs::path modPath);

  public:
	ModDownloader();

	/**
	 * Retrieves the verified mods list from the central authority.
	 *
	 * The Northstar auto-downloading feature does NOT allow automatically installing
	 * all mods for various (notably security) reasons; mods that are candidate to
	 * auto-downloading are rather listed on a GitHub repository
	 * (https://raw.githubusercontent.com/R2Northstar/VerifiedMods/master/mods.json),
	 * which this method gets via a HTTP call to load into local state.
	 *
	 * If list fetching fails, local mods list will be initialized as empty, thus
	 * preventing any mod from being auto-downloaded.
	 *
	 * @returns nothing
	 */
	void FetchModsListFromAPI();

	/**
	 * Downloads a given mod from Thunderstore API to local game profile.
	 *
	 * @param modName name of the mod to be downloaded
	 * @param modVersion version of the mod to be downloaded
	 * @returns nothing
	 **/
	void DownloadMod(std::string modName, std::string modVersion);

	/**
	 * Checks whether a mod is verified.
	 *
	 * A mod is deemed verified/authorized through a manual validation process that is
	 * described here: https://github.com/R2Northstar/VerifiedMods; in practice, a mod
	 * is considered authorized if their name AND exact version appear in the
	 * `verifiedMods` variable.
	 *
	 * @param modName name of the mod to be checked
	 * @param modVersion version of the mod to be checked, must follow semantic versioning
	 * @returns whether the mod is authorized and can be auto-downloaded
	 */
	bool IsModAuthorized(std::string modName, std::string modVersion);

	enum ModInstallState
	{
		// Normal installation process
		DOWNLOADING,
		CHECKSUMING,
		EXTRACTING,
		DONE, // Everything went great, mod can be used in-game

		// Errors
		FAILED, // Generic error message, should be avoided as much as possible
		FAILED_READING_ARCHIVE,
		FAILED_WRITING_TO_DISK,
		MOD_FETCHING_FAILED,
		MOD_CORRUPTED, // Downloaded archive checksum does not match verified hash
		NO_DISK_SPACE_AVAILABLE,
		NOT_FOUND // Mod is not currently being auto-downloaded
	};

	struct MOD_STATE
	{
		ModInstallState state;
		int progress;
		int total;
		float ratio;
	} modState;

	/**
	 * Retrieves installation information of a mod.
	 *
	 * Since mods are installed one at a time, mod installation data is stored in a global
	 * MOD_STATE instance.
	 *
	 * @param modName name of the mod to get information of
	 * @returns the class MOD_STATE instance
	 */
	MOD_STATE GetModInstallProgress(std::string modName);

	/**
	 * Cancels installation of a mod.
	 *
	 * Prevents installation of a mod currently being installed, no matter the install
	 * progress (downloading, checksuming, extracting), and frees all resources currently
	 * being used in this purpose.
	 *
	 * Does nothing if mod passed as argument is not being installed.
	 *
	 * @param modName name of the mod to prevent installation of
	 * @returns nothing
	 */
	void CancelDownload(std::string modName);
};
