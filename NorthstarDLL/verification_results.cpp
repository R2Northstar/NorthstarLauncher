#include "pch.h"
#include <string>
#include <verification_results.h>

std::array<std::string, 7> _resultValues {
	"OK", "FAILED", "FAILED_READING_ARCHIVE", "FAILED_WRITING_TO_DISK", "MOD_FETCHING_FAILED", "MOD_CORRUPTED", "NO_DISK_SPACE_AVAILABLE"};

const char* GetVerifiedModErrorString( VerificationResult error ) {
	return _resultValues[error].c_str();
}
