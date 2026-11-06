#include "server/r2server.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>

struct CUserCmd
{
	int command_number;
	int tick_count;
	float command_time;
	char gap0C[0x90];
	float frametime;
	char gapA0[0x98];
};
// clang-format off
static_assert(offsetof(CUserCmd, command_number) 	== 0x0);
static_assert(offsetof(CUserCmd, tick_count) 		== 0x04);
static_assert(offsetof(CUserCmd, command_time) 		== 0x08);
static_assert(offsetof(CUserCmd, frametime) 		== 0x9C);
static_assert(	sizeof(CUserCmd) 					== 0x138);
// clang-format on

struct CCommandContext
{
	// CUtlVector
	CUserCmd* cmds;
	int allocationCount;
	char gap0C[0xC];
	int size;
	char gap1C[4];
	//

	int numcmds;
	int totalcmds;
	int dropped_packets;
	char paused;
	char gap2D[3];
};
// clang-format off
static_assert(offsetof(CCommandContext, cmds) 				== 0x0);
static_assert(offsetof(CCommandContext, allocationCount) 	== 0x08);
static_assert(offsetof(CCommandContext, size) 				== 0x18);
static_assert(offsetof(CCommandContext, numcmds) 			== 0x20);
static_assert(offsetof(CCommandContext, totalcmds) 			== 0x24);
static_assert(offsetof(CCommandContext, dropped_packets) 	== 0x28);
static_assert(offsetof(CCommandContext, paused) 			== 0x2C);
static_assert(	sizeof(CCommandContext) 					== 0x30);
// clang-format on

constexpr size_t CONTEXT_DATA = 0x1FA8;
constexpr size_t CONTEXT_COUNT = 0x1FC0;
constexpr size_t LAST_EXECUTED = 0x2EAC;

using QueueCommandsFn = void(__fastcall*)(CBasePlayer*, const CUserCmd*, int, int, int, char);
QueueCommandsFn QueueCommands = nullptr;

int CountRecoverable(const uintptr_t player, const CUserCmd* commands, int newCommands, int totalCommands)
{
	const CCommandContext* contexts = *(CCommandContext**)(player + CONTEXT_DATA);
	const int contextCount = *(int*)(player + CONTEXT_COUNT);
	int lastExecuted = *(int*)(player + LAST_EXECUTED);

	for (int i = 0; i < contextCount; ++i)
	{
		const CCommandContext& context = contexts[i];
		// the vanilla gather only walks when newCount >= 1
		if (context.numcmds == 0)
			continue;

		lastExecuted = std::max(lastExecuted, context.cmds->command_number);
	}

	for (int i = 1; i < totalCommands; ++i)
	{
		if (commands[i].command_number < 1)
			return 0;
		if (commands[i].command_number >= commands[i - 1].command_number)
			return 0;
	}

	int added = 0;
	for (int i = newCommands; i < totalCommands; ++i)
	{
		const CUserCmd& command = commands[i];
		if (command.command_number <= lastExecuted)
			break;
		if (command.tick_count <= 0 || command.command_time <= 0.0f || command.frametime <= 0.0f)
			break;
		++added;
	}
	return added;
}

void h_QueueCommands(CBasePlayer* player, const CUserCmd* commands, int newCommands, int totalCommands, int droppedPackets, char paused)
{
	if (paused || totalCommands < 1 || newCommands < 1 || totalCommands <= newCommands)
		return QueueCommands(player, commands, newCommands, totalCommands, droppedPackets, paused);
	if (newCommands > 15 || totalCommands > newCommands + 7)
		return QueueCommands(player, commands, newCommands, totalCommands, droppedPackets, paused);

	const uintptr_t uPlayer = (uintptr_t)player;
	const int added = CountRecoverable(uPlayer, commands, newCommands, totalCommands);
	const int before = *(int*)(uPlayer + CONTEXT_COUNT);
	QueueCommands(player, commands, newCommands + added, totalCommands, droppedPackets, paused);

	if (added <= 0)
		return;
	// allocation can be refused
	if (*(int*)(uPlayer + CONTEXT_COUNT) != before + 1)
		return;

	CCommandContext* contexts = *(CCommandContext**)(uPlayer + CONTEXT_DATA);
	contexts[before].dropped_packets = std::max(droppedPackets - added, 0);
}

ON_DLL_LOAD("server.dll", UsercmdRecoveryServer, (CModule module))
{
	QueueCommands = module.Offset(0x5A81C0).RCast<QueueCommandsFn>();
	HookAttach((void**)&QueueCommands, (void*)h_QueueCommands);
}
