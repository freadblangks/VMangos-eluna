
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

namespace
{
	inline void lua_setglobalint(lua_State* L, const char* field, lua_Integer value)
	{
		lua_pushinteger(L, value);
		lua_setglobal(L, field);
	}
}

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
	lua_register(L, "GetPlayerById", GetPlayerById);
	lua_register(L, "GetSpellName", GetSpellName);
	lua_register(L, "isinteger", isinteger);
	BindItem(L);
	BindSpell(L);
	lua_setglobalint(L, "CVER", SUPPORTED_CLIENT_BUILD);
	lua_setglobalint(L, "CAST_NOT_SHAPESHIFT", SpellCastResult::SPELL_FAILED_NOT_SHAPESHIFT);
	lua_setglobalint(L, "CAST_ONLY_SHAPESHIFT", SpellCastResult::SPELL_FAILED_ONLY_SHAPESHIFT);
}


void LuaBindsAI::BindGlobalFunc(lua_State* L, const char* key, lua_CFunction func) {
	lua_pushcfunction(L, func);
	lua_setglobal(L, key);
}
