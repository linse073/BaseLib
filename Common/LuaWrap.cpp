#include "LuaWrap.h"

CLuaWrap::CLuaWrap()
{
	m_L = luaL_newstate();
	luaL_openlibs(m_L);
}

CLuaWrap::~CLuaWrap()
{
	lua_close(m_L);
}

void CLuaWrap::OpenLib(Lib lib)
{
	lib(m_L);
}

void CLuaWrap::Command(const char* cmd)
{
	int error = luaL_dostring(m_L, cmd);
	if (error)
	{
		LOG("Fail to execute command[%s], error[%s].", cmd, lua_tostring(m_L, -1));
		lua_pop(m_L, 1);
	}
}

void CLuaWrap::PushFunc(const char* func)
{
	const char* delimit = ".";
	char buffer[512] = "";
	strcpy_s(buffer, sizeof(buffer), func);
	char* next = NULL;
	char* token = strtok_s(buffer, delimit, &next);
	lua_getglobal(m_L, token);
	token = strtok_s(NULL, delimit, &next);
	while (token)
	{
		lua_getfield(m_L, -1, token);
		token = strtok_s(NULL, delimit, &next);
	}
}