#ifndef LUA_WRAP
#define LUA_WRAP

#include "lua.hpp"
#include "log.h"
#include <functional>

template <typename T>
struct LuaArg
{
	static void Push(lua_State* L, int& index, T value);
	static void To(lua_State* L, int& index, T& value);
	static void RPush(lua_State* L, T value);
	static void Get(lua_State* L, int& index, T& value);
};

template <>
struct LuaArg<bool>
{
	static void Push(lua_State* L, int& index, bool value){ ++index; lua_pushboolean(L, value); }
	static void To(lua_State* L, int& index, bool& value){ ++index; value = luaL_checkint(L, index)!=0; }
	static void RPush(lua_State* L, bool){}
	static void Get(lua_State* L, int&, bool&){}
};

template <>
struct LuaArg<bool*>
{
	static void Push(lua_State* L, int&, bool*){}
	static void To(lua_State* L, int&, bool*& value){ value = new bool(); }
	static void RPush(lua_State* L, bool*& value){ lua_pushboolean(L, *value); delete value; value = NULL; }
	static void Get(lua_State* L, int& index, bool* value){ *value = (luaL_checkint(L, index)!=0); ++index; }
};

template <>
struct LuaArg<const char*>
{
	static void Push(lua_State* L, int& index, const char* value){ ++index; lua_pushstring(L, value); }
	static void To(lua_State* L, int& index, const char*& value){ ++index; value = luaL_checkstring(L, index); }
	static void RPush(lua_State* L, const char*){}
	static void Get(lua_State* L, int&, const char*&){}
};

template <>
struct LuaArg<const char**>
{
	static void Push(lua_State* L, int&, const char**){}
	static void To(lua_State* L, int&, const char**& value){ value = new const char*(); }
	static void RPush(lua_State* L, const char**& value){ lua_pushstring(L, *value); delete value; value = NULL; }
	static void Get(lua_State* L, int& index, const char** value){ *value = luaL_checkstring(L, index); ++index; }
};

template <>
struct LuaArg<int>
{
	static void Push(lua_State* L, int& index, int value){ ++index; lua_pushinteger(L, value); }
	static void To(lua_State* L, int& index, int& value){ ++index; value = luaL_checkint(L, index); }
	static void RPush(lua_State* L, int){}
	static void Get(lua_State* L, int&, int&){}
};

template <>
struct LuaArg<int*>
{
	static void Push(lua_State* L, int&, int*){}
	static void To(lua_State* L, int&, int*& value){ value = new int(); }
	static void RPush(lua_State* L, int*& value){ lua_pushinteger(L, *value); delete value; value = NULL; }
	static void Get(lua_State* L, int& index, int* value){ *value = luaL_checkint(L, index); ++index; }
};

template <>
struct LuaArg<double>
{
	static void Push(lua_State* L, int& index, double value){ ++index; lua_pushnumber(L, value); }
	static void To(lua_State* L, int& index, double& value){ ++index; value = luaL_checknumber(L, index); }
	static void RPush(lua_State* L, double){}
	static void Get(lua_State* L, int&, double&){}
};

template <>
struct LuaArg<double*>
{
	static void Push(lua_State* L, int&, double*){}
	static void To(lua_State* L, int&, double*& value){ value = new double(); }
	static void RPush(lua_State* L, double*& value){ lua_pushnumber(L, *value); delete value; value = NULL; }
	static void Get(lua_State* L, int& index, double* value){ *value = luaL_checknumber(L, index); ++index; }
};

class CLuaWrap
{
private:
	typedef std::tr1::function<int(lua_State*)> Lib;

public:
	CLuaWrap();
	~CLuaWrap();

