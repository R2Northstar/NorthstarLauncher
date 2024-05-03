#pragma once

#include "engine/r2engine.h"

/// Determines if we are in vanilla-compatibility mode.
class VanillaCompatibility
{
public:
	bool GetVanillaCompatibility();
};

extern VanillaCompatibility* g_pVanillaCompatibility;
