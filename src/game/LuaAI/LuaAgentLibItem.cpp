#include "LuaAgentLibItem.h"
#include "LuaAgentLibSpell.h"
#include "Libs/Utility.h"
#include "LuaAgent.h"
#include "LuaAgentLibAI.h"
#include "ItemEnchantmentMgr.h"
#include "LuaAgentUtils.h"


namespace
{
	void Item_PrintItem(const ItemPrototype* proto)
	{
		printf("%d, -- %d, %d, %d, ", proto->ItemId, proto->RequiredLevel, proto->ItemLevel, proto->SourceQuestLevel);
		if (proto->Class == ItemClass::ITEM_CLASS_WEAPON)
			printf("D=[%.2f, %.2f], ", proto->Damage->DamageMin, proto->Damage->DamageMax);
		else if (proto->Class == ItemClass::ITEM_CLASS_ARMOR)
			printf("A=%d, ", proto->Armor);
		printf("%s, (", proto->Name1);
		for (int i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
		{

			auto stat = proto->ItemStat[i];
			std::string stat_name = "Unk";
			switch (stat.ItemStatType)
			{
			case ItemModType::ITEM_MOD_MANA:
				stat_name = "Mana";
				break;
			case ItemModType::ITEM_MOD_HEALTH:
				stat_name = "Hp";
				break;
			case ItemModType::ITEM_MOD_AGILITY:
				stat_name = "Agil";
				break;
			case ItemModType::ITEM_MOD_STRENGTH:
				stat_name = "Stre";
				break;
			case ItemModType::ITEM_MOD_INTELLECT:
				stat_name = "Inte";
				break;
			case ItemModType::ITEM_MOD_SPIRIT:
				stat_name = "Spir";
				break;
			case ItemModType::ITEM_MOD_STAMINA:
				stat_name = "Stam";
				break;
			}
			if (stat.ItemStatValue != 0)
				printf("%s=%d; ", stat_name.c_str(), stat.ItemStatValue);

		}
		printf("), ");
		if (proto->RandomProperty)
			printf("RandomProperty, ");
		for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
			if (proto->Spells[i].SpellId)
				if (const SpellEntry* spellInfo = sSpellMgr.GetSpellEntry(proto->Spells[i].SpellId))
					printf("%s, ", spellInfo->SpellName[0].c_str());
		if (proto->ArcaneRes != 0)
			printf("ArcaneRes=%d, ", proto->ArcaneRes);
		if (proto->FireRes != 0)
			printf("FireRes=%d, ", proto->FireRes);
		if (proto->FrostRes != 0)
			printf("FrostRes=%d, ", proto->FrostRes);
		if (proto->HolyRes != 0)
			printf("HolyRes=%d, ", proto->HolyRes);
		if (proto->NatureRes != 0)
			printf("NatureRes=%d, ", proto->NatureRes);
		if (proto->ShadowRes != 0)
			printf("ShadowRes=%d, ", proto->ShadowRes);
		printf("\n");
	}
}


void LuaBindsAI::Item_CreateUD(lua_State* L, uint32 id)
{
	if (const ItemPrototype* proto = sObjectMgr.GetItemPrototype(id))
	{
		LuaAI_Item* item = static_cast<LuaAI_Item*>(lua_newuserdatauv(L, sizeof(LuaAI_Item), 0));
		luaL_setmetatable(L, LuaBindsAI::ItemMtName);
		item->proto = proto;
	}
}


void LuaBindsAI::BindItem(lua_State* L)
{
	Item_CreateMetatable(L);
	lua_register(L, "Items_PrintItemsOfType", Item_PrintItemsOfType);
	lua_register(L, "Item_GetItemFromId", Item_GetItemFromId);
}


LuaAI_Item* LuaBindsAI::Item_GetItemObject(lua_State* L, int idx)
{
	return (LuaAI_Item*) luaL_checkudata(L, idx, LuaBindsAI::ItemMtName);
}


int LuaBindsAI_Item_CompareEquality(lua_State* L)
{
	LuaAI_Item* item1 = LuaBindsAI::Item_GetItemObject(L, 1);
	LuaAI_Item* item2 = LuaBindsAI::Item_GetItemObject(L, 2);
	lua_pushboolean(L, item1->proto->ItemId == item2->proto->ItemId);
	return 1;
}


void LuaBindsAI::Item_CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, LuaBindsAI::ItemMtName);
	lua_pushvalue(L, -1); // copy mt cos setfield pops
	lua_setfield(L, -1, "__index"); // mt.__index = mt
	luaL_setfuncs(L, Item_BindLib, 0); // copy funcs
	lua_pushcfunction(L, LuaBindsAI_Item_CompareEquality);
	lua_setfield(L, -2, "__eq");
	lua_pop(L, 1); // pop mt
}


