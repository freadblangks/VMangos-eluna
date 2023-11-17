#include "LuaAgentLibSpell.h"


void LuaBindsAI::Spell_CreateUD(lua_State* L, uint32 id)
{
	if (const SpellEntry* proto = sSpellMgr.GetSpellEntry(id))
	{
		LuaAI_Spell* spell = static_cast<LuaAI_Spell*>(lua_newuserdatauv(L, sizeof(LuaAI_Spell), 0));
		luaL_setmetatable(L, LuaBindsAI::SpellMtName);
		spell->proto = proto;
	}
}


void LuaBindsAI::BindSpell(lua_State* L)
{
	Spell_CreateMetatable(L);
}


LuaAI_Spell* LuaBindsAI::Spell_GetSpellObject(lua_State* L, int idx)
{
	return (LuaAI_Spell*) luaL_checkudata(L, idx, LuaBindsAI::SpellMtName);
}


int LuaBindsAI_Spell_CompareEquality(lua_State* L)
{
	LuaAI_Spell* spell1 = LuaBindsAI::Spell_GetSpellObject(L, 1);
	LuaAI_Spell* spell2 = LuaBindsAI::Spell_GetSpellObject(L, 2);
	lua_pushboolean(L, spell1->proto->Id == spell2->proto->Id);
	return 1;
}


void LuaBindsAI::Spell_CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, LuaBindsAI::SpellMtName);
	lua_pushvalue(L, -1); // copy mt cos setfield pops
	lua_setfield(L, -1, "__index"); // mt.__index = mt
	luaL_setfuncs(L, Spell_BindLib, 0); // copy funcs
	lua_pushcfunction(L, LuaBindsAI_Spell_CompareEquality);
	lua_setfield(L, -2, "__eq");
	lua_pop(L, 1); // pop mt
}


int LuaBindsAI::Spell_GetEffectAuraType(lua_State* L)
{
	LuaAI_Spell* spell = Spell_GetSpellObject(L, 1);
	int i = luaL_checkinteger(L, 2);
	if (i < 0 || i > MAX_EFFECT_INDEX)
		luaL_error(L, "Spell.GetEffectType index out of bounds. Got %d Max = %d", i, MAX_EFFECT_INDEX);
	lua_pushinteger(L, spell->proto->EffectApplyAuraName[i]);
	return 1;
}


int LuaBindsAI::Spell_GetEffectSimpleValue(lua_State* L)
{
	LuaAI_Spell* spell = Spell_GetSpellObject(L, 1);
	int i = luaL_checkinteger(L, 2);
	if (i < 0 || i > MAX_EFFECT_INDEX)
		luaL_error(L, "Spell.GetEffectSimpleValue index out of bounds. Got %d Max = %d", i, MAX_EFFECT_INDEX);
	lua_pushinteger(L, spell->proto->CalculateSimpleValue(SpellEffectIndex(i)));
	return 1;
}

