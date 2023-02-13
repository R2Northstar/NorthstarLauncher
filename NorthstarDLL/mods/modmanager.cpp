#include "pch.h"
#include "modmanager.h"
#include "core/convar/convar.h"
#include "core/convar/concommand.h"
#include "client/audio.h"
#include "engine/r2engine.h"
#include "masterserver/masterserver.h"
#include "core/filesystem/filesystem.h"
#include "core/filesystem/rpakfilesystem.h"
#include "config/profile.h"
#include "dedicated/dedicated.h"

#include "rapidjson/error/en.h"
#include "rapidjson/document.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

ModManager* g_pModManager;

Mod::Mod(fs::path modDir, std::string sJson, bool bRemote)
{
	m_bWasReadSuccessfully = false;

	m_ModDirectory = modDir;
	m_bRemote = bRemote;

	rapidjson_document modJson;
	modJson.Parse<rapidjson::ParseFlag::kParseCommentsFlag | rapidjson::ParseFlag::kParseTrailingCommasFlag>(sJson);

	// fail if parse error
	if (modJson.HasParseError())
	{
		spdlog::error(
			"Failed reading mod file {}: encountered parse error \"{}\" at offset {}",
			(modDir / "mod.json").string(),
			GetParseError_En(modJson.GetParseError()),
			modJson.GetErrorOffset());
		return;
	}

	// fail if it's not a json obj (could be an array, string, etc)
	if (!modJson.IsObject())
	{
		spdlog::error("Failed reading mod file {}: file is not a JSON object", (modDir / "mod.json").string());
		return;
	}

	// basic mod info
	// name is required
	if (!modJson.HasMember("Name"))
	{
		spdlog::error("Failed reading mod file {}: missing required member \"Name\"", (modDir / "mod.json").string());
		return;
	}

	Name = modJson["Name"].GetString();

	if (modJson.HasMember("Description"))
		Description = modJson["Description"].GetString();
	else
		Description = "";

	if (modJson.HasMember("Version"))
		Version = modJson["Version"].GetString();
	else
	{
		Version = "0.0.0";
		spdlog::warn("Mod file {} is missing a version, consider adding a version", (modDir / "mod.json").string());
	}

	if (modJson.HasMember("DownloadLink"))
		DownloadLink = modJson["DownloadLink"].GetString();
	else
		DownloadLink = "";

	if (modJson.HasMember("RequiredOnClient"))
		RequiredOnClient = modJson["RequiredOnClient"].GetBool();
	else
		RequiredOnClient = false;

	if (modJson.HasMember("LoadPriority"))
		LoadPriority = modJson["LoadPriority"].GetInt();
	else
	{
		spdlog::info("Mod file {} is missing a LoadPriority, consider adding one", (modDir / "mod.json").string());
		LoadPriority = 0;
	}

	// mod convars
	if (modJson.HasMember("ConVars") && modJson["ConVars"].IsArray())
	{
		for (auto& convarObj : modJson["ConVars"].GetArray())
		{
			if (!convarObj.IsObject() || !convarObj.HasMember("Name") || !convarObj.HasMember("DefaultValue"))
				continue;

			ModConVar convar;
			convar.Name = convarObj["Name"].GetString();
			convar.DefaultValue = convarObj["DefaultValue"].GetString();

			if (convarObj.HasMember("HelpString"))
				convar.HelpString = convarObj["HelpString"].GetString();
			else
				convar.HelpString = "";

			convar.Flags = FCVAR_NONE;

			if (convarObj.HasMember("Flags"))
			{
				// read raw integer flags
				if (convarObj["Flags"].IsInt())
					convar.Flags = convarObj["Flags"].GetInt();
				else if (convarObj["Flags"].IsString())
				{
					// parse cvar flags from string
					// example string: ARCHIVE_PLAYERPROFILE | GAMEDLL
					convar.Flags |= ParseConVarFlagsString(convar.Name, convarObj["Flags"].GetString());
				}
			}

			ConVars.push_back(convar);
		}
	}

	// mod commands
	if (modJson.HasMember("ConCommands") && modJson["ConCommands"].IsArray())
	{
		for (auto& concommandObj : modJson["ConCommands"].GetArray())
		{
			if (!concommandObj.IsObject() || !concommandObj.HasMember("Name") || !concommandObj.HasMember("Function") ||
				!concommandObj.HasMember("Context"))
			{
				continue;
			}

			ModConCommand concommand;
			concommand.Name = concommandObj["Name"].GetString();
			concommand.Function = concommandObj["Function"].GetString();
			concommand.Context = ScriptContextFromString(concommandObj["Context"].GetString());

			if (concommand.Context == ScriptContext::INVALID)
			{
				spdlog::warn("Mod ConCommand {} has invalid context {}", concommand.Name, concommandObj["Context"].GetString());
				continue;
			}

			if (concommandObj.HasMember("HelpString"))
				concommand.HelpString = concommandObj["HelpString"].GetString();
			else
				concommand.HelpString = "";

			concommand.Flags = FCVAR_NONE;

			if (concommandObj.HasMember("Flags"))
			{
				// read raw integer flags
				if (concommandObj["Flags"].IsInt())
					concommand.Flags = concommandObj["Flags"].GetInt();
				else if (concommandObj["Flags"].IsString())
				{
					// parse cvar flags from string
					// example string: ARCHIVE_PLAYERPROFILE | GAMEDLL
					concommand.Flags |= ParseConVarFlagsString(concommand.Name, concommandObj["Flags"].GetString());
				}
			}

			// for commands, client should always be FCVAR_CLIENTDLL, and server should always be FCVAR_GAMEDLL
			if (concommand.Context == ScriptContext::CLIENT)
				concommand.Flags |= FCVAR_CLIENTDLL;
			else if (concommand.Context == ScriptContext::SERVER)
				concommand.Flags |= FCVAR_GAMEDLL;


			ConCommands.push_back(concommand);
		}
	}

	// mod scripts
	if (modJson.HasMember("Scripts") && modJson["Scripts"].IsArray())
	{
		for (auto& scriptObj : modJson["Scripts"].GetArray())
		{
			if (!scriptObj.IsObject() || !scriptObj.HasMember("Path") || !scriptObj.HasMember("RunOn"))
				continue;

			ModScript script;

			script.Path = scriptObj["Path"].GetString();
			script.RunOn = scriptObj["RunOn"].GetString();

			if (scriptObj.HasMember("ServerCallback") && scriptObj["ServerCallback"].IsObject())
			{
				ModScriptCallback callback;
				callback.Context = ScriptContext::SERVER;

				if (scriptObj["ServerCallback"].HasMember("Before") && scriptObj["ServerCallback"]["Before"].IsString())
					callback.BeforeCallback = scriptObj["ServerCallback"]["Before"].GetString();

				if (scriptObj["ServerCallback"].HasMember("After") && scriptObj["ServerCallback"]["After"].IsString())
					callback.AfterCallback = scriptObj["ServerCallback"]["After"].GetString();

				if (scriptObj["ServerCallback"].HasMember("Destroy") && scriptObj["ServerCallback"]["Destroy"].IsString())
					callback.DestroyCallback = scriptObj["ServerCallback"]["Destroy"].GetString();

				script.Callbacks.push_back(callback);
			}

			if (scriptObj.HasMember("ClientCallback") && scriptObj["ClientCallback"].IsObject())
			{
				ModScriptCallback callback;
				callback.Context = ScriptContext::CLIENT;

				if (scriptObj["ClientCallback"].HasMember("Before") && scriptObj["ClientCallback"]["Before"].IsString())
					callback.BeforeCallback = scriptObj["ClientCallback"]["Before"].GetString();

				if (scriptObj["ClientCallback"].HasMember("After") && scriptObj["ClientCallback"]["After"].IsString())
					callback.AfterCallback = scriptObj["ClientCallback"]["After"].GetString();

				if (scriptObj["ClientCallback"].HasMember("Destroy") && scriptObj["ClientCallback"]["Destroy"].IsString())
					callback.DestroyCallback = scriptObj["ClientCallback"]["Destroy"].GetString();

				script.Callbacks.push_back(callback);
			}

			if (scriptObj.HasMember("UICallback") && scriptObj["UICallback"].IsObject())
			{
				ModScriptCallback callback;
				callback.Context = ScriptContext::UI;

				if (scriptObj["UICallback"].HasMember("Before") && scriptObj["UICallback"]["Before"].IsString())
					callback.BeforeCallback = scriptObj["UICallback"]["Before"].GetString();

				if (scriptObj["UICallback"].HasMember("After") && scriptObj["UICallback"]["After"].IsString())
					callback.AfterCallback = scriptObj["UICallback"]["After"].GetString();

				if (scriptObj["UICallback"].HasMember("Destroy") && scriptObj["UICallback"]["Destroy"].IsString())
					callback.DestroyCallback = scriptObj["UICallback"]["Destroy"].GetString();

				script.Callbacks.push_back(callback);
			}

			Scripts.push_back(script);
		}
	}

	if (modJson.HasMember("Localisation") && modJson["Localisation"].IsArray())
	{
		for (auto& localisationStr : modJson["Localisation"].GetArray())
		{
			if (!localisationStr.IsString())
				continue;

			LocalisationFiles.push_back(localisationStr.GetString());
		}
	}

	if (modJson.HasMember("Dependencies") && modJson["Dependencies"].IsObject())
	{
		for (auto v = modJson["Dependencies"].MemberBegin(); v != modJson["Dependencies"].MemberEnd(); v++)
		{
			if (!v->name.IsString() || !v->value.IsString())
				continue;

			spdlog::info("Constant {} defined by {} for mod {}", v->name.GetString(), Name, v->value.GetString());
			if (DependencyConstants.find(v->name.GetString()) != DependencyConstants.end() &&
				v->value.GetString() != DependencyConstants[v->name.GetString()])
			{
				spdlog::error("A dependency constant with the same name already exists for another mod. Change the constant name.");
				return;
			}

			if (DependencyConstants.find(v->name.GetString()) == DependencyConstants.end())
				DependencyConstants.emplace(v->name.GetString(), v->value.GetString());
		}
	}

	m_bWasReadSuccessfully = true;
}

