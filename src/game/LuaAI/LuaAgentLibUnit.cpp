
#include "LuaAgent.h"
#include "LuaAgentUtils.h"
#include "LuaAgentLibUnit.h"
#include "LuaAgentLibWorldObj.h"
#include "Spell.h"
#include "SpellAuras.h"


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
	{
		if ((spell->InterruptFlags & SpellInterruptFlags::SPELL_INTERRUPT_FLAG_MOVEMENT) && !unit->IsStopped())
			unit->StopMoving();
		SpellCastResult result = unit->CastSpell(target, spell, triggered);
		if (result == SpellCastResult::SPELL_FAILED_NO_AMMO)
			if (Player* player = unit->ToPlayer())
				if (LuaAgent* ai = player->GetLuaAI())
					if (uint32 ammo = ai->GetAmmo())
						if (!player->HasItemCount(ammo))
						{
							player->StoreNewItemInBestSlots(ammo, 200u);
							player->SetAmmo(ammo);
							result = unit->CastSpell(target, spell, triggered);
						}
		lua_pushinteger(L, result);
	}
	else
		luaL_error(L, "Unit_CastSpell: spell %d doesn't exist", spellId);
	return 1;
}


int LuaBindsAI::Unit_GetCurrentSpellId(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	if (Spell* spell = unit->GetCurrentSpell(CurrentSpellTypes::CURRENT_GENERIC_SPELL))
		lua_pushinteger(L, spell->m_spellInfo->Id);
	else
		lua_pushinteger(L, 0ll);
	return 1;
}


int LuaBindsAI::Unit_GetPowerCost(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer spellId = luaL_checkinteger(L, 2);
	if (const SpellEntry* spell = sSpellMgr.GetSpellEntry(spellId))
	{
		Spell s(unit, spell, false);
		lua_pushnumber(L, Spell::CalculatePowerCost(spell, unit, &s));
	}
	else
		luaL_error(L, "Unit_GetPowerCost: spell %d doesn't exist", spellId);
	return 1;
}


int LuaBindsAI::Unit_GetSpellDamageAndThreat(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	Unit* unitTarget = Unit_GetUnitObject(L, 2);
	lua_Integer spellId = luaL_checkinteger(L, 3);
	bool isHealSpell = luaL_checkboolean(L, 4);
	bool forceCalc = luaL_checkboolean(L, 5);
	int effIdx = SpellEffectIndex::EFFECT_INDEX_0;
	if (lua_gettop(L) == 6)
	{
		effIdx = luaL_checkinteger(L, 6);
		if (effIdx < 0 || effIdx > SpellEffectIndex::EFFECT_INDEX_2)
			luaL_error(L, "Unit_GetSpellDamageAndThreat: invalid effect index. Allowed [0, %d], got %d", SpellEffectIndex::EFFECT_INDEX_2, effIdx);
	}
	if (const SpellEntry* spellProto = sSpellMgr.GetSpellEntry(spellId))
	{
		Spell s(unit, spellProto, true);
		float damage = s.CalculateDamage(SpellEffectIndex(effIdx), unitTarget);
		float threat = 0.f;
		if (forceCalc || isHealSpell || unitTarget->CanHaveThreatList())
		{
			if (isHealSpell)
			{
				damage = unit->SpellHealingBonusDone(unitTarget, spellProto, SpellEffectIndex(effIdx), damage, DamageEffectType::HEAL, 1, &s);
				damage = unitTarget->SpellHealingBonusTaken(unit, spellProto, SpellEffectIndex(effIdx), damage, DamageEffectType::HEAL, 1, &s);
				threat = damage * (unit->GetClass() == Classes::CLASS_PALADIN ? 0.25f : 0.5f);
				uint32 size = unitTarget->GetHostileRefManager().getSize();
				threat /= size ? size : 1;
			}
			else if (spellProto->IsDirectDamageSpell())
				threat = damage;

			SpellThreatEntry const* entry = sSpellMgr.GetSpellThreatEntry(spellProto->Id);
			if (entry)
			{
				threat *= entry->multiplier;
				threat += entry->threat;
			}

			threat = unit->ApplyTotalThreatModifier(threat, s.m_spellSchoolMask);
		}
		lua_pushnumber(L, damage);
		lua_pushnumber(L, threat);
	}
	else
		luaL_error(L, "Unit_GetSpellDamageAndThreat: spell %d doesn't exist", spellId);
	return 2;
}


