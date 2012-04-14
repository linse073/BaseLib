#ifndef LUA_WRAP
#define LUA_WRAP

#include "lua.hpp"
#include "log.h"
#include <functional>

template <typename T>
struct LuaArg
{
	LuaArg(T val):value(val){}
	void Push(lua_State* L);
	T To(lua_State* L, int index);
	T value;
};

template <>
struct LuaArg<bool>
{
	LuaArg(bool val):value(val){}
	void Push(lua_State* L){ lua_pushboolean(L, value); }
	bool To(lua_State* L, int index){ return luaL_checkint(L, index)!=0; }
	bool value;
};

template <>
struct LuaArg<const char*>
{
	LuaArg(const char* val):value(val){}
	void Push(lua_State* L){ lua_pushstring(L, value); }
	const char* To(lua_State* L, int index){ return luaL_checkstring(L, index); }
	const char* value;
};

template <>
struct LuaArg<int>
{
	LuaArg(int val):value(val){}
	void Push(lua_State* L){ lua_pushinteger(L, value); }
	int To(lua_State* L, int index){ return luaL_checkint(L, index); }
	int value;
};

template <>
struct LuaArg<double>
{
	LuaArg(double val):value(val){}
	void Push(lua_State* L){ lua_pushnumber(L, value); }
	double To(lua_State* L, int index){ return luaL_checknumber(L, index); }
	double value;
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
		}
	}
	template <typename T>
	void Call(const char* func, T arg1)
	{
		PushFunc(func);
		LuaArg<T>(arg1).Push(m_L);
		if (lua_pcall(m_L, 1, 0, 0))
		{
			LOG("Fail to call function[%s], error[%s].", func, lua_tostring(m_L, -1));
			lua_pop(m_L, 1);
		}
	}
	template <typename T1, typename T2>
	void Call(const char* func, T1 arg1, T2 arg2)
	{
		PushFunc(func);
		LuaArg<T1>(arg1).Push(m_L);
		LuaArg<T2>(arg2).Push(m_L);
		if (lua_pcall(m_L, 2, 0, 0))
		{
			LOG("Fail to call function[%s], error[%s].", func, lua_tostring(m_L, -1));
			lua_pop(m_L, 1);
		}
	}
	template <typename T1, typename T2, typename T3>
	void Call(const char* func, T1 arg1, T2 arg2, T3 arg3)
	{
		PushFunc(func);
		LuaArg<T1>(arg1).Push(m_L);
		LuaArg<T2>(arg2).Push(m_L);
		LuaArg<T3>(arg3).Push(m_L);
		if (lua_pcall(m_L, 3, 0, 0))
		{
			LOG("Fail to call function[%s], error[%s].", func, lua_tostring(m_L, -1));
			lua_pop(m_L, 1);
		}
	}
	template <typename T1, typename T2, typename T3, typename T4>
	void Call(const char* func, T1 arg1, T2 arg2, T3 arg3, T4 arg4)
	{
		PushFunc(func);
		LuaArg<T1>(arg1).Push(m_L);
		LuaArg<T2>(arg2).Push(m_L);
		LuaArg<T3>(arg3).Push(m_L);
		LuaArg<T4>(arg4).Push(m_L);
		if (lua_pcall(m_L, 4, 0, 0))
		{
			LOG("Fail to call function[%s], error[%s].", func, lua_tostring(m_L, -1));
			lua_pop(m_L, 1);
		}
	}
	template <typename T1, typename T2, typename T3, typename T4, typename T5>
	void Call(const char* func, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
	{
		PushFunc(func);
		LuaArg<T1>(arg1).Push(m_L);
		LuaArg<T2>(arg2).Push(m_L);
		LuaArg<T3>(arg3).Push(m_L);
		LuaArg<T4>(arg4).Push(m_L);
		LuaArg<T5>(arg5).Push(m_L);
		if (lua_pcall(m_L, 5, 0, 0))
		{
			LOG("Fail to call function[%s], error[%s].", func, lua_tostring(m_L, -1));
			lua_pop(m_L, 1);
		}
	}

private:
	void PushFunc(const char* func);

	lua_State* m_L;
};