ModManager::ModManager()
{
	// precaculated string hashes
	// note: use backslashes for these, since we use lexically_normal for file paths which uses them
	m_hScriptsRsonHash = STR_HASH("scripts\\vscripts\\scripts.rson");
	m_hPdefHash = STR_HASH(
		"cfg\\server\\persistent_player_data_version_231.pdef" // this can have multiple versions, but we use 231 so that's what we hash
	);
	m_hKBActHash = STR_HASH("scripts\\kb_act.lst");

	m_LastModLoadState = nullptr;
	m_ModLoadState = new ModLoadState;

	LoadMods();
}

template <ScriptContext context> auto ModConCommandCallback_Internal(std::string name, const CCommand& command)
{
	if (g_pSquirrel<context>->m_pSQVM && g_pSquirrel<context>->m_pSQVM->sqvm)
	{
		std::vector<std::string> vArgs;
		vArgs.reserve(command.ArgC());
		for (int i = 1; i < command.ArgC(); i++)
			vArgs.push_back(command.Arg(i));

		g_pSquirrel<context>->AsyncCall(name, vArgs);
	}
	else
		spdlog::warn("ConCommand \"{}\" was called while the associated Squirrel VM \"{}\" was unloaded", name, GetContextName(context));
}

auto ModConCommandCallback(const CCommand& command)
{
	ModConCommand* pFoundCommand = nullptr;
	std::string sCommandName = command.Arg(0);

	// Find the mod this command belongs to
	for (Mod& mod : g_pModManager->GetMods() | ModManager::FilterEnabled)
	{
		auto res = std::find_if(
			mod.ConCommands.begin(),
			mod.ConCommands.end(),
			[&sCommandName](const ModConCommand& other) { return other.Name == sCommandName; });

		if (res != mod.ConCommands.end())
		{
			pFoundCommand = &*res;
			break;
		}
	}

	if (!pFoundCommand)
		return;

	switch (pFoundCommand->Context)
	{
	case ScriptContext::CLIENT:
		ModConCommandCallback_Internal<ScriptContext::CLIENT>(pFoundCommand->Function, command);
		break;
	case ScriptContext::SERVER:
		ModConCommandCallback_Internal<ScriptContext::SERVER>(pFoundCommand->Function, command);
		break;
	case ScriptContext::UI:
		ModConCommandCallback_Internal<ScriptContext::UI>(pFoundCommand->Function, command);
		break;
	};
}




