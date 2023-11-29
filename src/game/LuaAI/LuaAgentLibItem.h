#ifndef MANGOS_LuaAgentLibItem_H
#define MANGOS_LuaAgentLibItem_H

#include "lua.hpp"

class Item;
class ItemPrototype;

struct LuaAI_Item
{
	Item* item;
	const ItemPrototype* proto;
	LuaAI_Item() : item(nullptr), proto(nullptr) {}
	float GetDps() { return proto->Damage[0].DamageMax / proto->Delay / 1000.f; }
};

namespace LuaBindsAI {
	static const char* ItemMtName = "LuaAI.Item";

	void BindItem(lua_State* L);
	void Item_CreateMetatable(lua_State* L);
	LuaAI_Item* Item_GetItemObject(lua_State* L, int idx = 1);
	void Item_CreateUD(lua_State* L, uint32 id);

	int Item_GetItemFromId(lua_State* L);

	int Item_GetArmor(lua_State* L);
	int Item_GetContextualLevel(lua_State* L);
	int Item_GetDamage(lua_State* L);
	int Item_GetId(lua_State* L);
	int Item_GetName(lua_State* L);
	int Item_GetSlots(lua_State* L);
	int Item_GetSpell(lua_State* L);
	int Item_GetStat(lua_State* L);
	int Item_IsUnique(lua_State* L);

	int Item_GetUtility(lua_State* L);

	// debug

	int Item_PrintRandomEnchants(lua_State* L);
	int Item_Print(lua_State* L);
	int Item_PrintItemsOfType(lua_State* L);

	static const struct luaL_Reg Item_BindLib[]{
		{"GetArmor", Item_GetArmor},
		{"GetContextualLevel", Item_GetContextualLevel},
		{"GetDamage", Item_GetDamage},
		{"GetId", Item_GetId},
		{"GetName", Item_GetName},
		{"GetSlots", Item_GetSlots},
		{"GetSpell", Item_GetSpell},
		{"GetStat", Item_GetStat},
		{"IsUnique", Item_IsUnique},

		{"GetUtility", Item_GetUtility},

		{"PrintRandomEnchants", Item_PrintRandomEnchants},
		{"Print", Item_Print},

		{NULL, NULL}
	};


}

#endif
