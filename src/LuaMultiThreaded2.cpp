#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>
extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}
#include "Utils.h"

using LuaState = lua_State;





/** All the threads that are currently alive. */
std::vector<std::thread> gThreads;





/** Dumps the contents of the Lua stack to the specified ostream. */
static void dumpLuaStack(LuaState * L, std::ostream & aDest)
{
	aDest << "Lua stack contents:" << std::endl;
	for (int i = lua_gettop(L); i >= 0; --i)
	{
		aDest << "  " << i << "\t";
		aDest << lua_typename(L, lua_type(L, i)) << "\t";
		aDest << lua_getstring(L, i).c_str() << std::endl;
	}
	aDest << "(stack dump completed)" << std::endl;
}





/** Dumps the call stack to the specified ostream. */
static void dumpLuaTraceback(LuaState * L, std::ostream & aDest)
{
	luaL_traceback(L, L, "Stack trace: ", 0);
	aDest << lua_getstring(L, -1).c_str() << std::endl;
	lua_pop(L, 1);
	return;
}





/** Called by Lua when it encounters an unhandler error in the script file. */
extern "C" int errorHandler(LuaState * L)
{
	auto err = lua_getstring(L, -1);
	std::cerr << "Caught an error: " << err << std::endl;
	dumpLuaStack(L, std::cerr);
	exit(1);
}






extern "C" static int threadNew(LuaState * aState)
{
	static std::recursive_mutex mtx;
	std::scoped_lock lock(mtx);
	luaL_checktype(aState, 1, LUA_TFUNCTION);
	lua_pushvalue(aState, 1);                             // Push a copy of the fn to the top of the stack...
	auto luaFnRef = luaL_ref(aState, LUA_REGISTRYINDEX);  // ... move it to the registry...
	auto luaThread = lua_newthread(aState);
	lua_pushcfunction(aState, errorHandler);
	lua_rawgeti(luaThread, LUA_REGISTRYINDEX, luaFnRef);  // ... push it onto the new thread's stack...
	luaL_unref(aState, LUA_REGISTRYINDEX, luaFnRef);      // ... and remove it from the registry

	// Start the new thread:
	gThreads.emplace_back(
		[luaThread]()
		{
			auto numParams = lua_gettop(luaThread) - 2;
			lua_pcall(luaThread, numParams, LUA_MULTRET, 1);
		}
	);
	return 0;
}





/** Provides the thread.sleep() function.
Parameter: the number of seconds to sleep for (floating-point number). */
extern "C" static int threadSleep(LuaState * aState)
{
	auto seconds = luaL_checknumber(aState, 1);
	std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(seconds * 1000)));
	return 0;
}





static const luaL_Reg threadFuncs[] =
{
	{"new", &threadNew},
	{"sleep", &threadSleep},
	{NULL, NULL}
};





/** Registers the thread library into the Lua VM. */
extern "C" static int luaopen_thread(LuaState * aState)
{
	luaL_newlib(aState, threadFuncs);
	return 1;
}





int main(int argc, char * argv[])
{
	if (argc <= 1)
	{
		return 0;
	}

	// Create a new Lua state:
	lua_State * L = luaL_newstate();

	// Add the libraries:
	luaL_openlibs(L);
	luaL_requiref(L, "thread", &luaopen_thread, true);
	lua_pop(L, 1);
	lua_settop(L, 0);  // Trim off all excess values left over by the reg functions

	// Store the args to the script in a separate "arg" global:
	lua_createtable(L, argc - 1, 0);
	for (int i = 0; i < argc; i++)
	{
		lua_pushstring(L, argv[i]);
		lua_rawseti(L, -2, i - 1);
	}
	lua_setglobal(L, "arg");

	// Execute the script file with appropriate args:
	lua_pushcfunction(L, errorHandler);
	auto status = luaL_loadfile(L, argv[1]);
	if (status != LUA_OK)
	{
		errorHandler(L);
	}
	for (int i = 2; i < argc; ++i)
	{
		lua_pushstring(L, argv[i]);
	}
	lua_pcall(L, argc - 2, LUA_MULTRET, 1);

	for (auto & th: gThreads)
	{
		th.join();
	}
	return 0;
}