	void OpenLib(Lib lib); 
	void Command(const char* cmd);
	void Call(const char* func)
	{
		PushFunc(func);
		if (lua_pcall(m_L, 0, 0, 0))
		{
			LOG("Fail to call function[%s], error[%s].", func, lua_tostring(m_L, -1));
			lua_pop(m_L, 1);
			return;
		}
	}
	template <typename T>
	void Call(const char* func, T arg1)
	{
		const int totalarg = 1;
		PushFunc(func);
		int index = 0;
		LuaArg<T>::Push(m_L, index, arg1);
		if (lua_pcall(m_L, index, totalarg-index, 0))
		{
			LOG("Fail to call function[%s], error[%s].", func, lua_tostring(m_L, -1));
			lua_pop(m_L, 1);
			return;
		}
		index -= totalarg;
		LuaArg<T>::Get(m_L, index, arg1);
	}
	template <typename T1, typename T2>
	void Call(const char* func, T1 arg1, T2 arg2)
	{
		const int totalarg = 2;
		PushFunc(func);
		int index = 0;
		LuaArg<T1>::Push(m_L, index, arg1);
		LuaArg<T2>::Push(m_L, index, arg2);
		if (lua_pcall(m_L, index, totalarg-index, 0))
		{
			LOG("Fail to call function[%s], error[%s].", func, lua_tostring(m_L, -1));
			lua_pop(m_L, 1);
			return;
		}
		index -= totalarg;
		LuaArg<T1>::Get(m_L, index, arg1);
		LuaArg<T2>::Get(m_L, index, arg2);
	}
	template <typename T1, typename T2, typename T3>
	void Call(const char* func, T1 arg1, T2 arg2, T3 arg3)
	{
		const int totalarg = 3;
		PushFunc(func);
		int index = 0;
		LuaArg<T1>::Push(m_L, index, arg1);
		LuaArg<T2>::Push(m_L, index, arg2);
		LuaArg<T3>::Push(m_L, index, arg3);
		if (lua_pcall(m_L, index, totalarg-index, 0))
		{
			LOG("Fail to call function[%s], error[%s].", func, lua_tostring(m_L, -1));
			lua_pop(m_L, 1);
			return;
		}
		index -= totalarg;
		LuaArg<T1>::Get(m_L, index, arg1);
		LuaArg<T2>::Get(m_L, index, arg2);
		LuaArg<T3>::Get(m_L, index, arg3);
	}
	template <typename T1, typename T2, typename T3, typename T4>
	void Call(const char* func, T1 arg1, T2 arg2, T3 arg3, T4 arg4)
	{
		const int totalarg = 4;
		PushFunc(func);
		int index = 0;
		LuaArg<T1>::Push(m_L, index, arg1);
		LuaArg<T2>::Push(m_L, index, arg2);
		LuaArg<T3>::Push(m_L, index, arg3);
		LuaArg<T4>::Push(m_L, index, arg4);
		if (lua_pcall(m_L, index, totalarg-index, 0))
		{
			LOG("Fail to call function[%s], error[%s].", func, lua_tostring(m_L, -1));
			lua_pop(m_L, 1);
			return;
		}
		index -= totalarg;
		LuaArg<T1>::Get(m_L, index, arg1);
		LuaArg<T2>::Get(m_L, index, arg2);
		LuaArg<T3>::Get(m_L, index, arg3);
		LuaArg<T4>::Get(m_L, index, arg4);
	}
	template <typename T1, typename T2, typename T3, typename T4, typename T5>
	void Call(const char* func, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
	{
		const int totalarg = 5;
		PushFunc(func);
		int index = 0;
		LuaArg<T1>::Push(m_L, index, arg1);
		LuaArg<T2>::Push(m_L, index, arg2);
		LuaArg<T3>::Push(m_L, index, arg3);
		LuaArg<T4>::Push(m_L, index, arg4);
		LuaArg<T5>::Push(m_L, index, arg5);
		if (lua_pcall(m_L, index, totalarg-index, 0))
		{
			LOG("Fail to call function[%s], error[%s].", func, lua_tostring(m_L, -1));
			lua_pop(m_L, 1);
			return;
		}
		index -= totalarg;
		LuaArg<T1>::Get(m_L, index, arg1);
		LuaArg<T2>::Get(m_L, index, arg2);
		LuaArg<T3>::Get(m_L, index, arg3);
		LuaArg<T4>::Get(m_L, index, arg4);
		LuaArg<T5>::Get(m_L, index, arg5);
	}

private:
	void PushFunc(const char* func);

	lua_State* m_L;
};

inline int LuaFunc(lua_State* L, void (*func)(lua_State*))
{
	const int totalarg = 0;
	func(L);
	return 0;
}

template <typename T>
int LuaFunc(lua_State* L, void (*func)(lua_State*, T))
{
	const int totalarg = 1;
	int index = 0;
	T arg1=T();
	LuaArg<T>::To(L, index, arg1);
	func(L, arg1);
	LuaArg<T>::RPush(L, arg1);
	return totalarg-index;
}

template <typename T1, typename T2>
int LuaFunc(lua_State* L, void (*func)(lua_State*, T1, T2))
{
	const int totalarg = 2;
	int index = 0;
	T1 arg1=T1(); T2 arg2=T2();
	LuaArg<T1>::To(L, index, arg1);
	LuaArg<T2>::To(L, index, arg2);
	func(L, arg1, arg2);
	LuaArg<T1>::RPush(L, arg1);
	LuaArg<T2>::RPush(L, arg2);
	return totalarg-index;
}

template <typename T1, typename T2, typename T3>
int LuaFunc(lua_State* L, void (*func)(lua_State*, T1, T2, T3))
{
	const int totalarg = 3;
	int index = 0;
	T1 arg1=T1(); T2 arg2=T2(); T3 arg3=T3();
	LuaArg<T1>::To(L, index, arg1);
	LuaArg<T2>::To(L, index, arg2);
	LuaArg<T3>::To(L, index, arg3);
	func(L, arg1, arg2, arg3);
	LuaArg<T1>::RPush(L, arg1);
	LuaArg<T2>::RPush(L, arg2);
	LuaArg<T3>::RPush(L, arg3);
	return totalarg-index;
}

template <typename T1, typename T2, typename T3, typename T4>
int LuaFunc(lua_State* L, void (*func)(lua_State*, T1, T2, T3, T4))
{
	const int totalarg = 4;
	int index = 0;
	T1 arg1=T1(); T2 arg2=T2(); T3 arg3=T3(); T4 arg4=T4();
	LuaArg<T1>::To(L, index, arg1);
	LuaArg<T2>::To(L, index, arg2);
	LuaArg<T3>::To(L, index, arg3);
	LuaArg<T4>::To(L, index, arg4);
	func(L, arg1, arg2, arg3, arg4);
	LuaArg<T1>::RPush(L, arg1);
	LuaArg<T2>::RPush(L, arg2);
	LuaArg<T3>::RPush(L, arg3);
	LuaArg<T4>::RPush(L, arg4);
	return totalarg-index;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
int LuaFunc(lua_State* L, void (*func)(lua_State*, T1, T2, T3, T4, T5), int retnum)
{
	const int totalarg = 5;
	int index = 0;
	T1 arg1=T1(); T2 arg2=T2(); T3 arg3=T3(); T4 arg4=T4(); T5 arg5=T5();
	LuaArg<T1>::To(L, index, arg1);
	LuaArg<T2>::To(L, index, arg2);
	LuaArg<T3>::To(L, index, arg3);
	LuaArg<T4>::To(L, index, arg4);
	LuaArg<T5>::To(L, index, arg5);
	func(L, arg1, arg2, arg3, arg4, arg5);
	LuaArg<T1>::RPush(L, arg1);
	LuaArg<T2>::RPush(L, arg2);
	LuaArg<T3>::RPush(L, arg3);
	LuaArg<T4>::RPush(L, arg4);
	LuaArg<T5>::RPush(L, arg5);
	return totalarg-index;
}

#define MAX_LUA_REG 100

class CLuaAddFunc
{
public:
	CLuaAddFunc(luaL_Reg* lib, const char* name, lua_CFunction func)
	{
		int i = 0;
		for (; i<MAX_LUA_REG-1; ++i)
		{
			if (lib[i].name==NULL && lib[i].func==NULL)
			{
				lib[i].name = name;
				lib[i].func = func;
				break;
			}
		}
		if (i == MAX_LUA_REG-1)
			LOG("Register too many function.");
	}
};

#define INIT_DEFINE_LUA_FUNC	\
	static luaL_Reg lib[MAX_LUA_REG] = {0};

#define LUA_FUNC_EXPORT(luafunc, func)	\
	static int luafunc(lua_State* L)	\
	{									\
		return LuaFunc(L, func);		\
	}									\
	static CLuaAddFunc func##_addtolib(lib, #luafunc, luafunc);	

#endif