#ifndef MANGOS_LuaAgentLibAux_H
#define MANGOS_LuaAgentLibAux_H

struct lua_State;

namespace LuaBindsAI {
	int GetUnitByGuid(lua_State* L);
	int GetPlayerByGuid(lua_State* L);
}

#endif