void ModManager::LoadMods()
{
	// reset state of all currently loaded mods, if we've loaded once already
	if (m_bHasLoadedMods)
		UnloadMods();

	// ensure dirs exist
	fs::create_directories(GetModFolderPath());
	fs::create_directories(GetRemoteModFolderPath());

	// load definitions (mod.json files)
	LoadModDefinitions();

	// install mods (load all files)
	InstallMods();

	// write json storing currently enabled mods
	SaveEnabledMods();

	// build modinfo obj for masterserver
	rapidjson_document modinfoDoc;
	auto& alloc = modinfoDoc.GetAllocator();
	modinfoDoc.SetObject();
	modinfoDoc.AddMember("Mods", rapidjson::kArrayType, alloc);

	int currentModIndex = 0;
	for (Mod& mod : GetMods())
	{
		if (!mod.m_bEnabled || !mod.RequiredOnClient) // (!mod.RequiredOnClient && !mod.Pdiff.size())
			continue;

		modinfoDoc["Mods"].PushBack(rapidjson::kObjectType, modinfoDoc.GetAllocator());
		modinfoDoc["Mods"][currentModIndex].AddMember("Name", rapidjson::StringRef(&mod.Name[0]), modinfoDoc.GetAllocator());
		modinfoDoc["Mods"][currentModIndex].AddMember("Version", rapidjson::StringRef(&mod.Version[0]), modinfoDoc.GetAllocator());
		modinfoDoc["Mods"][currentModIndex].AddMember("RequiredOnClient", mod.RequiredOnClient, modinfoDoc.GetAllocator());

		currentModIndex++;
	}

	rapidjson::StringBuffer buffer;
	buffer.Clear();
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	modinfoDoc.Accept(writer);
	g_pMasterServerManager->m_sOwnModInfoJson = std::string(buffer.GetString());

	// don't need this anymore
	delete m_LastModLoadState;
	m_LastModLoadState = nullptr;

	m_bHasLoadedMods = true;
}

