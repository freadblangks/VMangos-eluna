#ifndef MANGOS_LuaAgentLibUnit_H
#define MANGOS_LuaAgentLibUnit_H

#include "lua.hpp"

namespace LuaBindsAI {

	static const char* UnitMtName = "LuaAI.Unit";

	// Creates metatable for the AI userdata with name specified by AI::Unit_MTNAME
	void BindUnit(lua_State* L);
	// Creates metatable for the AI userdata with name specified by AI::Unit_MTNAME.
	// Registers all the functions listed in LuaBindsBot::Unit_BindLib with that metatable.
	void Unit_CreateMetatable(lua_State* L);
	Unit** Unit_GetUnitObject(lua_State* L, int idx = 1);
	void Unit_CreateUD(Unit* unit, lua_State* L);

	// General info

	int Unit_GetClass(lua_State* L);
	int Unit_GetGuid(lua_State* L);
	int Unit_GetLevel(lua_State* L);
	int Unit_GetName(lua_State* L);

	// Motion control

	int Unit_ClearMotion(lua_State* L);
	int Unit_GetMotionType(lua_State* L);
	int Unit_MoveFollow(lua_State* L);

	static const struct luaL_Reg Unit_BindLib[]{
		{"GetClass", Unit_GetClass},
		{"GetGuid", Unit_GetGuid},
		{"GetLevel", Unit_GetLevel},
		{"GetName", Unit_GetName},

		{"ClearMotion", Unit_ClearMotion},
		{"GetMotionType", Unit_GetMotionType},
		{"MoveFollow", Unit_MoveFollow},

		{NULL, NULL}
	};

}


#endif