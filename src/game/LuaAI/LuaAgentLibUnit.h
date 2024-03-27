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

	int Unit_GetAI(lua_State* L);

	// Attacking

	int Unit_Attack(lua_State* L);
	int Unit_AttackStop(lua_State* L);
	int Unit_CalculateDamage(lua_State* L);
	int Unit_CanAttack(lua_State* L);
	int Unit_CastSpell(lua_State* L);
	int Unit_GetCurrentSpellId(lua_State* L);
	int Unit_GetPowerCost(lua_State* L);
	/**
	 * @brief Calculates spell effect value. Only accounts for spell mods for healing spells.
	 * Does not check if spell is a healing spell. If spell is a healing spell threat is divided among all hostiles.
	 * @param L 
	 * @return 
	*/
	int Unit_GetSpellDamageAndThreat(lua_State* L);
	int Unit_GetSpellCastLeft(lua_State* L);
	int Unit_HasEnoughPowerFor(lua_State* L);
	int Unit_InterruptSpell(lua_State* L);
	int Unit_IsCastingHeal(lua_State* L);
	int Unit_IsCastingInterruptableSpell(lua_State* L);
	int Unit_IsImmuneToSpell(lua_State* L);
	int Unit_IsInPositionToCast(lua_State* L);
	int Unit_IsInLOS(lua_State* L);
	int Unit_IsSpellReady(lua_State* L);
	int Unit_GetTotemEntry(lua_State* L);
	int Unit_RemoveSpellCooldown(lua_State* L);
	int Unit_UnsummonAllTotems(lua_State* L);

	// Info

	int Unit_IsAgent(lua_State* L);
	int Unit_IsAlive(lua_State* L);
	int Unit_IsInCombat(lua_State* L);
	int Unit_IsNonMeleeSpellCasted(lua_State* L);
	int Unit_IsNextSwingSpellCasted(lua_State* L);
	int Unit_IsTanking(lua_State* L);

	int Unit_GetCreatureChaseInfo(lua_State* L);
	int Unit_GetDistance(lua_State* L);
	int Unit_GetForwardVector(lua_State* L);
	int Unit_GetOrientation(lua_State* L);
	int Unit_GetPosition(lua_State* L);
	int Unit_GetMapId(lua_State* L);
	int Unit_GetZoneId(lua_State* L);
	int Unit_IsInDungeon(lua_State* L);
	int Unit_GetMapHeight(lua_State* L);
	int Unit_GetAllowedZ(lua_State* L);

	int Unit_GetHealth(lua_State* L);
	int Unit_GetHealthPct(lua_State* L);
	int Unit_GetMaxHealth(lua_State* L);
	int Unit_GetPower(lua_State* L);
	int Unit_GetPowerPct(lua_State* L);
	int Unit_GetMaxPower(lua_State* L);
	int Unit_GetPowerType(lua_State* L);

	int Unit_SetHealth(lua_State* L);
	int Unit_SetMaxHealth(lua_State* L);
	int Unit_SetHealthPct(lua_State* L);
	int Unit_SetPower(lua_State* L);
	int Unit_SetMaxPower(lua_State* L);
	int Unit_SetPowerPct(lua_State* L);

	int Unit_CanReachWithMeleeAutoAttack(lua_State* L);
	int Unit_CanHaveThreatList(lua_State* L);
	int Unit_GetAttackers(lua_State* L);
	int Unit_GetAttackersNum(lua_State* L);
	int Unit_GetHighestThreat(lua_State* L);
	int Unit_GetShapeshiftForm(lua_State* L);
	int Unit_GetThreat(lua_State* L);
	int Unit_GetThreatTbl(lua_State* L);
	int Unit_GetVictim(lua_State* L);
	int Unit_IsRanged(lua_State* L);

	int Unit_GetAuraStacks(lua_State* L);
	int Unit_GetAuraTimeLeft(lua_State* L);
	int Unit_GetDispelsTbl(lua_State* L);
	int Unit_HasAura(lua_State* L);
	int Unit_HasAuraType(lua_State* L);
	int Unit_HasAuraWithMechanics(lua_State* L);
	int Unit_RemoveAuraByCancel(lua_State* L);
	int Unit_RemoveSpellsCausingAura(lua_State* L);

	// General info

	int Unit_GetClass(lua_State* L);
	int Unit_GetGuid(lua_State* L);
	int Unit_GetLevel(lua_State* L);
	int Unit_GetName(lua_State* L);
	int Unit_GetRace(lua_State* L);
	int Unit_GetRole(lua_State* L);
	int Unit_GetTeam(lua_State* L);
	int Unit_IsPlayer(lua_State* L);

	// Motion control

	int Unit_ClearMotion(lua_State* L);
	int Unit_GetMotionType(lua_State* L);
	int Unit_IsMoving(lua_State* L);
	int Unit_MoveFollow(lua_State* L);
	int Unit_MoveChase(lua_State* L);
	int Unit_MovePoint(lua_State* L);
	int Unit_StopMoving(lua_State* L);
	int Unit_GetStandState(lua_State* L);
	int Unit_SetStandState(lua_State* L);
	
	static const struct luaL_Reg Unit_BindLib[]{
		{"GetAI", Unit_GetAI},
		// Attacking
		{"Attack", Unit_Attack},
		{"AttackStop", Unit_AttackStop},
		{"CalculateDamage", Unit_CalculateDamage},
		{"CanAttack", Unit_CanAttack},
		{"CastSpell", Unit_CastSpell},
		{"GetCurrentSpellId", Unit_GetCurrentSpellId},
		{"GetPowerCost", Unit_GetPowerCost},
		{"GetSpellDamageAndThreat", Unit_GetSpellDamageAndThreat},
		{"GetSpellCastLeft", Unit_GetSpellCastLeft},
		{"HasEnoughPowerFor", Unit_HasEnoughPowerFor},
		{"InterruptSpell", Unit_InterruptSpell},
		{"IsCastingInterruptableSpell", Unit_IsCastingInterruptableSpell},
		{"IsCastingHeal", Unit_IsCastingHeal},
		{"IsImmuneToSpell", Unit_IsImmuneToSpell},
		{"IsInPositionToCast", Unit_IsInPositionToCast},
		{"IsInLOS", Unit_IsInLOS},
		{"IsSpellReady", Unit_IsSpellReady},
		{"GetTotemEntry", Unit_GetTotemEntry},
		{"RemoveSpellCooldown", Unit_RemoveSpellCooldown},
		{"UnsummonAllTotems", Unit_UnsummonAllTotems},

		// Info
		{"IsAgent", Unit_IsAgent},
		{"IsAlive", Unit_IsAlive},
		{"IsInCombat", Unit_IsInCombat},
		{"IsNonMeleeSpellCasted", Unit_IsNonMeleeSpellCasted},
		{"IsNextSwingSpellCasted", Unit_IsNextSwingSpellCasted},
		{"IsTanking", Unit_IsTanking},

		{"GetCreatureChaseInfo", Unit_GetCreatureChaseInfo},
		{"GetDistance", Unit_GetDistance},
		{"GetForwardVector", Unit_GetForwardVector},
		{"GetOrientation", Unit_GetOrientation},
		{"GetPosition", Unit_GetPosition},
		{"GetMapId", Unit_GetMapId},
		{"GetZoneId", Unit_GetZoneId},
		{"IsInDungeon", Unit_IsInDungeon},
		{"GetMapHeight", Unit_GetMapHeight},
		{"GetAllowedZ", Unit_GetAllowedZ},

		{"GetHealth", Unit_GetHealth},
		{"GetHealthPct", Unit_GetHealthPct},
		{"GetMaxHealth", Unit_GetMaxHealth},
		{"GetPower", Unit_GetPower},
		{"GetPowerPct", Unit_GetPowerPct},
		{"GetMaxPower", Unit_GetMaxPower},
		{"GetPowerType", Unit_GetPowerType},

		{"SetHealth", Unit_SetHealth},
		{"SetHealthPct", Unit_SetHealthPct},
		{"SetMaxHealth", Unit_SetMaxHealth},
		{"SetPower", Unit_SetPower},
		{"SetPowerPct", Unit_SetPowerPct},
		{"SetMaxPower", Unit_SetMaxPower},

		{"CanReachWithMelee", Unit_CanReachWithMeleeAutoAttack},
		{"CanHaveThreatList", Unit_CanHaveThreatList},
		{"GetAttackers", Unit_GetAttackers},
		{"GetAttackersNum", Unit_GetAttackersNum},
		{"GetThreat", Unit_GetThreat},
		{"GetThreatTbl", Unit_GetThreatTbl},
		{"GetHighestThreat", Unit_GetHighestThreat},
		{"GetShapeshiftForm", Unit_GetShapeshiftForm},
		{"GetVictim", Unit_GetVictim},
		{"IsRanged", Unit_IsRanged},

		{"CancelAura", Unit_RemoveAuraByCancel},
		{"GetAuraStacks", Unit_GetAuraStacks},
		{"GetAuraTimeLeft", Unit_GetAuraTimeLeft},
		{"GetDispelsTbl", Unit_GetDispelsTbl},
		{"HasAura", Unit_HasAura},
		{"HasAuraType", Unit_HasAuraType},
		{"HasAuraWithMechanics", Unit_HasAuraWithMechanics},
		{"RemoveSpellsCausingAura", Unit_RemoveSpellsCausingAura},

		// General info
		{"GetClass", Unit_GetClass},
		{"GetGuid", Unit_GetGuid},
		{"GetLevel", Unit_GetLevel},
		{"GetName", Unit_GetName},
		{"GetRace", Unit_GetRace},
		{"GetRole", Unit_GetRole},
		{"GetTeam", Unit_GetTeam},
		{"IsPlayer", Unit_IsPlayer},

		// motion
		{"ClearMotion", Unit_ClearMotion},
		{"GetMotionType", Unit_GetMotionType},
		{"IsMoving", Unit_IsMoving},
		{"MoveFollow", Unit_MoveFollow},
		{"MoveChase", Unit_MoveChase},
		{"MovePoint", Unit_MovePoint},
		{"StopMoving", Unit_StopMoving},
		{"GetStandState", Unit_GetStandState},
		{"SetStandState", Unit_SetStandState},

		{NULL, NULL}
	};

}


#endif