template <typename T>
void GetLuaArg(lua_State* L, T& arg1)
{
	arg1 = LuaArg<T>(arg1).To(L, 1);
}
template <typename T1, typename T2>
void GetLuaArg(lua_State* L, T1& arg1, T2& arg2)
{
	arg1 = LuaArg<T1>(arg1).To(L, 1);
	arg2 = LuaArg<T2>(arg2).To(L, 2);
}
template <typename T1, typename T2, typename T3>
void GetLuaArg(lua_State* L, T1& arg1, T2& arg2, T3& arg3)
{
	arg1 = LuaArg<T1>(arg1).To(L, 1);
	arg2 = LuaArg<T2>(arg2).To(L, 2);
	arg3 = LuaArg<T2>(arg3).To(L, 3);
}
template <typename T1, typename T2, typename T3, typename T4>
void GetLuaArg(lua_State* L, T1& arg1, T2& arg2, T3& arg3, T4& arg4)
{
	arg1 = LuaArg<T1>(arg1).To(L, 1);
	arg2 = LuaArg<T2>(arg2).To(L, 2);
	arg3 = LuaArg<T3>(arg3).To(L, 3);
	arg4 = LuaArg<T4>(arg4).To(L, 4);
}
template <typename T1, typename T2, typename T3, typename T4, typename T5>
void GetLuaArg(lua_State* L, T1& arg1, T2& arg2, T3& arg3, T4& arg4, T5& arg5)
{
	arg1 = LuaArg<T1>(arg1).To(L, 1);
	arg2 = LuaArg<T2>(arg2).To(L, 2);
	arg3 = LuaArg<T3>(arg3).To(L, 3);
	arg4 = LuaArg<T4>(arg4).To(L, 4);
	arg5 = LuaArg<T5>(arg5).To(L, 5);
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

#define DEFINE_LUA_FUNC_0(func)						\
	static int func##_define(lua_State* L);			\
	static int func(lua_State* L)					\
	{												\
		return func##_define(L);					\
	}												\
	CLuaAddFunc func##_addtolib(lib, #func, func);	\
	static int func##_define(lua_State* L)

#define DEFINE_LUA_FUNC_1(func, t1)					\
	static int func##_define(lua_State* L, t1);		\
	static int func(lua_State* L)					\
	{												\
		t1 arg1;									\
		GetLuaArg(L, arg1);							\
		return func##_define(L, arg1);				\
	}												\
	CLuaAddFunc func##_addtolib(lib, #func, func);	\
	static int func##_define(lua_State* L, t1 arg1)

#define DEFINE_LUA_FUNC_2(func, t1, t2)				\
	static int func##_define(lua_State* L, t1, t2);	\
	static int func(lua_State* L)					\
	{												\
		t1 arg1; t2 arg2;							\
		GetLuaArg(L, arg1, arg2);					\
		return func##_define(L, arg1, arg2);		\
	}												\
	CLuaAddFunc func##_addtolib(lib, #func, func);	\
	static int func##_define(lua_State* L, t1 arg1, t2 arg2)

#define DEFINE_LUA_FUNC_3(func, t1, t2, t3)				\
	static int func##_define(lua_State* L, t1, t2, t3);	\
	static int func(lua_State* L)						\
	{													\
		t1 arg1; t2 arg2; t3 arg3;						\
		GetLuaArg(L, arg1, arg2, arg3);					\
		return func##_define(L, arg1, arg2, arg3);		\
	}													\
	CLuaAddFunc func##_addtolib(lib, #func, func);		\
	static int func##_define(lua_State* L, t1 arg1, t2 arg2, t3 arg3)

#define DEFINE_LUA_FUNC_4(func, t1, t2, t3, t4)				\
	static int func##_define(lua_State* L, t1, t2, t3, t4);	\
	static int func(lua_State* L)							\
	{														\
		t1 arg1; t2 arg2; t3 arg3; t4 arg4;					\
		GetLuaArg(L, arg1, arg2, arg3, arg4);				\
		return func##_define(L, arg1, arg2, arg3, arg4);	\
	}														\
	CLuaAddFunc func##_addtolib(lib, #func, func);			\
	static int func##_define(lua_State* L, t1 arg1, t2 arg2, t3 arg3, t4 arg4)

#define DEFINE_LUA_FUNC_5(func, t1, t2, t3, t4, t5)				\
	static int func##_define(lua_State* L, t1, t2, t3, t4, t5);	\
	static int func(lua_State* L)								\
	{															\
		t1 arg1; t2 arg2; t3 arg3; t4 arg4; t5 arg5;			\
		GetLuaArg(L, arg1, arg2, arg3, arg4, arg5);				\
		return func##_define(L, arg1, arg2, arg3, arg4, arg5);	\
	}															\
	CLuaAddFunc func##_addtolib(lib, #func, func);				\
	static int func##_define(lua_State* L, t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5)

#endif