int LuaBindsAI::Unit_GetSpellCastLeft(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	if (Spell* spell = unit->GetCurrentSpell(CurrentSpellTypes::CURRENT_GENERIC_SPELL))
		lua_pushinteger(L, spell->GetCastedTime());
	else
		lua_pushinteger(L, 0ll);
	return 1;
}


int LuaBindsAI::Unit_InterruptSpell(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer tp = luaL_checkinteger(L, 2);
	if (tp < 0 || tp > CurrentSpellTypes::CURRENT_CHANNELED_SPELL)
		luaL_error(L, "Unit_InterruptSpell: invalid spell type. Allowed values [0, %d], got %d", CurrentSpellTypes::CURRENT_CHANNELED_SPELL, tp);
	unit->InterruptSpell(CurrentSpellTypes(tp), false);
	return 0;
}


int LuaBindsAI::Unit_IsCastingHeal(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	if (Spell* spell = unit->GetCurrentSpell(CurrentSpellTypes::CURRENT_GENERIC_SPELL))
		lua_pushboolean(L, spell->m_spellInfo->IsHealSpell());
	else
		lua_pushboolean(L, false);
	return 1;
}


int LuaBindsAI::Unit_IsInPositionToCast(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	Unit* target = Unit_GetUnitObject(L, 2);
	lua_Integer spellId = luaL_checkinteger(L, 3);
	lua_Number bufferDist = luaL_checknumber(L, 4);
	if (const SpellEntry* spell = sSpellMgr.GetSpellEntry(spellId))
	{
		// Spell::CheckTarget and Spell:CheckRange bits
		if (!(spell->AttributesEx2 & SpellAttributesEx2::SPELL_ATTR_EX2_IGNORE_LINE_OF_SIGHT) && !unit->IsWithinLOSInMap(target))
			lua_pushinteger(L, SpellCastResult::SPELL_FAILED_LINE_OF_SIGHT);
		else
		{
			SpellCastResult result = SpellCastResult::SPELL_CAST_OK;
			SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(spell->rangeIndex);
			float max_range = srange ? srange->maxRange : 0.f;
			float min_range = srange ? srange->minRange : 0.f;

			if (Player* modOwner = unit->ToPlayer())
				modOwner->ApplySpellMod(spell->Id, SpellModOp::SPELLMOD_RANGE, max_range, nullptr);

			if (target && target != unit)
			{
				float const dist = unit->GetCombatDistance(target);

				if (dist > max_range - bufferDist)
					result = SpellCastResult::SPELL_FAILED_OUT_OF_RANGE;
				else if (min_range && dist < min_range)
					result = SpellCastResult::SPELL_FAILED_TOO_CLOSE;
			}
			lua_pushinteger(L, result);
		}
	}
	else
		luaL_error(L, "Unit_IsInPositionToCast: spell %d doesn't exist", spellId);
	return 1;
}


int LuaBindsAI::Unit_IsInLOS(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	Unit* target = Unit_GetUnitObject(L, 2);
	lua_pushboolean(L, unit->IsWithinLOSInMap(target));
	return 1;
}


// ---------------------------------------------------------
// --                     Info
// ---------------------------------------------------------


int LuaBindsAI::Unit_IsAlive(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushboolean(L, unit->IsAlive());
	return 1;
}


int LuaBindsAI::Unit_IsInCombat(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushboolean(L, unit->IsInCombat());
	return 1;
}


int LuaBindsAI::Unit_IsNonMeleeSpellCasted(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushboolean(L, unit->IsNonMeleeSpellCasted(false, false, true));
	return 1;
}


int LuaBindsAI::Unit_IsTanking(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	if (Player* player = unit->ToPlayer())
		if (LuaAgent* agentAI = player->GetLuaAI())
		{
			lua_pushboolean(L, agentAI->CommandsGetFirstType() == AgentCmdType::Tank);
			return 1;
		}
	lua_pushboolean(L, false);
	return 1;
}


int LuaBindsAI::Unit_GetAttackersNum(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushinteger(L, unit->GetAttackers().size());
	return 1;
}


