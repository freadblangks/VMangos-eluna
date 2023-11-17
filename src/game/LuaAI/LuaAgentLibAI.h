#ifndef MANGOS_LuaAgentLibAI_H
#define MANGOS_LuaAgentLibAI_H

#include "lua.hpp"

class LuaAgent;

namespace LuaBindsAI {

	// Creates metatable for the AI userdata with name specified by AI::AI_MTNAME
	void BindAI(lua_State* L);
	// Creates metatable for the AI userdata with name specified by AI::AI_MTNAME.
	// Registers all the functions listed in LuaBindsAI::AI_BindLib with that metatable.
	void AI_CreateMetatable(lua_State* L);
	LuaAgent* AI_GetAIObject(lua_State* L, int idx = 1);

	int AI_AddTopGoal(lua_State* L);

	int AI_GetSpec(lua_State* L);
	int AI_GetPlayer(lua_State* L);
	int AI_GetMaster(lua_State* L);

	static const struct luaL_Reg AI_BindLib[]{
		{"AddTopGoal", AI_AddTopGoal},

		{"GetSpec", AI_GetSpec},
		{"GetMaster", AI_GetMaster},
		{"GetPlayer", AI_GetPlayer},

		{NULL, NULL}
	};

}

#endif
