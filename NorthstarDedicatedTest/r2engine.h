#pragma once

// use the R2 namespace for game funcs
namespace R2
{
	// Cbuf
	enum class ECommandTarget_t
	{
		CBUF_FIRST_PLAYER = 0,
		CBUF_LAST_PLAYER = 1, // MAX_SPLITSCREEN_CLIENTS - 1, MAX_SPLITSCREEN_CLIENTS = 2
		CBUF_SERVER = CBUF_LAST_PLAYER + 1,

		CBUF_COUNT,
	};

	enum class cmd_source_t
	{
		// Added to the console buffer by gameplay code.  Generally unrestricted.
		kCommandSrcCode,

		// Sent from code via engine->ClientCmd, which is restricted to commands visible
		// via FCVAR_CLIENTCMD_CAN_EXECUTE.
		kCommandSrcClientCmd,

		// Typed in at the console or via a user key-bind.  Generally unrestricted, although
		// the client will throttle commands sent to the server this way to 16 per second.
		kCommandSrcUserInput,

		// Came in over a net connection as a clc_stringcmd
		// host_client will be valid during this state.
		//
		// Restricted to FCVAR_GAMEDLL commands (but not convars) and special non-ConCommand
		// server commands hardcoded into gameplay code (e.g. "joingame")
		kCommandSrcNetClient,

		// Received from the server as the client
		//
		// Restricted to commands with FCVAR_SERVER_CAN_EXECUTE
		kCommandSrcNetServer,

		// Being played back from a demo file
		//
		// Not currently restricted by convar flag, but some commands manually ignore calls
		// from this source.  FIXME: Should be heavily restricted as demo commands can come
		// from untrusted sources.
		kCommandSrcDemoFile,

		// Invalid value used when cleared
		kCommandSrcInvalid = -1
	};

	typedef ECommandTarget_t (*Cbuf_GetCurrentPlayerType)();
	extern Cbuf_GetCurrentPlayerType Cbuf_GetCurrentPlayer;

	typedef void (*Cbuf_AddTextType)(ECommandTarget_t eTarget, const char* text, cmd_source_t source);
	extern Cbuf_AddTextType Cbuf_AddText;

	typedef void (*Cbuf_ExecuteType)();
	extern Cbuf_ExecuteType Cbuf_Execute;

	// CEngine

	enum EngineQuitState
	{
		QUIT_NOTQUITTING = 0,
		QUIT_TODESKTOP,
		QUIT_RESTART
	};

	enum class EngineState_t
	{
		DLL_INACTIVE = 0, // no dll
		DLL_ACTIVE, // engine is focused
		DLL_CLOSE, // closing down dll
		DLL_RESTART, // engine is shutting down but will restart right away
		DLL_PAUSED, // engine is paused, can become active from this state
	};

	class CEngine
	{
	  public:
		virtual void unknown() {} // unsure if this is where
		virtual bool Load(bool dedicated, const char* baseDir) {}
		virtual void Unload() {}
		virtual void SetNextState(EngineState_t iNextState) {}
		virtual EngineState_t GetState() {}
		virtual void Frame() {}
		virtual double GetFrameTime() {}
		virtual float GetCurTime() {}

		EngineQuitState m_nQuitting;
		EngineState_t m_nDllState;
		EngineState_t m_nNextDllState;
		double m_flCurrentTime;
		float m_flFrameTime;
		double m_flPreviousTime;
		float m_flFilteredTime;
		float m_flMinFrameTime; // Expected duration of a frame, or zero if it is unlimited.
	};

	extern CEngine* g_pEngine;
} // namespace R2