void ModManager::LoadModDefinitions()
{
	bool bHasEnabledModsCfg = false;
	rapidjson_document enabledModsCfg;

	// read enabled mods cfg
	{
		std::ifstream enabledModsStream(GetNorthstarPrefix() / "enabledmods.json");
		std::stringstream enabledModsStringStream;

		if (!enabledModsStream.fail())
		{
			while (enabledModsStream.peek() != EOF)
				enabledModsStringStream << (char)enabledModsStream.get();

			enabledModsStream.close();
			enabledModsCfg.Parse<rapidjson::ParseFlag::kParseCommentsFlag | rapidjson::ParseFlag::kParseTrailingCommasFlag>(
				enabledModsStringStream.str().c_str());

			bHasEnabledModsCfg = enabledModsCfg.IsObject();
		}
	}

	// get mod directories for both local and remote mods
	std::vector<std::tuple<fs::path, bool>> vModDirs;
	for (fs::directory_entry dir : fs::directory_iterator(GetModFolderPath()))
		if (fs::exists(dir.path() / "mod.json"))
			vModDirs.push_back({dir.path(), false});

	for (fs::directory_entry dir : fs::directory_iterator(GetRemoteModFolderPath()))
		if (fs::exists(dir.path() / "mod.json"))
			vModDirs.push_back({dir.path(), true});

	for (auto remoteOrLocalModDir : vModDirs)
	{
		fs::path modDir = std::get<0>(remoteOrLocalModDir);
		bool bRemote = std::get<1>(remoteOrLocalModDir);

		std::string sJsonString;

		// read mod json file
		{
			std::stringstream jsonStringStream;
			std::ifstream jsonStream(modDir / "mod.json");

			// fail if no mod json
			if (jsonStream.fail())
			{
				spdlog::warn("Mod {} has a directory but no mod.json", modDir.string());
				continue;
			}

			while (jsonStream.peek() != EOF)
				jsonStringStream << (char)jsonStream.get();

			jsonStream.close();
			sJsonString = jsonStringStream.str();
		}

		// read mod
		Mod mod(modDir, sJsonString, bRemote);

		// maybe this should be in InstallMods()? unsure
		for (auto& pair : mod.DependencyConstants)
		{
			if (m_ModLoadState->m_DependencyConstants.find(pair.first) != m_ModLoadState->m_DependencyConstants.end() &&
				m_ModLoadState->m_DependencyConstants[pair.first] != pair.second)
			{
				spdlog::error("Constant {} in mod {} already exists in another mod.", pair.first, mod.Name);
				mod.m_bWasReadSuccessfully = false;
				break;
			}

			if (m_ModLoadState->m_DependencyConstants.find(pair.first) == m_ModLoadState->m_DependencyConstants.end())
				m_ModLoadState->m_DependencyConstants.emplace(pair);
		}

		if (!bRemote)
		{
			if (bHasEnabledModsCfg && enabledModsCfg.HasMember(mod.Name.c_str()))
				mod.m_bEnabled = enabledModsCfg[mod.Name.c_str()].IsTrue();
			else
				mod.m_bEnabled = true;
		}
		else
		{
			// todo: need custom logic for deciding whether to enable remote mods, but should be off by default
			// in the future, remote mods should only be enabled explicitly at runtime, never based on any file or persistent state
			mod.m_bEnabled = false;
		}

		if (mod.m_bWasReadSuccessfully)
		{
			spdlog::info("Loaded mod {} successfully", mod.Name);
			if (mod.m_bEnabled)
				spdlog::info("Mod {} is enabled", mod.Name);
			else
				spdlog::info("Mod {} is disabled", mod.Name);

			m_ModLoadState->m_LoadedMods.push_back(mod);
		}
		else
			spdlog::warn("Skipping loading mod file {}", (modDir / "mod.json").string());
	}

	// sort by load prio, lowest-highest
	std::sort(
		m_ModLoadState->m_LoadedMods.begin(),
		m_ModLoadState->m_LoadedMods.end(),
		[](Mod& a, Mod& b) { return a.LoadPriority < b.LoadPriority; });
}

#pragma region Mod asset installation funcs
void ModManager::InstallModCvars(Mod& mod)
{
	// register convars
	for (ModConVar convar : mod.ConVars)
	{
		ConVar* pVar = R2::g_pCVar->FindVar(convar.Name.c_str());

		// make sure convar isn't registered yet, if it is then modify its flags, helpstring etc
		if (!pVar)
		{
			// allocate there here, we can delete later if needed
			int nNameSize = convar.Name.size();
			char* pName = new char[nNameSize + 1];
			strncpy_s(pName, nNameSize + 1, convar.Name.c_str(), convar.Name.size());

			int nDefaultValueSize = convar.DefaultValue.size();
			char* pDefaultValue = new char[nDefaultValueSize + 1];
			strncpy_s(pDefaultValue, nDefaultValueSize + 1, convar.DefaultValue.c_str(), convar.DefaultValue.size());

			int nHelpSize = convar.HelpString.size();
			char* pHelpString = new char[nHelpSize + 1];
			strncpy_s(pHelpString, nHelpSize + 1, convar.HelpString.c_str(), convar.HelpString.size());

			pVar = new ConVar(pName, pDefaultValue, convar.Flags, pHelpString);
			m_RegisteredModConVars.insert(pVar);
		}
		else
		{
			// not a mod cvar, don't let us edit it!
			if (!m_RegisteredModConVars.contains(pVar))
			{
				spdlog::warn("Mod {} tried to create ConVar {} that was already defined in native code!", mod.Name, convar.Name);
				continue;
			}

			pVar->m_ConCommandBase.m_nFlags = convar.Flags;

			if (convar.HelpString.compare(pVar->GetHelpText()))
			{
				int nHelpSize = convar.HelpString.size();
				char* pNewHelpString = new char[nHelpSize + 1];
				strncpy_s(pNewHelpString, nHelpSize + 1, convar.HelpString.c_str(), convar.HelpString.size());

				// delete old, assign new
				delete pVar->m_ConCommandBase.m_pszHelpString;
				pVar->m_ConCommandBase.m_pszHelpString = pNewHelpString;
			}

			if (convar.DefaultValue.compare(pVar->m_pszDefaultValue))
			{
				bool bIsDefaultValue = !strcmp(pVar->GetString(), pVar->m_pszDefaultValue);

				int nDefaultValueSize = convar.DefaultValue.size();
				char* pNewDefaultValue = new char[nDefaultValueSize + 1];
				strncpy_s(pNewDefaultValue, nDefaultValueSize + 1, convar.DefaultValue.c_str(), convar.DefaultValue.size());

				// delete old, assign new
				delete pVar->m_pszDefaultValue;
				pVar->m_pszDefaultValue = pNewDefaultValue;

				if (bIsDefaultValue) // only set value if it's currently default value, if changed then don't
					pVar->SetValue(pNewDefaultValue);
			}
		}
	}

	// register command
	for (ModConCommand command : mod.ConCommands)
	{
		// make sure command isnt't registered multiple times.
		ConCommand* pCommand = R2::g_pCVar->FindCommand(command.Name.c_str());

		if (!pCommand)
		{
			// allocate there here, we can delete later if needed
			int nNameSize = command.Name.size();
			char* pName = new char[nNameSize + 1];
			strncpy_s(pName, nNameSize + 1, command.Name.c_str(), command.Name.size());

			int nHelpSize = command.HelpString.size();
			char* pHelpString = new char[nHelpSize + 1];
			strncpy_s(pHelpString, nHelpSize + 1, command.HelpString.c_str(), command.HelpString.size());

			pCommand = RegisterConCommand(pName, ModConCommandCallback, pHelpString, command.Flags);
			m_RegisteredModConCommands.insert(pCommand);
		}
		else
		{
			if (!m_RegisteredModConCommands.contains(pCommand))
			{
				spdlog::warn("Mod {} tried to create ConCommand {} that was already defined in native code!", mod.Name, command.Name);
				continue;
			}

			pCommand->m_nFlags = command.Flags;

			if (command.HelpString.compare(pCommand->GetHelpText()))
			{
				int nHelpSize = command.HelpString.size();
				char* pNewHelpString = new char[nHelpSize + 1];
				strncpy_s(pNewHelpString, nHelpSize + 1, command.HelpString.c_str(), command.HelpString.size());

				// delete old, assign new
				delete pCommand->m_pszHelpString;
				pCommand->m_pszHelpString = pNewHelpString;
			}
		}
	}
}