int LuaBindsAI::Item_GetItemFromId(lua_State* L)
{
	int itemId = luaL_checkinteger(L, 1);
	if (const ItemPrototype* proto = sObjectMgr.GetItemPrototype(itemId))
		Item_CreateUD(L, itemId);
	else
		lua_pushnil(L);
	return 1;
}


int LuaBindsAI::Item_CanEquipToSlot(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	lua_Integer slot = luaL_checkinteger(L, 2);
	lua_Integer cls = luaL_checkinteger(L, 3);
	if (slot < 0 || slot >= EQUIPMENT_SLOT_END)
		luaL_error(L, "Item_CanEquipToSlot: incorrect slot value, allowed [0, %d], got %d", EQUIPMENT_SLOT_END, slot);
	if (cls < 0 || cls > CLASS_DRUID)
		luaL_error(L, "Item_CanEquipToSlot: incorrect class value, allowed [0, %d], got %d", CLASS_DRUID, cls);
	const ItemPrototype* proto = item->proto;
	uint8 slots[4] = {};
	proto->GetAllowedEquipSlots(slots, cls, true);
	for (auto& possibleSlot : slots)
		if (slot == possibleSlot)
		{
			lua_pushboolean(L, true);
			return 1;
		}
	lua_pushboolean(L, false);
	return 1;
}


int LuaBindsAI::Item_GetUtility(lua_State* L)
{
	luaL_checktype(L, 3, LUA_TTABLE);
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	LuaAgent* agentAI = AI_GetAIObject(L, 2);
	Utility::GetItemUtility(L, item, agentAI->GetPlayer());
	return 2;
}


int LuaBindsAI::Item_GetName(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	lua_pushstring(L, item->proto->Name1);
	return 1;
}


int LuaBindsAI::Item_GetId(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	lua_pushinteger(L, item->proto->ItemId);
	return 1;
}


int LuaBindsAI::Item_GetArmor(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	lua_pushinteger(L, item->proto->Armor);
	return 1;
}


int LuaBindsAI::Item_GetDamage(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	lua_pushnumber(L, item->proto->Damage[0].DamageMin);
	lua_pushnumber(L, item->proto->Damage[0].DamageMax);
	lua_pushinteger(L, item->proto->Damage[0].DamageType);
	return 3;
}


int LuaBindsAI::Item_GetStat(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	int i = luaL_checkinteger(L, 2);
	if (i < 0 || i >= MAX_ITEM_PROTO_STATS)
		luaL_error(L, "Item.GetStat index out of bounds. Got %d Max = %d", i, MAX_ITEM_PROTO_STATS);
	lua_pushinteger(L, item->proto->ItemStat[i].ItemStatType);
	lua_pushinteger(L, item->proto->ItemStat[i].ItemStatValue);
	return 2;
}


int LuaBindsAI::Item_GetSpell(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	int i = luaL_checkinteger(L, 2);
	if (i < 0 || i >= MAX_ITEM_PROTO_SPELLS)
		luaL_error(L, "Item.GetSpell index out of bounds. Got %d Max = %d", i, MAX_ITEM_PROTO_SPELLS);
	uint32 spellId = item->proto->Spells[i].SpellId;
	if (spellId)
		Spell_CreateUD(L, item->proto->Spells[i].SpellId);
	else
		lua_pushnil(L);
	return 1;
}


int LuaBindsAI::Item_IsUnique(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	lua_pushboolean(L, item->proto->HasItemFlag(ItemPrototypeFlags::ITEM_FLAG_UNIQUE_EQUIPPED) || item->proto->MaxCount == 1);
	return 1;
}


