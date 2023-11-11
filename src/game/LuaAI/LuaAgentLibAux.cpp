
#include "LuaAgentLibPlayer.h"
#include "LuaAgentLibUnit.h"
#include "LuaAgentLibAux.h"

bool is_uint64(const std::string& s)
{
	std::string::const_iterator it = s.begin();
	while (it != s.end() && std::isdigit(*it)) ++it;
	return !s.empty() && it == s.end() && s.size() <= 20;
}


uint64 LuaBindsAI::GetRawGuidFromString(lua_State* L, int n) {
	const char* guidStr = luaL_checkstring(L, n);
	if (is_uint64(guidStr)) {
		uint64 guid = 0;
		try {
			guid = std::stoull(guidStr);
			return guid;
		}
		catch (std::invalid_argument e) {
			sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_ERROR, "Failed to convert string to uint64, is_uint64 returned true. Str = %s", guidStr);
			luaL_error(L, "Exception caught. Passed string failed to convert to uint64 - %s\n", guidStr);
		}
		catch (std::out_of_range e) {
			sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_ERROR, "uint64 conversion overflow, is_uint64 returned true. Str = %s", guidStr);
			luaL_error(L, "Exception caught. Passed string overflowed uint64 - %s\n", guidStr);
		}
	}
	else
		luaL_error(L, "Passed string is not a valid uint64 number - %s\n", guidStr);
	return 0;
}


int LuaBindsAI::GetUnitByGuid(lua_State* L) {
	Player* ai = *Player_GetPlayerObject(L);
	std::string guidStr = luaL_checkstring(L, 2);

	if (is_uint64(guidStr)) {
		uint64 guid = 0;
		try {
			guid = std::stoull(guidStr);
		}
		catch (std::invalid_argument e) {
			sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_ERROR, "Failed to convert string to uint64, is_uint64 returned true. Str = %s", guidStr);
			luaL_error(L, "Exception caught. Passed string failed to convert to uint64 - %s\n", guidStr);
		}
		catch (std::out_of_range e) {
			sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_ERROR, "uint64 conversion overflow, is_uint64 returned true. Str = %s", guidStr);
			luaL_error(L, "Exception caught. Passed string overflowed uint64 - %s\n", guidStr);
		}
		ObjectGuid oGuid(guid);
		Unit* unit = ai->GetMap()->GetUnit(oGuid);
		if (unit)
			Unit_CreateUD(unit, L);
		else
			lua_pushnil(L);
	}
	else
		luaL_error(L, "Passed string is not a valid uint64 number - %s\n", guidStr);
	return 1;
}


int LuaBindsAI::GetPlayerByGuid(lua_State* L) {
	std::string guidStr = luaL_checkstring(L, 1);
	if (is_uint64(guidStr)) {
		uint64 guid = 0;
		try {
			guid = std::stoull(guidStr);
		}
		catch (std::invalid_argument e) {
			sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_ERROR, "Failed to convert string to uint64, is_uint64 returned true. Str = %s", guidStr);
			luaL_error(L, "Exception caught. Passed string failed to convert to uint64 - %s\n", guidStr);
		}
		catch (std::out_of_range e) {
			sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_ERROR, "uint64 conversion overflow, is_uint64 returned true. Str = %s", guidStr);
			luaL_error(L, "Exception caught. Passed string overflowed uint64 - %s\n", guidStr);
		}
		ObjectGuid oGuid(guid);
		Player* player = ObjectAccessor::FindPlayer(oGuid);
		if (player)
			Player_CreateUD(player, L);
		else
			lua_pushnil(L);
	}
	else
		luaL_error(L, "Passed string is not a valid uint64 number - %s\n", guidStr);
	return 1;
}


int LuaBindsAI::Items_PrintItemsOfType(lua_State* L)
{
	lua_Integer subclass = luaL_checkinteger(L, 1);
	std::vector<ItemPrototype const*> result;
	for (auto const& itr : sObjectMgr.GetItemPrototypeMap())
	{
		ItemPrototype const* pProto = &itr.second;

		// Only gear and weapons
		if (pProto->Class != ITEM_CLASS_WEAPON && pProto->Class != ITEM_CLASS_ARMOR)
			continue;

		if (pProto->SubClass != subclass)
			continue;

		auto racemask = RACEMASK_ALL_PLAYABLE;
		auto classmask = CLASSMASK_ALL_PLAYABLE;
		auto race = pProto->AllowableRace & racemask;
		auto cls = pProto->AllowableClass & classmask;
		auto raceQ = pProto->SourceQuestRaces ? pProto->SourceQuestRaces & racemask : 1;
		auto clsQ = pProto->SourceQuestClasses ? pProto->SourceQuestClasses & classmask : 1;

		if (!race || !cls || !raceQ || !clsQ)
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
	{
		printf("%d, -- %d, %d, %d, %s, (", proto->ItemId, proto->RequiredLevel, proto->ItemLevel, proto->SourceQuestLevel, proto->Name1);
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
		printf(")\n");
	}
	return 0;
}

