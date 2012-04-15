#include "winConsole.h"
#include "LuaWrap.h"
#include <string>

INIT_DEFINE_LUA_FUNC

static void printf(lua_State* L, const char* s)
{
	CONSOLE->printf(s);
}

static void enumlua(lua_State* L, std::string path)
{
	if (*path.rbegin()!='\\' && *path.rbegin()!='/')
		path += '\\';
	std::string file = path + "*";
	WIN32_FIND_DATAA fd;  
	HANDLE hFindFile = FindFirstFileA(file.c_str(), &fd);  
	if (hFindFile == INVALID_HANDLE_VALUE)  
	{
		LOG("Fail to call FindFirstFile.");
		return;  
	}  
	do
	{
		std::string find = path + fd.cFileName;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (strcmp(fd.cFileName, ".")==0 || strcmp(fd.cFileName, "..")==0)
				continue;
			else
				enumlua(L, find);
		}
		const char* ext = strrchr(fd.cFileName, '.');
		if (ext && strcmp(ext, ".lua")==0)
			luaL_dofile(L, find.c_str());
	} while (FindNextFileA(hFindFile, &fd));
	FindClose(hFindFile);  
}

static void luascript(lua_State* L, const char* path)
{
	enumlua(L, path);
}

static void chello(lua_State* L, const char** s, int* i)
{
	*s = "hello from c++.";
	*i = 1;
}

LUA_FUNC_EXPORT(printf, printf)
LUA_FUNC_EXPORT(luascript, luascript)
LUA_FUNC_EXPORT(chello, chello)

LUAMOD_API int open_elib(lua_State* L)
{
	luaL_register(L, "elib", lib);
	return 1;
}