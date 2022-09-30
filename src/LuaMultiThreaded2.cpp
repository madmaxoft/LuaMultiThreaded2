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





/** The name of the thread's Lua object's metatable within the Lua registry.
Every thread object that is pushed to the Lua side has the metatable of this name set to it. */
static const char * THREAD_METATABLE_NAME = "std::thread *";





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

	// Push the (currently empty) thread object to the Lua side
	auto threadObj = reinterpret_cast<std::thread **>(lua_newuserdata(aState, sizeof(std::thread **)));
	luaL_setmetatable(aState, THREAD_METATABLE_NAME);

	// Start the new thread:
	*threadObj = new std::thread(
		[luaThread]()
		{
			auto numParams = lua_gettop(luaThread) - 2;
			lua_pcall(luaThread, numParams, LUA_MULTRET, 1);
		}
	);
	return 1;
}





/** Provides the thread.sleep() function.
Parameter: the number of seconds to sleep for (floating-point number). */
extern "C" static int threadSleep(LuaState * aState)
{
	auto seconds = luaL_checknumber(aState, 1);
	std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(seconds * 1000)));
	return 0;
}





/** Implements the thread:join() function.
Joins the specified thread.
Errors if asked to join the current thread. */
extern "C" static int threadObjJoin(LuaState * aState)
{
	auto threadObj = reinterpret_cast<std::thread **>(luaL_checkudata(aState, 1, THREAD_METATABLE_NAME));
	if (threadObj == nullptr)
	{
		luaL_argerror(aState, 0, "`thread' expected");
		return 0;
	}
	if (*threadObj == nullptr)
	{
		luaL_argerror(aState, 0, "thread already joined");
		return 0;
	}
	if ((*threadObj)->get_id() == std::this_thread::get_id())
	{
		luaL_argerror(aState, 0, "`thread' must not be the current thread");
		return 0;
	}
	(*threadObj)->join();
	*threadObj = nullptr;
	return 0;
}





/** Implements the thread:id() function.
Returns the thread's ID, as an implementation-dependent detail.
The ID is guaranteed to be unique within a single process at any single time moment (but not within multiple time moments). */
extern "C" static int threadObjID(LuaState * aState)
{
	auto threadObj = reinterpret_cast<std::thread **>(luaL_checkudata(aState, 1, THREAD_METATABLE_NAME));
	if (threadObj == nullptr)
	{
		luaL_argerror(aState, 0, "`thread' expected");
		return 0;
	}
	if (*threadObj == nullptr)
	{
		luaL_argerror(aState, 0, "thread already joined");
		return 0;
	}
	if ((*threadObj)->get_id() == std::this_thread::get_id())
	{
		luaL_argerror(aState, 0, "`thread' must not be the current thread");
		return 0;
	}
	std::stringstream ss;
	ss << (*threadObj)->get_id();
	auto str = ss.str();
	lua_pushlstring(aState, str.data(), str.size());
	return 1;
}





/** Implements the thread.currentid() function.
Returns the current thread's ID. */
extern "C" static int threadCurrentID(LuaState * aState)
{
	std::stringstream ss;
	ss << std::this_thread::get_id();
	auto str = ss.str();
	lua_pushlstring(aState, str.data(), str.size());
	return 1;
}





/** Called when the Lua side GC's the thread object.
Joins the thread, if not already joined. */
extern "C" static int threadObjGc(LuaState * aState)
{
	auto threadObj = reinterpret_cast<std::thread **>(luaL_checkudata(aState, 1, THREAD_METATABLE_NAME));
	// We shouldn't get an invalid thread object, but let's check nevertheless:
	if (threadObj == nullptr)
	{
		luaL_argerror(aState, 0, "`thread' expected");
		return 0;
	}
	if (*threadObj == nullptr)
	{
		return 0;
	}
	if ((*threadObj)->get_id() == std::this_thread::get_id())
	{
		// Current thread is GC-ing self? No idea if that is allowed to happen, but we don't care; just don't join
		return 0;
	}
	(*threadObj)->join();
	*threadObj = nullptr;
	return 0;
}





/** The functions in the thread library. */
static const luaL_Reg threadFuncs[] =
{
	{"new", &threadNew},
	{"sleep", &threadSleep},
	{"currentid", &threadCurrentID},
	{NULL, NULL}
};





/** The functions of the thread object. */
static const luaL_Reg threadObjFuncs[] =
{
	{"join", &threadObjJoin},
	{"id",   &threadObjID},
	{"__gc", &threadObjGc},
	{NULL, NULL}
};





/** Registers the thread library into the Lua VM. */
extern "C" static int luaopen_thread(LuaState * aState)
{
	luaL_newlib(aState, threadFuncs);

	// Register the metatable for std::thread objects:
	luaL_newmetatable(aState, THREAD_METATABLE_NAME);
	lua_pushvalue(aState, -1);
	lua_setfield(aState, -2, "__index");  // metatable.__index = metatable
	luaL_setfuncs(aState, threadObjFuncs, 0);  // Add the object functions to the table
	lua_pop(aState, 1);  // pop the new metatable

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

	return 0;
}

