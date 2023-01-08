#pragma once

/**
 * This enumeration allows to characterize result of verified mod downloading and extraction.
 * Its values match localization entries client-side, which will be displayed by Squirrel VM
 * to users if operation result is different than OK.
 **/
enum VerificationResult
{
	OK,
	FAILED,						// Generic error message, should be avoided as much as possible
	FAILED_WRITING_TO_DISK,
	MOD_FETCHING_FAILED,
	MOD_CORRUPTED,				// Downloaded archive checksum does not match verified hash
	NO_DISK_SPACE_AVAILABLE,
};
