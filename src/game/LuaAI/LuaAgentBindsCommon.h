#pragma once

#include "lua.hpp"

struct lua_State;

namespace LuaBindsAI {
	void BindAll( lua_State* L );
	void BindGlobalFunc( lua_State* L, const char* key, lua_CFunction func );
}