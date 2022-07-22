#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

// add headers that you want to pre-compile here

#include <Windows.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Psapi.h>
#include <comdef.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <regex>
#include <thread>
#include <set>
#include <chrono>

#include "spdlog/spdlog.h"
#endif