int LuaBindsAI::Unit_GetDistance(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	if (lua_gettop(L) == 2)
	{
		Unit* target = Unit_GetUnitObject(L, 2);
		lua_pushnumber(L, unit->GetDistance(target));
	}
	else
	{
		lua_Number x = luaL_checknumber(L, 2);
		lua_Number y = luaL_checknumber(L, 3);
		lua_Number z = luaL_checknumber(L, 4);
		lua_pushnumber(L, unit->GetDistance(x, y, z));
	}
	return 1;
}


int LuaBindsAI::Unit_GetOrientation(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushnumber(L, unit->GetOrientation());
	return 1;
}


int LuaBindsAI::Unit_GetPosition(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushnumber(L, unit->GetPositionX());
	lua_pushnumber(L, unit->GetPositionY());
	lua_pushnumber(L, unit->GetPositionZ());
	return 3;
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


int LuaBindsAI::Unit_GetPowerType(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushinteger(L, unit->GetPowerType());
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


int LuaBindsAI::Unit_HasAuraType(lua_State* L) {
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer auraId = luaL_checkinteger(L, 2);
	if (auraId < SPELL_AURA_NONE || auraId >= TOTAL_AURAS)
		luaL_error(L, "Unit_HasAuraType: invalid aura type id, expected value in range [0, %d], got %d", SPELL_AURA_NONE, TOTAL_AURAS - 1, auraId);
	lua_pushboolean(L, unit->HasAuraType((AuraType) auraId));
	return 1;
}


int LuaBindsAI::Unit_GetAuraStacks(lua_State* L) {
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer spellId = luaL_checkinteger(L, 2);
	if (spellId < 0 || !sSpellMgr.GetSpellEntry(spellId))
		luaL_error(L, "Unit_GetAuraStacks: spell %d doesn't exist", spellId);
	if (SpellAuraHolder* sah = unit->GetSpellAuraHolder(spellId))
		lua_pushinteger(L, sah->GetStackAmount());
	else
		lua_pushinteger(L, -1);
	return 1;
}


int LuaBindsAI::Unit_GetAuraTimeLeft(lua_State* L) {
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer spellId = luaL_checkinteger(L, 2);
	if (spellId < 0 || !sSpellMgr.GetSpellEntry(spellId))
		luaL_error(L, "Unit_GetAuraTimeLeft: spell %d doesn't exist", spellId);
	if (SpellAuraHolder* sah = unit->GetSpellAuraHolder(spellId))
		lua_pushinteger(L, sah->GetAuraDuration());
	else
		lua_pushinteger(L, -1);
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


int LuaBindsAI::Unit_GetRace(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushinteger(L, unit->GetRace());
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
	unit->StopMoving();
	return 0;
}


int LuaBindsAI::Unit_GetMotionType(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushnumber(L, unit->GetMotionMaster()->GetCurrentMovementGeneratorType());
	return 1;
}


int LuaBindsAI::Unit_IsMoving(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_pushboolean(L, unit->IsMoving());
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


int LuaBindsAI::Unit_MovePoint(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Number x = luaL_checknumber(L, 2);
	lua_Number y = luaL_checknumber(L, 3);
	lua_Number z = luaL_checknumber(L, 4);
	bool force = luaL_checkboolean(L, 5);
	uint32 options = MoveOptions::MOVE_PATHFINDING;
	if (force)
		options |= MoveOptions::MOVE_FORCE_DESTINATION;
	unit->GetMotionMaster()->MovePoint(2048, x, y, z, options);
	return 0;
}


int LuaBindsAI::Unit_StopMoving(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	unit->StopMoving();
	return 0;
}


int LuaBindsAI::Unit_SetStandState(lua_State* L)
{
	Unit* unit = Unit_GetUnitObject(L);
	lua_Integer state = luaL_checkinteger(L, 2);
	if (state < 0 || state > UnitStandStateType::UNIT_STAND_STATE_KNEEL)
		luaL_error(L, "Unit_SetStandState: invalid state. Allowed values [0, %d], got %d", UnitStandStateType::UNIT_STAND_STATE_KNEEL, state);
	unit->SetStandState(state);
	return 0;
}
