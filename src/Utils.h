#pragma once

#include <string>





// fwd:
struct lua_State;




#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))





/** Returns the value at the specified stack index as a std::string. */
std::string lua_getstring(lua_State * L, int aIndex);

/** Returns the value of the named field of a table at the specified index, as a std::string. */
std::string lua_getstringfield(lua_State * L, int aTableIndex, const char * aFieldName);