void ModManager::InstallModVpks(Mod& mod)
{
	// read vpk paths
	if (fs::exists(mod.m_ModDirectory / "vpk"))
	{
		// read vpk cfg
		std::ifstream vpkJsonStream(mod.m_ModDirectory / "vpk/vpk.json");
		std::stringstream vpkJsonStringStream;

		bool bUseVPKJson = false;
		rapidjson::Document dVpkJson;

		if (!vpkJsonStream.fail())
		{
			while (vpkJsonStream.peek() != EOF)
				vpkJsonStringStream << (char)vpkJsonStream.get();

			vpkJsonStream.close();
			dVpkJson.Parse<rapidjson::ParseFlag::kParseCommentsFlag | rapidjson::ParseFlag::kParseTrailingCommasFlag>(
				vpkJsonStringStream.str().c_str());

			bUseVPKJson = !dVpkJson.HasParseError() && dVpkJson.IsObject();
		}

		for (fs::directory_entry file : fs::directory_iterator(mod.m_ModDirectory / "vpk"))
		{
			// a bunch of checks to make sure we're only adding dir vpks and their paths are good
			// note: the game will literally only load vpks with the english prefix
			if (fs::is_regular_file(file) && file.path().extension() == ".vpk" &&
				file.path().string().find("english") != std::string::npos &&
				file.path().string().find(".bsp.pak000_dir") != std::string::npos)
			{
				std::string formattedPath = file.path().filename().string();

				// this really fucking sucks but it'll work
				std::string vpkName = formattedPath.substr(strlen("english"), formattedPath.find(".bsp") - 3);

				ModVPKEntry& modVpk = mod.Vpks.emplace_back();
				modVpk.m_bAutoLoad = !bUseVPKJson || (dVpkJson.HasMember("Preload") && dVpkJson["Preload"].IsObject() &&
													  dVpkJson["Preload"].HasMember(vpkName) && dVpkJson["Preload"][vpkName].IsTrue());
				modVpk.m_sVpkPath = (file.path().parent_path() / vpkName).string();

				if (m_bHasLoadedMods && modVpk.m_bAutoLoad)
					(*R2::g_pFilesystem)->m_vtable->MountVPK(*R2::g_pFilesystem, vpkName.c_str());
			}
		}
	}
}