int LuaBindsAI::Item_GetContextualLevel(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	uint32 level = item->proto->RequiredLevel;
	if (!level && item->proto->SourceQuestLevel > 0)
		level = item->proto->SourceQuestLevel;
	if (!level)
		level = item->proto->ItemLevel;
	lua_pushinteger(L, level);
	return 1;
}


int LuaBindsAI::Item_GetSlots(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	uint8 slots[4];
	item->proto->GetAllowedEquipSlots(slots, luaL_checkinteger(L, 2), luaL_checkboolean(L, 3));
	for (auto& slot : slots)
		lua_pushinteger(L, slot);
	return 4;
}


int LuaBindsAI::Item_GetSubclass(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	lua_pushinteger(L, item->proto->SubClass);
	return 1;
}


int LuaBindsAI::Item_Print(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	Item_PrintItem(item->proto);
	return 0;
}


int LuaBindsAI::Item_PrintItemsOfType(lua_State* L)
{
	lua_Integer cls = luaL_checkinteger(L, 1);
	lua_Integer subclass = luaL_checkinteger(L, 2);
	lua_Integer invtype = luaL_checkinteger(L, 3);
	std::vector<ItemPrototype const*> result;
	for (auto const& itr : sObjectMgr.GetItemPrototypeMap())
	{
		ItemPrototype const* pProto = &itr.second;

		if (pProto->Class != cls)
			continue;

		if (subclass != -1 && pProto->SubClass != subclass)
			continue;

		if (invtype != -1 && pProto->InventoryType != invtype)
			if (subclass != ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_CLOTH || invtype != InventoryType::INVTYPE_CHEST || pProto->InventoryType != InventoryType::INVTYPE_ROBE)
				continue;

		result.push_back(pProto);
	}
	struct customSort
	{
		uint32 GetLevelValue(ItemPrototype const* item) const
		{
			uint32 level = item->RequiredLevel;
			if (!level)
				level = item->SourceQuestLevel;
			if (!level || level == -1)
				level = item->ItemLevel;
			return level;
		}
		bool operator()(ItemPrototype const* a, ItemPrototype const* b) const
		{
			uint32 levelA = GetLevelValue(a);
			uint32 levelB = GetLevelValue(b);
			return levelA < levelB;
		}
	} sort;
	std::sort(result.begin(), result.end(), sort);
	for (auto& proto : result)
		Item_PrintItem(proto);
	return 0;
}


int LuaBindsAI::Item_PrintRandomEnchants(lua_State* L)
{
	LuaAI_Item* item = Item_GetItemObject(L, 1);
	uint32 randomPropertyId = item->proto->RandomProperty;
	if (!randomPropertyId)
		return 0;

	std::vector<uint32> enchants;
	GetAllEnchantsForRandomProperty(randomPropertyId, enchants);

	printf("Item %s [%d] random property id = %d\n", item->proto->Name1, item->proto->ItemId, randomPropertyId);

	for (const auto& randomId : enchants)
	{
		printf("  RandomID = %d\n", randomId);
		const ItemRandomPropertiesEntry* entry = sItemRandomPropertiesStore.LookupEntry(randomId);
		if (!entry)
			continue;

		for (const auto& enchantId : entry->enchant_id)
		{
			if (!enchantId)
				continue;

			printf("    EnchantID = %d\n", enchantId);
			const SpellItemEnchantmentEntry* enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
			if (!enchant)
				continue;

			printf("      %s\n", enchant->description[0]);
			for (int i = 0; i < 3; ++i)
			{
				if (!enchant->spellid[i])
					continue;

				const SpellEntry* spell = sSpellMgr.GetSpellEntry(enchant->spellid[i]);
				printf("        Spell %d", enchant->spellid[i]);
				if (!spell)
				{
					printf(" doesn't exist\n");
					continue;
				}
				printf("\n");
				for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
				{
					if (spell->Effect[j])
						printf("          Effect %d Value %d\n", spell->Effect[j], spell->CalculateSimpleValue(SpellEffectIndex(j)));
				}
			}
		}
	}
	return 0;
}
