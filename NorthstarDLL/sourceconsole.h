#pragma once
#include "pch.h"
#include "sourceinterface.h"
#include "spdlog/sinks/base_sink.h"
#include <map>

class EditablePanel
{
  public:
	virtual ~EditablePanel() = 0;
	unsigned char unknown[0x2B0];
};

struct SourceColor
{
	unsigned char R;
	unsigned char G;
	unsigned char B;
	unsigned char A;

	SourceColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	{
		R = r;
		G = g;
		B = b;
		A = a;
	}

	SourceColor()
	{
		R = 0;
		G = 0;
		B = 0;
		A = 0;
	}
};

class IConsoleDisplayFunc
{
  public:
	virtual void ColorPrint(const SourceColor& clr, const char* pMessage) = 0;
	virtual void Print(const char* pMessage) = 0;
	virtual void DPrint(const char* pMessage) = 0;
};

class CConsolePanel : public EditablePanel, public IConsoleDisplayFunc
{
};

class CConsoleDialog
{
  public:
	struct VTable
	{
		void* unknown[298];
		void (*OnCommandSubmitted)(CConsoleDialog* consoleDialog, const char* pCommand);
	};

	VTable* m_vtable;
	unsigned char unknown[0x398];
	CConsolePanel* m_pConsolePanel;
};

class CGameConsole
{
  public:
	virtual ~CGameConsole() = 0;

	// activates the console, makes it visible and brings it to the foreground
	virtual void Activate() = 0;

	virtual void Initialize() = 0;

	// hides the console
	virtual void Hide() = 0;

	// clears the console
	virtual void Clear() = 0;

	// return true if the console has focus
	virtual bool IsConsoleVisible() = 0;

	virtual void SetParent(int parent) = 0;

	bool m_bInitialized;
	CConsoleDialog* m_pConsole;
};

extern SourceInterface<CGameConsole>* g_pSourceGameConsole;

// spdlog logger
class SourceConsoleSink : public spdlog::sinks::base_sink<std::mutex>
{
  private:
	std::map<spdlog::level::level_enum, SourceColor> m_LogColours = {
		{spdlog::level::trace, SourceColor(0, 255, 255, 255)},
		{spdlog::level::debug, SourceColor(0, 255, 255, 255)},
		{spdlog::level::info, SourceColor(255, 255, 255, 255)},
		{spdlog::level::warn, SourceColor(255, 255, 0, 255)},
		{spdlog::level::err, SourceColor(255, 150, 150, 255)}, // i changed this red to be different to the critical red
		{spdlog::level::critical, SourceColor(255, 0, 0, 255)},
		{spdlog::level::off, SourceColor(0, 0, 0, 0)}};

	// this map is used to print coloured tags (strings in the form "[<tag>]") to the console
	// if you add stuff to this, mimic the changes in logging.h
	// clang-format off
	std::map<std::string, SourceColor> m_tags = {
		// UI is light blue, SV is pink, CL is light green
		{"UI SCRIPT", SourceColor(100, 255, 255, 255)},
		{"CL SCRIPT", SourceColor(100, 255, 100, 255)},
		{"SV SCRIPT", SourceColor(255, 100, 255, 255)},
		// native is just a darker version of script
		{"UI NATIVE", SourceColor(50, 150, 150, 255)},
		{"CL NATIVE", SourceColor(50, 150, 50, 255)},
		{"SV NATIVE", SourceColor(150, 50, 150, 255)},
		// cool launcher things (filesystem, etc.) add more as they come up
		// finding unique colours is hard
		{"FS NATIVE", SourceColor(0, 150, 150, 255)}, // dark cyan sorta thing idk
		{"RP NATIVE", SourceColor(255, 190, 0, 255)}, // orange
		{"NORTHSTAR", SourceColor(66, 72, 128, 255)}, // one of the blues ripped from northstar logo
		// echo is just a bit grey
		{"echo", SourceColor(150, 150, 150, 255)}};
	// clang-format on
		

  protected:
	void sink_it_(const spdlog::details::log_msg& msg) override;
	void flush_() override;
};

void InitialiseConsoleOnInterfaceCreation();