void ModManager::InstallModRpaks(Mod& mod)
{
	// read rpak paths
	if (fs::exists(mod.m_ModDirectory / "paks"))
	{
		// read rpak cfg
		std::ifstream rpakJsonStream(mod.m_ModDirectory / "paks/rpak.json");
		std::stringstream rpakJsonStringStream;

		bool bUseRpakJson = false;
		rapidjson::Document dRpakJson;

		if (!rpakJsonStream.fail())
		{
			while (rpakJsonStream.peek() != EOF)
				rpakJsonStringStream << (char)rpakJsonStream.get();

			rpakJsonStream.close();
			dRpakJson.Parse<rapidjson::ParseFlag::kParseCommentsFlag | rapidjson::ParseFlag::kParseTrailingCommasFlag>(
				rpakJsonStringStream.str().c_str());

			bUseRpakJson = !dRpakJson.HasParseError() && dRpakJson.IsObject();
		}

		// read pak aliases
		if (bUseRpakJson && dRpakJson.HasMember("Aliases") && dRpakJson["Aliases"].IsObject())
		{
			for (rapidjson::Value::ConstMemberIterator iterator = dRpakJson["Aliases"].MemberBegin();
				 iterator != dRpakJson["Aliases"].MemberEnd();
				 iterator++)
			{
				if (!iterator->name.IsString() || !iterator->value.IsString())
					continue;

				mod.RpakAliases.insert(std::make_pair(iterator->name.GetString(), iterator->value.GetString()));
			}
		}

		for (fs::directory_entry file : fs::directory_iterator(mod.m_ModDirectory / "paks"))
		{
			// ensure we're only loading rpaks
			if (fs::is_regular_file(file) && file.path().extension() == ".rpak")
			{
				std::string pakName(file.path().filename().string());

				ModRpakEntry& modPak = mod.Rpaks.emplace_back();
				modPak.m_bAutoLoad = !bUseRpakJson || (dRpakJson.HasMember("Preload") && dRpakJson["Preload"].IsObject() &&
													   dRpakJson["Preload"].HasMember(pakName) && dRpakJson["Preload"][pakName].IsTrue());

				// postload things
				if (!bUseRpakJson ||
					(dRpakJson.HasMember("Postload") && dRpakJson["Postload"].IsObject() && dRpakJson["Postload"].HasMember(pakName)))
					modPak.m_sLoadAfterPak = dRpakJson["Postload"][pakName].GetString();

				modPak.m_sPakName = pakName;

				// read header of file and get the starpak paths
				// this is done here as opposed to on starpak load because multiple rpaks can load a starpak
				// and there is seemingly no good way to tell which rpak is causing the load of a starpak :/

				std::ifstream rpakStream(file.path(), std::ios::binary);

				// seek to the point in the header where the starpak reference size is
				rpakStream.seekg(0x38, std::ios::beg);
				int starpaksSize = 0;
				rpakStream.read((char*)&starpaksSize, 2);

				// seek to just after the header
				rpakStream.seekg(0x58, std::ios::beg);
				// read the starpak reference(s)
				std::vector<char> buf(starpaksSize);
				rpakStream.read(buf.data(), starpaksSize);

				rpakStream.close();

				// split the starpak reference(s) into strings to hash
				std::string str = "";
				for (int i = 0; i < starpaksSize; i++)
				{
					// if the current char is null, that signals the end of the current starpak path
					if (buf[i] != 0x00)
					{
						str += buf[i];
					}
					else
					{
						// only add the string we are making if it isnt empty
						if (!str.empty())
						{
							mod.StarpakPaths.push_back(STR_HASH(str));
							spdlog::info("Mod {} registered starpak '{}'", mod.Name, str);
							str = "";
						}
					}
				}

				// not using atm because we need to resolve path to rpak
				// if (m_hasLoadedMods && modPak.m_bAutoLoad)
				//	g_pPakLoadManager->LoadPakAsync(pakName.c_str());
			}
		}
	}
}

void ModManager::InstallModKeyValues(Mod& mod)
{
	// read keyvalues paths
	if (fs::exists(mod.m_ModDirectory / "keyvalues"))
	{
		for (fs::directory_entry file : fs::recursive_directory_iterator(mod.m_ModDirectory / "keyvalues"))
		{
			if (fs::is_regular_file(file))
			{
				std::string kvStr = g_pModManager->NormaliseModFilePath(file.path().lexically_relative(mod.m_ModDirectory / "keyvalues"));
				mod.KeyValues.emplace(STR_HASH(kvStr), kvStr);
			}
		}
	}
}

void ModManager::InstallModBinks(Mod& mod)
{
	// read bink video paths
	if (fs::exists(mod.m_ModDirectory / "media"))
	{
		for (fs::directory_entry file : fs::recursive_directory_iterator(mod.m_ModDirectory / "media"))
			if (fs::is_regular_file(file) && file.path().extension() == ".bik")
				mod.BinkVideos.push_back(file.path().filename().string());
	}
}

void ModManager::InstallModAudioOverrides(Mod& mod)
{
	// try to load audio
	if (fs::exists(mod.m_ModDirectory / "audio"))
	{
		for (fs::directory_entry file : fs::directory_iterator(mod.m_ModDirectory / "audio"))
		{
			if (fs::is_regular_file(file) && file.path().extension().string() == ".json")
			{
				if (!g_CustomAudioManager.TryLoadAudioOverride(file.path()))
				{
					spdlog::warn("Mod {} has an invalid audio def {}", mod.Name, file.path().filename().string());
					continue;
				}
			}
		}
	}
}

void ModManager::InstallModFileOverrides(Mod& mod)
{
	// install all "normal" file overrides (source/vpk filesystem) e.g. files in Northstar.CustomServers/mod
	if (fs::exists(mod.m_ModDirectory / MOD_OVERRIDE_DIR))
	{
		for (fs::directory_entry file : fs::recursive_directory_iterator(mod.m_ModDirectory / MOD_OVERRIDE_DIR))
		{
			std::string path = g_pModManager->NormaliseModFilePath(file.path().lexically_relative(mod.m_ModDirectory / MOD_OVERRIDE_DIR));
			if (file.is_regular_file() && m_ModLoadState->m_ModFiles.find(path) == m_ModLoadState->m_ModFiles.end())
			{
				ModOverrideFile modFile;
				modFile.m_pOwningMod = &mod;
				modFile.m_Path = path;
				modFile.m_tLastWriteTime = fs::last_write_time(file.path()); // need real path for this
				m_ModLoadState->m_ModFiles.insert(std::make_pair(path, modFile));
			}
		}
	}
}
#pragma endregion

