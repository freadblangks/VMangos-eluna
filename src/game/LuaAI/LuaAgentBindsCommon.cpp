#include "LuaAgentBindsCommon.h"
#include "Goal/GoalManager.h"
#include "Goal/LogicManager.h"
#include "LuaAgentLibAI.h"
#include "LuaAgentLibPlayer.h"
#include "LuaAgentLibUnit.h"
#include "LuaAgentLibAux.h"
#include "lua.hpp"

void LuaBindsAI::BindAll(lua_State* L) {
	BindGoalManager(L);
	BindGoal(L);
	BindLogicManager(L);
	BindAI(L);
	BindPlayer(L);
	BindUnit(L);
	lua_register(L, "GetUnitByGuid", GetUnitByGuid);
	lua_register(L, "GetPlayerByGuid", GetPlayerByGuid);
}

void LuaBindsAI::BindGlobalFunc(lua_State* L, const char* key, lua_CFunction func) {
	lua_pushcfunction(L, func);
	lua_setglobal(L, key);
}