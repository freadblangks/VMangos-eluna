
#include "LuaAgent.h"
#include "LuaAgentUtils.h"
#include "LuaAgentLibUnit.h"
#include "LuaAgentLibWorldObj.h"


void LuaBindsAI::BindUnit(lua_State* L) {
	Unit_CreateMetatable(L);
}


Unit* LuaBindsAI::Unit_GetUnitObject(lua_State* L, int idx) {
	return *((Unit**) luaL_checkudwithfield(L, idx, "isUnit"));//(Unit**) luaL_checkudata(L, idx, LuaBindsAI::UnitMtName);
}


void LuaBindsAI::Unit_CreateUD(Unit* unit, lua_State* L) {
	// create userdata on top of the stack pointing to a pointer of an AI object
	Unit** unitud = static_cast<Unit**>(lua_newuserdatauv(L, sizeof(Unit*), 0));
	*unitud = unit; // swap the AI object being pointed to to the current instance
	luaL_setmetatable(L, LuaBindsAI::UnitMtName);
}


int LuaBindsAI_Unit_CompareEquality(lua_State* L) {
	WorldObject* obj1 = *LuaBindsAI::WObj_GetWObjObject(L);
	WorldObject* obj2 = *LuaBindsAI::WObj_GetWObjObject(L, 2);
	lua_pushboolean(L, obj1 == obj2);
	return 1;
}


void LuaBindsAI::Unit_CreateMetatable(lua_State* L) {
	luaL_newmetatable(L, LuaBindsAI::UnitMtName);
	lua_pushvalue(L, -1); // copy mt cos setfield pops
	lua_setfield(L, -1, "__index"); // mt.__index = mt
	luaL_setfuncs(L, Unit_BindLib, 0); // copy funcs
	lua_pushcfunction(L, LuaBindsAI_Unit_CompareEquality);
	lua_setfield(L, -2, "__eq");
	lua_pushboolean(L, true);
	lua_setfield(L, -2, "isWorldObject");
	lua_pushboolean(L, true);
	lua_setfield(L, -2, "isUnit");
	lua_pop(L, 1); // pop mt
}


// ---------------------------------------------------------
// --                Attacking
// ---------------------------------------------------------


int LuaBindsAI::Unit_Attack(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	Unit* target = Unit_GetUnitObject(L, 2);
	lua_pushboolean(L, unit->Attack(target, true));
	return 1;
}


int LuaBindsAI::Unit_AttackStop(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushboolean(L, unit->AttackStop());
	return 1;
}


int LuaBindsAI::Unit_CastSpell(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	Unit* target = Unit_GetUnitObject(L, 2);
	lua_Integer spellId = luaL_checkinteger(L, 3);
	bool triggered = luaL_checkboolean(L, 4);
	if (const SpellEntry* spell = sSpellMgr.GetSpellEntry(spellId))
		lua_pushinteger(L, unit->CastSpell(target, spell, triggered));
	else
		luaL_error(L, "Unit_CastSpell: spell %d doesn't exist", spellId);
	return 1;
}


// ---------------------------------------------------------
// --                     Info
// ---------------------------------------------------------


int LuaBindsAI::Unit_GetDistance(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	Unit* target = Unit_GetUnitObject(L, 2);
	lua_pushnumber(L, unit->GetDistance(target));
	return 1;
}


int LuaBindsAI::Unit_GetHealth(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushinteger(L, unit->GetHealth());
	return 1;
}


int LuaBindsAI::Unit_GetHealthPct(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushnumber(L, unit->GetHealthPercent());
	return 1;
}


int LuaBindsAI::Unit_GetMaxHealth(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushinteger(L, unit->GetMaxHealth());
	return 1;
}


int LuaBindsAI::Unit_GetPower(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer powerType = luaL_checkinteger(L, 2);
	if (powerType < 0 || powerType > Powers::POWER_HAPPINESS)
		luaL_error(L, "Unit_GetPower: allowed power types [0, %d], got %d", Powers::POWER_HAPPINESS, powerType);
	lua_pushinteger(L, unit->GetPower(Powers(powerType)));
	return 1;
}


