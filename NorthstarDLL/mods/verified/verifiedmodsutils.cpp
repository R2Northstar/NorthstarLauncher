#include "pch.h"
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <libzip/include/zip.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <fstream>

bool MarkModJson(std::filesystem::path filepath)
{
	spdlog::info("Add autodownload key to {}", filepath.generic_string());
	using namespace rapidjson;

	std::ifstream ifs {filepath.generic_string().c_str()};
	if (!ifs.is_open())
	{
		spdlog::error("Could not open manifest file for reading.");
		return false;
	}
	IStreamWrapper isw {ifs};
	Document doc {};
	doc.ParseStream(isw);

	if (doc.HasParseError())
	{
		spdlog::error("Manifest file is not formatted correctly.");
		return false;
	}

	doc.AddMember("AutoDownloaded", true, doc.GetAllocator());

	std::ofstream ofs {filepath.generic_string().c_str()};
	if (!ofs.is_open())
	{
		spdlog::error("Could not open manifest file for writing.");
		return false;
	}
	OStreamWrapper osw {ofs};
	PrettyWriter<OStreamWrapper> writer(osw);
	doc.Accept(writer);

	return true;
}

bool CreateModAuthorFile(std::string authorName, std::filesystem::path filepath) {
	std::ofstream writeStream(filepath);
	if (!writeStream.is_open())
	{
		spdlog::error("Failed to open author file.");
		return false;
	}
	writeStream << authorName;
	writeStream.close();
	return true;
}

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

int get_mod_archive_content_size(zip_t* zip)
{
	int totalSize = 0;
	int num_entries = zip_get_num_entries(zip, 0);

	for (zip_uint64_t i = 0; i < num_entries; ++i)
	{
		const char* name = zip_get_name(zip, i, 0);
		std::string modName = name;
		if (modName.back() == '/' || strcmp(name, "mods/") == 0 || modName.substr(0, 5) != "mods/")
		{
			continue;
		}

		struct zip_stat sb;
		zip_stat_index(zip, i, 0, &sb);
		totalSize += sb.size;
	}

	return totalSize;
}

bool check_mod_archive_checksum(std::filesystem::path path, std::string expectedHash)
{
	auto sha256 = [](std::string fname, std::vector<unsigned char>& hash) -> bool
	{
		std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX*)> evpCtx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
		EVP_DigestInit_ex(evpCtx.get(), EVP_sha256(), nullptr);

		constexpr size_t buffer_size {1 << 12};
		std::vector<char> buffer(buffer_size, '\0');

		std::ifstream fp(fname, std::ios::binary);
		if (!fp.is_open())
		{
			spdlog::error("Unable to open {}.", fname);
			return false;
		}
		while (fp.good())
		{
			fp.read(buffer.data(), buffer_size);
			EVP_DigestUpdate(evpCtx.get(), buffer.data(), fp.gcount());
		}
		fp.close();

		hash.resize(SHA256_DIGEST_LENGTH);
		std::fill(hash.begin(), hash.end(), 0);
		unsigned int len;
		EVP_DigestFinal_ex(evpCtx.get(), hash.data(), &len);

		return true;
	};

	// Compute hash of downloaded archive.
	std::vector<unsigned char> hash;
	if (!sha256(path.generic_string(), hash))
	{
		return false;
	}
	std::stringstream out;
	for (size_t i = 0; i < hash.size(); i++)
		out << std::setfill('0') << std::setw(2) << std::hex << int(hash[i]);
	std::string archiveHash = out.str();

	// Compare computed hash with expected hash.
	return strcmp(archiveHash.c_str(), expectedHash.c_str()) == 0;
}
