
#include "LuaAgentBindsCommon.h"
#include "Goal/Goal.h"
#include "Goal/GoalManager.h"
#include "Goal/LogicManager.h"
#include "LuaAgentLibAI.h"
#include "LuaAgentLibPlayer.h"
#include "LuaAgentLibUnit.h"
#include "LuaAgentLibAux.h"
#include "LuaAgentLibItem.h"
#include "LuaAgentLibSpell.h"
#include "LuaAgentLibWorldObj.h"
#include "Hierarchy/LuaAgentPartyInt.h"
#include "lua.hpp"


void LuaBindsAI::BindAll(lua_State* L) {
	BindGoalManager(L);
	BindGoal(L);
	BindLogicManager(L);
	BindAI(L);
	BindPlayer(L);
	BindUnit(L);
	BindWorldObject(L);
	BindPartyIntelligence(L);
	lua_register(L, "GetUnitByGuid", GetUnitByGuid);
	lua_register(L, "GetPlayerByGuid", GetPlayerByGuid);
	lua_register(L, "GetSpellName", GetSpellName);
	BindItem(L);
	BindSpell(L);
	lua_pushinteger(L, SUPPORTED_CLIENT_BUILD);
	lua_setglobal(L, "CVER");
}


void LuaBindsAI::BindGlobalFunc(lua_State* L, const char* key, lua_CFunction func) {
	lua_pushcfunction(L, func);
	lua_setglobal(L, key);
}
