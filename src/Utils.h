#pragma once

#include <string>
#include <thread>
#include <sstream>





// fwd:
struct lua_State;




#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))





/** Returns the value at the specified stack index as a std::string. */
std::string lua_getstring(lua_State * L, int aIndex);

/** Returns the value of the named field of a table at the specified index, as a std::string. */
std::string lua_getstringfield(lua_State * L, int aTableIndex, const char * aFieldName);

/** Pushes the value of a std::thread::id into a lua stack as a string */
int pushThreadIdOnLuaStack(lua_State* aState, std::thread::id aThreadId);