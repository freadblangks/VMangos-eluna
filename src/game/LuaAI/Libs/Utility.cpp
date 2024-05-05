
#include "Utility.h"
#include "LuaAI/LuaAgentLibItem.h"
#include "LuaAI/LuaAgent.h"
#include "LuaAI/LuaAgentLibAI.h"
#include "lua.hpp"
#include "ItemEnchantmentMgr.h"


namespace
{
	int UnitModStat2Stats(ItemModType tp)
	{
		switch (tp)
		{
		case UnitMods::UNIT_MOD_STAT_AGILITY:
			return ItemModType::ITEM_MOD_AGILITY;
		case UnitMods::UNIT_MOD_STAT_INTELLECT:
			return ItemModType::ITEM_MOD_INTELLECT;
		case UnitMods::UNIT_MOD_STAT_SPIRIT:
			return ItemModType::ITEM_MOD_SPIRIT;
		case UnitMods::UNIT_MOD_STAT_STAMINA:
			return ItemModType::ITEM_MOD_STAMINA;
		case UnitMods::UNIT_MOD_STAT_STRENGTH:
			return ItemModType::ITEM_MOD_STRENGTH;
		}
		return -1;
	}


	float Linear(float x, float max, float minX = 0.f, float c = 0.f, float m = 1.f)
	{
		x = std::max(x, minX);
		return m * x / max + c;
	}


	struct ChosenRandomProperty
	{
		ChosenRandomProperty(float score = 0.f, uint32 id = 0u) : score(score), id(id) {}
		float score;
		uint32 id;
	};
	ChosenRandomProperty GetItemUtilityBestRandomProperty(lua_State* L, const ItemPrototype* proto)
	{
		luaL_checktype(L, -1, LUA_TTABLE);
		uint32 randomPropertyId = proto->RandomProperty;
		if (!randomPropertyId)
			return ChosenRandomProperty();

		std::vector<uint32> enchants;
		GetAllEnchantsForRandomProperty(randomPropertyId, enchants);

		float max = 0.f;
		uint32 chosenId = 0u;
		for (const auto& randomId : enchants)
		{
			const ItemRandomPropertiesEntry* entry = sItemRandomPropertiesStore.LookupEntry(randomId);
			if (!entry)
				continue;

			for (const auto& enchantId : entry->enchant_id)
			{
				const SpellItemEnchantmentEntry* enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
				if (!enchant)
					continue;
				
				float score = 0.f;
				for (auto& enchantSpellId : enchant->spellid)
				{
					const SpellEntry* spell = sSpellMgr.GetSpellEntry(enchantSpellId);
					if (!spell)
						continue;
					for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
						if (spell->Effect[j] == SPELL_EFFECT_APPLY_AURA && spell->EffectApplyAuraName[j] == SPELL_AURA_MOD_STAT)
						{
							int tp = UnitModStat2Stats(ItemModType(spell->EffectMiscValue[j]));
							if (tp == -1)
								continue;

							if (lua_geti(L, -1, tp))
								score += Linear(spell->CalculateSimpleValue(SpellEffectIndex(j)), Utility::MAX_ITEM_STAT) * luaL_checknumber(L, -1);
							lua_pop(L, 1);
						}
				}
				if (max <= score)
				{
					max = score;
					chosenId = randomId;
				}
			}
		}
		//printf("(RP max=%.2f id=%d), ", max, chosenId);
		return ChosenRandomProperty(max, chosenId);
	}
}


float Utility::Compensate(float score, int nConsiderations)
{
	float factor = 1 - 1.f / (1 + nConsiderations);
	factor *= (1 - score);
	return score + (factor * score);
}


void Utility::GetItemUtility(lua_State* L, LuaAI_Item* item, Player* agent)
{
	if (agent->CanUseItem(item->proto) != InventoryResult::EQUIP_ERR_OK)
	{
		//printf("item %d score=0\n", item->proto->ItemId);
	 	lua_pushinteger(L, 0);
		lua_pushnumber(L, 0.f);
		return;
	}
	// weighted sum
	//printf("top=%d, ", lua_gettop(L));
	//printf("top=%d, ---id=%d name=%s---, ", lua_gettop(L), item->proto->ItemId, item->proto->Name1);
	luaL_checktype(L, -1, LUA_TTABLE);

	float score = 0.f;
	const ItemPrototype* proto = item->proto;
	
	if (proto->Class == ItemClass::ITEM_CLASS_WEAPON)
	{
		lua_getfield(L, -1, "DamageWeight");
		score = Linear(item->GetDps(), MAX_ITEM_DPS) * luaL_checknumber(L, -1);
		//printf("[%.2f, ", score);
	}
	else
	{
		lua_getfield(L, -1, "ArmorWeight");
		score = Linear(proto->Armor, MAX_ITEM_ARMOR) * luaL_checknumber(L, -1);
		//printf("[%.2f, ", score);
	}

	lua_getfield(L, -2, "StatWeights");
	luaL_checktype(L, -1, LUA_TTABLE);
	ChosenRandomProperty rp(GetItemUtilityBestRandomProperty(L, proto));

	for (auto& stat : proto->ItemStat)
		if (stat.ItemStatValue)
		{
			// do not consider items with negative stats
			if (stat.ItemStatValue < 0)
			{
				//printf("\n");
				lua_pushinteger(L, 0);
				lua_pushnumber(L, 0.f);
				return;
			}
			if (lua_geti(L, -1, stat.ItemStatType) != LUA_TNIL)
			{
				score += Linear(stat.ItemStatValue, MAX_ITEM_STAT) * luaL_checknumber(L, -1);
				//printf("%.2f, ", Linear(stat.ItemStatValue, MAX_ITEM_STAT) * luaL_checknumber(L, -1));
			}
			lua_pop(L, 1);
		}

	int schoolMask = SpellSchoolMask::SPELL_SCHOOL_MASK_ALL;
	if (lua_getfield(L, -3, "SpellSchools") != LUA_TNIL)
		schoolMask = luaL_checkinteger(L, -1);

	lua_getfield(L, -4, "AuraWeights");
	luaL_checktype(L, -1, LUA_TTABLE);

	//printf("], [");
	for (auto& itemSpell : proto->Spells)
		if (itemSpell.SpellId)
			if (const SpellEntry* spellEntry = sSpellMgr.GetSpellEntry(itemSpell.SpellId))
				for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
				{
					if (lua_geti(L, -1, spellEntry->EffectApplyAuraName[i]) != LUA_TNIL && spellEntry->HasAttribute(SpellAttributes::SPELL_ATTR_PASSIVE))
						if (spellEntry->EffectApplyAuraName[i] != AuraType::SPELL_AURA_MOD_DAMAGE_DONE || (schoolMask & spellEntry->EffectMiscValue[i]))
						{
							score += Linear(spellEntry->CalculateSimpleValue(SpellEffectIndex(i)), MAX_ITEM_AURA_VAL) * luaL_checknumber(L, -1);
							//printf("%.2f, ", Linear(spellEntry->CalculateSimpleValue(SpellEffectIndex(i)), MAX_ITEM_AURA_VAL) * luaL_checknumber(L, -1));
						}
					lua_pop(L, 1);
				}

	lua_pop(L, 4);

	if (rp.id != 0)
		score += rp.score;

	//printf("] score=%f %d %s\n", score, lua_gettop(L), proto->Name1);
	lua_pushinteger(L, rp.id);
	lua_pushnumber(L, score);
}

