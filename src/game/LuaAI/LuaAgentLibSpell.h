#ifndef MANGOS_LuaAgentLibSpell_H
#define MANGOS_LuaAgentLibSpell_H

#include "lua.hpp"

class Spell;
class SpellEntry;

struct LuaAI_Spell
{
	Spell* spell;
	const SpellEntry* proto;
};

namespace LuaBindsAI {
	static const char* SpellMtName = "LuaAI.Spell";

	void BindSpell(lua_State* L);
	void Spell_CreateMetatable(lua_State* L);
	LuaAI_Spell* Spell_GetSpellObject(lua_State* L, int idx = 1);
	void Spell_CreateUD(lua_State* L, uint32 id);

	int Spell_GetEffectAuraType(lua_State* L);
	int Spell_GetEffectSimpleValue(lua_State* L);

	static const struct luaL_Reg Spell_BindLib[]{
		{"GetEffectType", Spell_GetEffectAuraType},
		{"GetEffectSimpleValue", Spell_GetEffectSimpleValue},

		{NULL, NULL}
	};


}

#endif
