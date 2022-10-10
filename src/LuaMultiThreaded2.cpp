#include <iostream>
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
static const char * MUTEX_METATABLE_NAME = "std::mutex *";





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





/** Creates a new mutex object.
Returns: The mutex object with the functions in mutexObjFuncs as methods in it's metatable. */
extern "C" static int mutexNew(LuaState * aState)
{
	lua_pushcfunction(aState, errorHandler);

	// Push the (currently empty) mutex object to the Lua side
	auto mutexObj = reinterpret_cast<std::mutex **>(lua_newuserdata(aState, sizeof(std::mutex **)));
	luaL_setmetatable(aState, MUTEX_METATABLE_NAME);

	// Create the new mutex:
	*mutexObj = new std::mutex();
	return 1;
}





/** Executes the provided function while the mutex is locked. */
extern "C" static int mutexDoWhileLocked(LuaState * aState)
{
	auto mutexObj = reinterpret_cast<std::mutex **>(luaL_checkudata(aState, 1, MUTEX_METATABLE_NAME));
	if (mutexObj == nullptr)
	{
		luaL_argerror(aState, 0, "'mutex' expected");
		return 0;
	}
	if (*mutexObj == nullptr) 
	{
		luaL_argerror(aState, 0, "Internal error: 'mutex' object is invalid");
		return 0;
	}
	(*mutexObj)->lock();
	auto numParams = lua_gettop(aState);
	luaL_checktype(aState, 2, LUA_TFUNCTION);
	lua_pcall(aState, numParams - 2, 0, 0);
	(*mutexObj)->unlock();

	return 0;
}





/** Locks the provided mutex. */
extern "C" static int mutexLock(LuaState * aState) 
{
	auto mutexObj = reinterpret_cast<std::mutex **>(luaL_checkudata(aState, 1, MUTEX_METATABLE_NAME));
	if (mutexObj == nullptr)
	{
		luaL_argerror(aState, 0, "'mutex' expected");
		return 0;
	}
	if (*mutexObj == nullptr)
	{
		luaL_argerror(aState, 0, "Internal error: 'mutex' object is invalid");
		return 0;
	}
	(*mutexObj)->lock();
	return 0;
}





/** Unlocks the provided mutex. */
extern "C" static int mutexUnlock(LuaState * aState) 
{
	auto mutexObj = reinterpret_cast<std::mutex **>(luaL_checkudata(aState, 1, MUTEX_METATABLE_NAME));
	if (mutexObj == nullptr)
	{
		luaL_argerror(aState, 0, "'mutex' expected");
		return 0;
	}
	if (*mutexObj == nullptr)
	{
		luaL_argerror(aState, 0, "Internal error: 'mutex' object is invalid");
		return 0;
	}
	(*mutexObj)->unlock();
	return 0;
}





/** Starts a new thread and runs the provided Lua function on it. 
Parameter: The Lua callback which will be called on the new thread.
Returns: The thread object with the functions in threadObjFuncs as methods in it's metatable. */
extern "C" static int threadNew(LuaState * aState)
{
	static std::recursive_mutex mtx;
	std::scoped_lock lock(mtx);
	auto numParams = lua_gettop(aState);
	luaL_checktype(aState, 1, LUA_TFUNCTION);
	auto luaThread = lua_newthread(aState);

	// Move all the provided parameters to the new thread.
	for (int i = 1; i < numParams + 1; i++) 
	{
		lua_pushvalue(aState, i);                             // Push a copy of the parameter to the top of the stack...
		auto luaFnRef = luaL_ref(aState, LUA_REGISTRYINDEX);  // ... move it to the registry...
		lua_rawgeti(luaThread, LUA_REGISTRYINDEX, luaFnRef);  // ... push it onto the new thread's stack...
		luaL_unref(aState, LUA_REGISTRYINDEX, luaFnRef);      // ... and remove it from the registry
	}
	lua_pushcfunction(aState, errorHandler);

	// Push the (currently empty) thread object to the Lua side
	auto threadObj = reinterpret_cast<std::thread **>(lua_newuserdata(aState, sizeof(std::thread **)));
	luaL_setmetatable(aState, THREAD_METATABLE_NAME);

	// Start the new thread:
	*threadObj = new std::thread(
		[luaThread, numParams]()
		{
			lua_pcall(luaThread, numParams - 1, LUA_MULTRET, 1);
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
	return pushThreadIdOnLuaStack(aState, (*threadObj)->get_id());
}





/** Implements the thread.currentid() function.
Returns the current thread's ID. This also works on the main thread. */
extern "C" static int threadCurrentID(LuaState * aState)
{
	return pushThreadIdOnLuaStack(aState, std::this_thread::get_id());
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





/** The functions in the mutex library. */
static const luaL_Reg mutexFuncs[] =
{
	{"new", &mutexNew},
	{NULL,NULL}
};





/** The functions of the mutex object.  */
static const luaL_Reg mutexObjFuncs[] =
{
	{"dowhilelocked", &mutexDoWhileLocked},
	{"lock", &mutexLock},
	{"unlock", &mutexUnlock},
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





/** Registers the mutex library into the Lua VM. */
extern "C" static int luaopen_mutex(LuaState * aState)
{
	luaL_newlib(aState, mutexFuncs);

	// Register the metatable for std::thread objects:
	luaL_newmetatable(aState, MUTEX_METATABLE_NAME);
	lua_pushvalue(aState, -1);
	lua_setfield(aState, -2, "__index");  // metatable.__index = metatable
	luaL_setfuncs(aState, mutexObjFuncs, 0);  // Add the object functions to the table
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
	luaL_requiref(L, "mutex", &luaopen_mutex, true);
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

