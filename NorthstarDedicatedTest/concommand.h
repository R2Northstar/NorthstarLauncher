#pragma once

// From Source SDK
class ConCommandBase;
class IConCommandBaseAccessor
{
  public:
	// Flags is a combination of FCVAR flags in cvar.h.
	// hOut is filled in with a handle to the variable.
	virtual bool RegisterConCommandBase(ConCommandBase* pVar) = 0;
};

class CCommand
{
  public:
	CCommand() = delete;

	int64_t ArgC() const;
	const char** ArgV() const;
	const char* ArgS() const;				  // All args that occur after the 0th arg, in string form
	const char* GetCommandString() const;	  // The entire command in string form, including the 0th arg
	const char* operator[](int nIndex) const; // Gets at arguments
	const char* Arg(int nIndex) const;		  // Gets at arguments

	static int MaxCommandLength();

  private:
	enum
	{
		COMMAND_MAX_ARGC = 64,
		COMMAND_MAX_LENGTH = 512,
	};

	int64_t m_nArgc;
	int64_t m_nArgv0Size;
	char m_pArgSBuffer[COMMAND_MAX_LENGTH];
	char m_pArgvBuffer[COMMAND_MAX_LENGTH];
	const char* m_ppArgv[COMMAND_MAX_ARGC];
};

inline int CCommand::MaxCommandLength() { return COMMAND_MAX_LENGTH - 1; }
inline int64_t CCommand::ArgC() const { return m_nArgc; }
inline const char** CCommand::ArgV() const { return m_nArgc ? (const char**)m_ppArgv : NULL; }
inline const char* CCommand::ArgS() const { return m_nArgv0Size ? &m_pArgSBuffer[m_nArgv0Size] : ""; }
inline const char* CCommand::GetCommandString() const { return m_nArgc ? m_pArgSBuffer : ""; }
inline const char* CCommand::Arg(int nIndex) const
{
	// FIXME: Many command handlers appear to not be particularly careful
	// about checking for valid argc range. For now, we're going to
	// do the extra check and return an empty string if it's out of range
	if (nIndex < 0 || nIndex >= m_nArgc)
		return "";
	return m_ppArgv[nIndex];
}
inline const char* CCommand::operator[](int nIndex) const { return Arg(nIndex); }

// From r5reloaded
class ConCommandBase
{
  public:
	void AddFlags(int nFlags);
	void RemoveFlags(int nFlags);
	bool HasFlags(int nFlags);
	// bool IsFlagSet(ConCommandBase* pCommandBase, int nFlags);

	void* m_pConCommandBaseVTable;		  // 0x0000
	ConCommandBase* m_pNext;			  // 0x0008
	bool m_bRegistered;					  // 0x0010
	char pad_0011[7];					  // 0x0011
	const char* m_pszName;				  // 0x0018
	const char* m_pszHelpString;		  // 0x0020
	int m_nFlags;						  // 0x0028
	ConCommandBase* s_pConCommandBases;	  // 0x002C
	IConCommandBaseAccessor* s_pAccessor; // 0x0034
};										  // Size: 0x0040

// taken from ttf2sdk
class ConCommand
{
  public:
	virtual void EngineDestructor(void) {}
	virtual bool IsCommand(void) const { return false; }
	virtual bool IsFlagSet(int flag) { return false; }
	virtual void AddFlags(int flags) {}
	virtual void RemoveFlags(int flags) {}
	virtual int GetFlags() const { return 0; }
	virtual const char* GetName(void) const { return nullptr; }
	virtual const char* GetHelpText(void) const { return nullptr; }
	virtual bool IsRegistered(void) const { return false; }
	// NOTE: there are more virtual methods here
	// NOTE: Not using the engine's destructor here because it doesn't do anything useful for us

	// This is taken from apex. The size is the same
	// as titanfall 2 (0x68). but i have not confirmed
	// if all members offsets are the same. They should
	// match but if they don't, check the reg function
	// called by the constructor and align accordingly.
	ConCommandBase m_ConCommandBase{}; // 0x0000
	void* m_nNullCallBack{};		   // 0x0040
	char m_nPad48[8]{};				   // 0x0048
	void* m_pCommandCallback{};		   // 0x0050
	void* m_pCompletionCallback{};	   // 0x0058
	int m_nCallbackFlags{};			   // 0x0060
	char m_nPad68[4]{};				   // 0x0068
};

void RegisterConCommand(const char* name, void (*callback)(const CCommand&), const char* helpString, int flags);
void InitialiseConCommands(HMODULE baseAddress);