void ModManager::CheckModFilesForChanges()
{
	// normal mod files
	{
		// check which file overrides have changed
		// we need to trigger a reload of a given asset if
		// a) the asset was overriden previously but has changed owner
		// b) the asset no longer has any overrides (use vanilla file)
		// c) the asset was using vanilla file but isn't anymore

		std::vector<ModOverrideFile*> vpChangedFiles;

		// check currently loaded mods for any removed or updated files vs last load
		for (auto& filePair : m_ModLoadState->m_ModFiles)
		{
			auto findFile = m_LastModLoadState->m_ModFiles.find(filePair.first);
			if (findFile == m_LastModLoadState->m_ModFiles.end() || findFile->second.m_tLastWriteTime != filePair.second.m_tLastWriteTime)
				vpChangedFiles.push_back(&filePair.second);
		}

		// check last load for any files removed
		for (auto& filePair : m_LastModLoadState->m_ModFiles)
			if (filePair.second.m_pOwningMod != nullptr &&
				m_ModLoadState->m_ModFiles.find(filePair.first) == m_ModLoadState->m_ModFiles.end())
				vpChangedFiles.push_back(&filePair.second);

		for (ModOverrideFile* pChangedFile : vpChangedFiles)
		{
			if (IsDedicatedServer())
			{
				// could check localisation here? but what's the point, localisation shouldn't be in mod fs
				// if (m_AssetTypesToReload.bLocalisation)

				if (!m_AssetTypesToReload.bAimAssistSettings && !pChangedFile->m_Path.parent_path().compare("cfg/aimassist/"))
				{
					m_AssetTypesToReload.bAimAssistSettings = true;
					continue;
				}

				if (!m_AssetTypesToReload.bMaterials && !pChangedFile->m_Path.parent_path().compare("materials/"))
				{
					m_AssetTypesToReload.bMaterials = true;
					continue;
				}

				if (!m_AssetTypesToReload.bUiScript)
				{

					// TODO: need to check whether any ui scripts have changed

					if (!pChangedFile->m_Path.parent_path().compare("resource/ui/"))
					{
						m_AssetTypesToReload.bUiScript = true;
						continue;
					}
				}
			}

			if (!m_AssetTypesToReload.bModels && !pChangedFile->m_Path.parent_path().compare("models/"))
			{
				m_AssetTypesToReload.bModels = true;
				continue;
			}

			// could also check this but no point as it should only be changed from mod keyvalues
			// if (!m_AssetTypesToReload.bPlaylists && !pChangedFile->m_Path.compare("playlists_v2.txt"))

			// we also check these on change of mod keyvalues
			if (!m_AssetTypesToReload.bWeaponSettings && !pChangedFile->m_Path.parent_path().compare("scripts/weapons/"))
			{
				m_AssetTypesToReload.bWeaponSettings = true;
				continue;
			}

			if (!m_AssetTypesToReload.bPlayerSettings && !pChangedFile->m_Path.parent_path().compare("scripts/players/"))
			{
				m_AssetTypesToReload.bPlayerSettings = true;
				continue;
			}

			// maybe also aibehaviour?
			if (!m_AssetTypesToReload.bAiSettings && !pChangedFile->m_Path.parent_path().compare("scripts/aisettings/"))
			{
				m_AssetTypesToReload.bAiSettings = true;
				continue;
			}

			if (!m_AssetTypesToReload.bDamageDefs && !pChangedFile->m_Path.parent_path().compare("scripts/damage/"))
			{
				m_AssetTypesToReload.bDamageDefs = true;
				continue;
			}
		}
	}

	// keyvalues

	//if (!m_AssetTypesToReload.bWeaponSettings && kvStr.compare("scripts/weapons/"))
	//{
	//	m_AssetTypesToReload.bWeaponSettings = true;
	//	continue;
	//}
	//
	//if (!m_AssetTypesToReload.bPlayerSettings && kvStr.compare("scripts/players/"))
	//{
	//	m_AssetTypesToReload.bPlayerSettings = true;
	//	continue;
	//}
	//
	//// maybe also aibehaviour?
	//if (!m_AssetTypesToReload.bAiSettings && kvStr.compare("scripts/aisettings/"))
	//{
	//	m_AssetTypesToReload.bAiSettings = true;
	//	continue;
	//}
	//
	//if (!m_AssetTypesToReload.bDamageDefs && kvStr.compare("scripts/damage/"))
	//{
	//	m_AssetTypesToReload.bDamageDefs = true;
	//	continue;
	//}
}

