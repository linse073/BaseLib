// TestLua.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winConsole.h"
#include "LuaWrap.h"
#include <signal.h>

using std::tr1::bind;
using namespace std::tr1::placeholders;

LUAMOD_API int open_elib(lua_State* L);

bool run = true;
BOOL WINAPI HandlerRoutine(DWORD ctrlTpye)
{
	run = false;
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	CLuaWrap luaWrap;
	WinConsole console("TestLua", bind(&CLuaWrap::Command, &luaWrap, _1));
	if (SetConsoleCtrlHandler(HandlerRoutine, TRUE) == 0)
		LOG("Fail to call SetConsoleCtrlHandler.");
	luaWrap.OpenLib(open_elib);
	luaWrap.Call("elib.luascript", "../Script");
	while (run)
		console.process();

	return 0;
}

