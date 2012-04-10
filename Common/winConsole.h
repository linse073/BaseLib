//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _WINCONSOLE_H_
#define _WINCONSOLE_H_

#include <functional>
#include <WinSock2.h>

#define MAX_CMDS 20
#define CONSOLE_PROMPT "$ "
#define CONSOLE_PROMPT_SIZE (int)strlen(CONSOLE_PROMPT)
#define OUTPUT_PROMPT "> "
#define PTAIL(str) str+strlen(str), sizeof(str)-strlen(str)

class WinConsole
{
private:
	friend class ConsoleColor;
	typedef std::tr1::function<void(const char*)> ExecFunc;

public:
	static WinConsole* GetInstance();
	WinConsole(const char* title, ExecFunc execFunc);
	~WinConsole();

	void process();
	void printf(const char *_format, ...);

private:
	HANDLE stdOut;
	HANDLE stdIn;
	HANDLE stdErr;
	char inbuf[512];
	int  inpos;
	char curTabComplete[512];
	int  completeStart;
	char rgCmds[MAX_CMDS][512];
	int  iCmdIndex;
	ExecFunc m_execFunc;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	bool command;
	CRITICAL_SECTION m_criticalSection;

	void _printf(const char *s, ...);
};

#define CONSOLE WinConsole::GetInstance()

class ConsoleColor
{
public:
	ConsoleColor(WORD color);
	~ConsoleColor();
};

#endif
