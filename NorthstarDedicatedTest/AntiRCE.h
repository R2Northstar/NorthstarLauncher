#pragma once
#include "NSMem.h"
#define EXITCODE_TERRIBLE 666

// The goal of AntiRCE is to prevent a potential RCE exploit from doing damage to someone's computer or stealing their data
namespace AntiRCE
{

	// Something has gone TERRIBLY wrong, a dangerous exploit may be in use
	void EmergencyReport(std::string msg);

	bool Init();

} // namespace AntiRCE