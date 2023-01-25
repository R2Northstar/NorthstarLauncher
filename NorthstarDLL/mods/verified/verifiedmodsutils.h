#include <filesystem>
#include <libzip/include/zip.h>

/**
 * Adds a `"AutoDownloaded": true` entry into json file located at `filepath`.
 **/
bool MarkModJson(std::filesystem::path filepath);

/**
 * cURL method to write data to disk.
 **/
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream);

/**
 * Returns uncompressed size of all mod files from a Thunderstore archive.
 * This excludes all Thunderstore-related files.
 **/
int get_mod_archive_content_size(zip_t* zip);

/**
 * Computes the hash of the archive located at `path`, and compares it
 * with `expectedHash`: this will return true if the two are equal,
 * false otherwise.
 * SHA256 code shamelessly stolen from https://stackoverflow.com/a/74671723/11243782
 **/
bool check_mod_archive_checksum(std::filesystem::path path, std::string expectedHash);
