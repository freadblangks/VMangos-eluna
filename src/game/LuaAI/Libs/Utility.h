
#ifndef MANGOS_LuaAILibsUtility_H
#define MANGOS_LuaAILibsUtility_H

class LuaAI_Item;
struct lua_State;
class Player;

namespace Utility
{
	constexpr float MAX_ITEM_DPS = 150.f;
	constexpr float MAX_ITEM_ARMOR = 3500.f;
	constexpr float MAX_ITEM_STAT = 100.f;
	constexpr float MAX_ITEM_AURA_VAL = 100.f;

	float Compensate(float score, int nConsiderations);
	void GetItemUtility(lua_State* L, LuaAI_Item* item, Player* agent);
}

#endif