int LuaBindsAI::Unit_GetPowerPct(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer powerType = luaL_checkinteger(L, 2);
	if (powerType < 0 || powerType > Powers::POWER_HAPPINESS)
		luaL_error(L, "Unit_GetPowerPct: allowed power types [0, %d], got %d", Powers::POWER_HAPPINESS, powerType);
	lua_pushnumber(L, unit->GetPowerPercent(Powers(powerType)));
	return 1;
}


int LuaBindsAI::Unit_GetMaxPower(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer powerType = luaL_checkinteger(L, 2);
	if (powerType < 0 || powerType > Powers::POWER_HAPPINESS)
		luaL_error(L, "Unit_GetMaxPower: allowed power types [0, %d], got %d", Powers::POWER_HAPPINESS, powerType);
	lua_pushinteger(L, unit->GetMaxPower(Powers(powerType)));
	return 1;
}


int LuaBindsAI::Unit_SetHealth(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer value = luaL_checkinteger(L, 2);
	if (value < 0ll)
		luaL_error(L, "Unit_SetHealth: value must be >= 0");
	unit->SetHealth(value);
	return 0;
}


int LuaBindsAI::Unit_SetHealthPct(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Number value = luaL_checknumber(L, 2);
	if (value < .0)
		luaL_error(L, "Unit_SetHealthPct: value must be >= 0");
	unit->SetHealthPercent(value);
	return 0;
}


int LuaBindsAI::Unit_SetMaxHealth(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer value = luaL_checkinteger(L, 2);
	if (value < 1ll)
		luaL_error(L, "Unit_SetMaxHealth: value must be >= 1");
	unit->SetMaxHealth(value);
	return 0;
}


int LuaBindsAI::Unit_SetPower(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer powerType = luaL_checkinteger(L, 2);
	lua_Integer value = luaL_checkinteger(L, 3);
	if (powerType < 0ll || powerType > Powers::POWER_HAPPINESS)
		luaL_error(L, "Unit_SetPower: allowed power types [0, %d], got %d", Powers::POWER_HAPPINESS, powerType);
	if (value < 0ll)
		luaL_error(L, "Unit_SetPower: value must be >= 0");
	unit->SetPower(Powers(powerType), value);
	return 0;
}


int LuaBindsAI::Unit_SetPowerPct(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer powerType = luaL_checkinteger(L, 2);
	lua_Number value = luaL_checknumber(L, 3);
	if (powerType < 0ll || powerType > Powers::POWER_HAPPINESS)
		luaL_error(L, "Unit_SetPowerPct: allowed power types [0, %d], got %d", Powers::POWER_HAPPINESS, powerType);
	if (value < .0)
		luaL_error(L, "Unit_SetPowerPct: value must be >= 0");
	unit->SetPowerPercent(Powers(powerType), value);
	return 0;
}


int LuaBindsAI::Unit_SetMaxPower(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer powerType = luaL_checkinteger(L, 2);
	lua_Integer value = luaL_checkinteger(L, 3);
	if (powerType < 0ll || powerType > Powers::POWER_HAPPINESS)
		luaL_error(L, "Unit_SetMaxPower: allowed power types [0, %d], got %d", Powers::POWER_HAPPINESS, powerType);
	if (value < 1ll)
		luaL_error(L, "Unit_SetMaxPower: value must be >= 1");
	unit->SetMaxPower(Powers(powerType), value);
	return 0;
}


int LuaBindsAI::Unit_CanReachWithMeleeAutoAttack(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	Unit* target = Unit_GetUnitObject(L, 2);
	lua_pushboolean(L, unit->CanReachWithMeleeAutoAttack(target));
	return 1;
}


int LuaBindsAI::Unit_CanHaveThreatList(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushboolean(L, unit->CanHaveThreatList());
	return 1;
}


int LuaBindsAI::Unit_GetThreat(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	Unit* target = Unit_GetUnitObject(L, 2);
	lua_pushnumber(L, unit->GetThreatManager().getThreat(target));
	return 1;
}


int LuaBindsAI::Unit_GetThreatTbl(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_newtable(L);
	int tblIdx = 1;
	for (auto v : unit->GetThreatManager().getThreatList()) {
		if (Unit* hostile = v->getTarget()) {
			Unit_CreateUD(hostile, L);
			lua_seti(L, -2, tblIdx); // stack[1][tblIdx] = stack[-1], pops unit
			tblIdx++;
		}
	}
	return 1;
}