void ModManager::ReloadNecessaryModAssets()
{
	std::vector<std::string> vReloadCommands;

	if (m_AssetTypesToReload.bUiScript)
		vReloadCommands.push_back("uiscript_reset");

	if (m_AssetTypesToReload.bLocalisation)
		vReloadCommands.push_back("reload_localization");

	// after we reload_localization, we need to loadPlaylists, to keep playlist localisation
	if (m_AssetTypesToReload.bPlaylists || m_AssetTypesToReload.bLocalisation)
		vReloadCommands.push_back("loadPlaylists");

	if (m_AssetTypesToReload.bAimAssistSettings)
		vReloadCommands.push_back("ReloadAimAssistSettings");

	// need to reimplement mat_reloadmaterials for this
	//if (m_AssetTypesToReload.bMaterials)
	//	R2::Cbuf_AddText(R2::Cbuf_GetCurrentPlayer(), "mat_reloadmaterials", R2::cmd_source_t::kCommandSrcCode);

	//if (m_AssetTypesToReload.bWeaponSettings)
	//if (m_AssetTypesToReload.bPlayerSettings)
	//if (m_AssetTypesToReload.bAiSettings)
	//if (m_AssetTypesToReload.bDamageDefs)

	if (m_AssetTypesToReload.bModels)
		spdlog::warn("Need to reload models but can't without a restart!");

	for (std::string& sReloadCommand : vReloadCommands)
	{
		spdlog::info("Executing command {} for asset reload", sReloadCommand);
		R2::Cbuf_AddText(R2::Cbuf_GetCurrentPlayer(), sReloadCommand.c_str(), R2::cmd_source_t::kCommandSrcCode);
	}

	R2::Cbuf_Execute();
}

void ModManager::InstallMods()
{
	for (Mod& mod : GetMods() | FilterEnabled)
	{
		InstallModCvars(mod);
		InstallModVpks(mod);
		InstallModRpaks(mod);
		InstallModKeyValues(mod);
		InstallModBinks(mod);
		InstallModAudioOverrides(mod);
	}

	// in a seperate loop because we register mod files in reverse order, since mods loaded later should have their files prioritised
	for (Mod& mod : GetMods() | FilterEnabled | std::views::reverse)
		InstallModFileOverrides(mod);

	if (m_bHasLoadedMods) // only reload assets after initial load
	{
		CheckModFilesForChanges();
		ReloadNecessaryModAssets();
	}
}

void ModManager::SaveEnabledMods()
{
	// write from scratch every time, don't include unnecessary mods
	rapidjson_document enabledModsCfg;
	enabledModsCfg.SetObject();

	// add values
	for (Mod& mod : GetMods())
		enabledModsCfg.AddMember(rapidjson_document::StringRefType(mod.Name.c_str()), mod.m_bEnabled, enabledModsCfg.GetAllocator());

	// write
	std::ofstream sWriteStream(GetNorthstarPrefix() / "enabledmods.json");
	rapidjson::OStreamWrapper sWriteStreamWrapper(sWriteStream);
	rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(sWriteStreamWrapper);
	enabledModsCfg.Accept(writer);
}

void ModManager::UnloadMods()
{
	// save last state so we know what we need to reload
	m_LastModLoadState = m_ModLoadState;
	m_ModLoadState = new ModLoadState;

	// reset assets to reload
	m_AssetTypesToReload.bUiScript = false;
	m_AssetTypesToReload.bLocalisation = false;
	m_AssetTypesToReload.bPlaylists = false;
	m_AssetTypesToReload.bAimAssistSettings = false;
	m_AssetTypesToReload.bMaterials = false;
	m_AssetTypesToReload.bRPaks = false;
	m_AssetTypesToReload.bWeaponSettings = false;
	m_AssetTypesToReload.bPlayerSettings = false;
	m_AssetTypesToReload.bAiSettings = false;
	m_AssetTypesToReload.bDamageDefs = false;
	m_AssetTypesToReload.bModels = false;

	// clean up stuff from mods before we unload
	fs::remove_all(GetCompiledAssetsPath());

	// TODO: remove, should only reload required overrides, and don't do it here
	g_CustomAudioManager.ClearAudioOverrides(); 
}

std::string ModManager::NormaliseModFilePath(const fs::path path)
{
	std::string str = path.lexically_normal().string();

	// force to lowercase
	for (char& c : str)
		if (c <= 'Z' && c >= 'A')
			c = c - ('Z' - 'z');

	return str;
}

void ModManager::CompileAssetsForFile(const char* filename)
{
	size_t fileHash = STR_HASH(NormaliseModFilePath(fs::path(filename)));

	if (fileHash == m_hScriptsRsonHash)
		BuildScriptsRson();
	//else if (fileHash == m_hPdefHash)
	//{
	//	// BuildPdef(); todo
	//}
	else if (fileHash == m_hKBActHash)
		BuildKBActionsList();
	else
	{
		// check if we should build keyvalues, depending on whether any of our mods have patch kvs for this file
		for (Mod& mod : GetMods())
		{
			if (!mod.m_bEnabled)
				continue;

			if (mod.KeyValues.find(fileHash) != mod.KeyValues.end())
			{
				TryBuildKeyValues(filename);
				return;
			}
		}
	}
}

void ConCommand_reload_mods(const CCommand& args)
{
	g_pModManager->LoadMods();
}

fs::path GetModFolderPath()
{
	return GetNorthstarPrefix() / MOD_FOLDER_SUFFIX;
}
fs::path GetRemoteModFolderPath()
{
	return GetNorthstarPrefix() / REMOTE_MOD_FOLDER_SUFFIX;
}
fs::path GetCompiledAssetsPath()
{
	return GetNorthstarPrefix() / COMPILED_ASSETS_SUFFIX;
}

ON_DLL_LOAD_RELIESON("engine.dll", ModManager, (ConCommand, MasterServer), (CModule module))
{
	g_pModManager = new ModManager;

	RegisterConCommand("reload_mods", ConCommand_reload_mods, "reloads mods", FCVAR_NONE);
}
