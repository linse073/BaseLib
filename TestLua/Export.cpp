#include "winConsole.h"
#include "LuaWrap.h"
#include <string>

INIT_DEFINE_LUA_FUNC

DEFINE_LUA_FUNC_1(printf, const char*)
{
	CONSOLE->printf(arg1);
	return 0;
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

DEFINE_LUA_FUNC_1(luascript, const char*)
{
	enumlua(L, arg1);
	return 0;
}

LUAMOD_API int open_elib(lua_State* L)
{
	luaL_register(L, "elib", lib);
	return 1;
}