// TestConsole.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winConsole.h"
#include "Log.h"
#include <signal.h>

using std::tr1::bind;
using namespace std::tr1::placeholders;

void processFunc(const char* cmd)
{
	ConsoleColor consoleColor(FOREGROUND_BLUE|FOREGROUND_INTENSITY);
	CONSOLE->printf("%s", cmd);
}

bool run = true;

BOOL WINAPI HandlerRoutine(DWORD ctrlTpye)
{
	run = false;
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	WinConsole console("TestConsole", bind(&processFunc, _1));
	if (SetConsoleCtrlHandler(HandlerRoutine, TRUE) == 0)
		LOG("Fail to call SetConsoleCtrlHandler.");
	LOG("Virtula console.");
	while (run)
	{
		console.process();
	}

	return 0;
}