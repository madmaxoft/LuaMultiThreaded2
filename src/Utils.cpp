#include "Utils.h"
extern "C"
{
	#include <lua.h>
}





std::string lua_getstring(lua_State * L, int aIndex)
{
	size_t len;
	auto s = lua_tolstring(L, aIndex, &len);
	return std::string(s, len);
}





std::string lua_getstringfield(lua_State * L, int aTableIndex, const char * aFieldName)
{
	lua_getfield(L, aTableIndex, aFieldName);
	std::string res = lua_getstring(L, -1);
	lua_pop(L, 1);
	return res;
}





int pushThreadIdOnLuaStack(lua_State* aState, std::thread::id aThreadId)
{
	std::stringstream ss;
	ss << aThreadId;
	auto str = ss.str();
	lua_pushlstring(aState, str.data(), str.size());
	return 1;
}