int LuaBindsAI::Unit_GetHighestThreat(lua_State* L)
{
	// todo: delete this function?
	Unit* unit = Unit_GetUnitObject(L);
	Unit* except = nullptr;

	if (lua_gettop(L) > 1)
		except = Unit_GetUnitObject(L, 2);

	float maxTank = 0.f;
	float maxNonTank = 0.f;
	for (auto& entry : unit->GetThreatManager().getThreatList())
	{
		Player* player = entry->getTarget()->ToPlayer();
		if (!player || (except && player->GetObjectGuid() == except->GetObjectGuid()))
			continue;

		LuaAgent* agentAI = player->GetLuaAI();
		bool isNotTank = !agentAI || agentAI->GetRole() != LuaAgentRoles::Tank;
		if (isNotTank)
		{
			if (maxNonTank == 0.f)
				maxNonTank = entry->getThreat();
		}
		else if (maxTank == 0.f)
			maxTank = entry->getThreat();

		if (maxTank != 0.f && maxNonTank != 0.f)
			break;
	}
	lua_pushnumber(L, maxNonTank);
	lua_pushnumber(L, maxTank);
	return 2;
}


int LuaBindsAI::Unit_GetVictim(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushunitornil(L, unit->GetVictim());
	return 1;
}


int LuaBindsAI::Unit_GetShapeshiftForm(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushinteger(L, unit->GetShapeshiftForm());
	return 1;
}


int LuaBindsAI::Unit_HasAura(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer spellId = luaL_checkinteger(L, 2);
	if (!sSpellMgr.GetSpellEntry(spellId))
		luaL_error(L, "Unit_HasAura: spell %d doesn't exist", spellId);
	lua_pushboolean(L, unit->HasAura(spellId));
	return 1;
}


// ---------------------------------------------------------
// --                General Info
// ---------------------------------------------------------


int LuaBindsAI::Unit_GetClass(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushinteger(L, unit->GetClass());
	return 1;
}


int LuaBindsAI::Unit_GetGuid(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	Guid_CreateUD(L, unit->GetObjectGuid());
	return 1;
}


int LuaBindsAI::Unit_GetLevel(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushinteger(L, unit->GetLevel());
	return 1;
}


int LuaBindsAI::Unit_GetName(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushstring(L, unit->GetName());
	return 1;
}


int LuaBindsAI::Unit_GetRole(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	if (Player* player = unit->ToPlayer())
		if (LuaAgent* agentAI = player->GetLuaAI())
		{
			lua_pushinteger(L, lua_Integer(agentAI->GetRole()));
			return 1;
		}
	lua_pushinteger(L, 0ll);
	return 1;
}


// ---------------------------------------------------------
// --                Movement Control
// ---------------------------------------------------------


int LuaBindsAI::Unit_ClearMotion(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	unit->GetMotionMaster()->Clear();
	return 0;
}


int LuaBindsAI::Unit_GetMotionType(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushnumber(L, unit->GetMotionMaster()->GetCurrentMovementGeneratorType());
	return 1;
}


int LuaBindsAI::Unit_MoveFollow(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	Unit* target = Unit_GetUnitObject(L, 2);
	double offset = luaL_checknumber(L, 3);
	double angle = luaL_checknumber(L, 4);
	unit->GetMotionMaster()->LuaAIMoveFollow(target, offset, angle);
	return 0;
}


int LuaBindsAI::Unit_MoveChase(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	Unit* target = Unit_GetUnitObject(L, 2);
	double offset = luaL_checknumber(L, 3);
	double offsetMin = luaL_checknumber(L, 4);
	double offsetMax = luaL_checknumber(L, 5);
	double angle = luaL_checknumber(L, 6);
	double angleT = luaL_checknumber(L, 7);
	bool noMinOffsetIfMutual = luaL_checkboolean(L, 8);
	unit->GetMotionMaster()->LuaAIMoveChase(target, offset, offsetMin, offsetMax, angle, angleT, noMinOffsetIfMutual);
	return 0;
}

