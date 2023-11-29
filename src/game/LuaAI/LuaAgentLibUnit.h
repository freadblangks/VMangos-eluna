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
	Unit* Unit_GetUnitObject(lua_State* L, int idx = 1);
	void Unit_CreateUD(Unit* unit, lua_State* L);

	// Auto attacking

	int Unit_Attack(lua_State* L);
	int Unit_AttackStop(lua_State* L);
	int Unit_CastSpell(lua_State* L);

	// Info

	int Unit_GetDistance(lua_State* L);
	int Unit_GetHealth(lua_State* L);
	int Unit_GetHealthPct(lua_State* L);
	int Unit_GetMaxHealth(lua_State* L);
	int Unit_GetPower(lua_State* L);
	int Unit_GetPowerPct(lua_State* L);
	int Unit_GetMaxPower(lua_State* L);

	int Unit_SetHealth(lua_State* L);
	int Unit_SetMaxHealth(lua_State* L);
	int Unit_SetHealthPct(lua_State* L);
	int Unit_SetPower(lua_State* L);
	int Unit_SetMaxPower(lua_State* L);
	int Unit_SetPowerPct(lua_State* L);

	int Unit_CanReachWithMeleeAutoAttack(lua_State* L);
	int Unit_CanHaveThreatList(lua_State* L);
	int Unit_GetHighestThreat(lua_State* L);
	int Unit_GetShapeshiftForm(lua_State* L);
	int Unit_GetThreat(lua_State* L);
	int Unit_GetThreatTbl(lua_State* L);
	int Unit_GetVictim(lua_State* L);

	int Unit_HasAura(lua_State* L);

	// General info

	int Unit_GetClass(lua_State* L);
	int Unit_GetGuid(lua_State* L);
	int Unit_GetLevel(lua_State* L);
	int Unit_GetName(lua_State* L);
	int Unit_GetRole(lua_State* L);

	// Motion control

	int Unit_ClearMotion(lua_State* L);
	int Unit_GetMotionType(lua_State* L);
	int Unit_MoveFollow(lua_State* L);
	int Unit_MoveChase(lua_State* L);

	static const struct luaL_Reg Unit_BindLib[]{
		// Auto attacking
		{"Attack", Unit_Attack},
		{"AttackStop", Unit_AttackStop},
		{"CastSpell", Unit_CastSpell},

		// Info
		{"GetDistance", Unit_GetDistance},
		{"GetHealth", Unit_GetHealth},
		{"GetHealthPct", Unit_GetHealthPct},
		{"GetMaxHealth", Unit_GetMaxHealth},
		{"GetPower", Unit_GetPower},
		{"GetPowerPct", Unit_GetPowerPct},
		{"GetMaxPower", Unit_GetMaxPower},

		{"SetHealth", Unit_SetHealth},
		{"SetHealthPct", Unit_SetHealthPct},
		{"SetMaxHealth", Unit_SetMaxHealth},
		{"SetPower", Unit_SetPower},
		{"SetPowerPct", Unit_SetPowerPct},
		{"SetMaxPower", Unit_SetMaxPower},

		{"CanReachWithMelee", Unit_CanReachWithMeleeAutoAttack},
		{"CanHaveThreatList", Unit_CanHaveThreatList},
		{"GetThreat", Unit_GetThreat},
		{"GetThreatTbl", Unit_GetThreatTbl},
		{"GetHighestThreat", Unit_GetHighestThreat},
		{"GetShapeshiftForm", Unit_GetShapeshiftForm},
		{"GetVictim", Unit_GetVictim},

		{"HasAura", Unit_HasAura},

		// General info
		{"GetClass", Unit_GetClass},
		{"GetGuid", Unit_GetGuid},
		{"GetLevel", Unit_GetLevel},
		{"GetName", Unit_GetName},
		{"GetRole", Unit_GetRole},

		// motion
		{"ClearMotion", Unit_ClearMotion},
		{"GetMotionType", Unit_GetMotionType},
		{"MoveFollow", Unit_MoveFollow},
		{"MoveChase", Unit_MoveChase},

		{NULL, NULL}
	};

}


#endif
