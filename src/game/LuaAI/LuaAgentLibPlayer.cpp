
#include "LuaAgentLibPlayer.h"
#include "LuaAgentLibWorldObj.h"
#include "LuaAgentLibUnit.h"
#include "LuaAgentUtils.h"
#include "Spell.h"
#include "Group.h"


namespace
{
	inline const TalentEntry* Player_TalentGetInfo(lua_State* L, Player* me, uint32 talentID)
	{
		TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentID);
		if (!talentInfo)
			luaL_error(L, "Player_TalentGetInfo: talent doesn't exist %d", talentID);

		TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
		if (!talentTabInfo)
			luaL_error(L, "Player_TalentGetInfo: talent tab not found for talent %d", talentID);

		uint32 classMask = me->GetClassMask();
		if ((classMask & talentTabInfo->ClassMask) == 0)
			luaL_error(L, "Player_TalentGetInfo: class mask and talent class mask do not match cls = %d, talent = %d", classMask, talentTabInfo->ClassMask);

		return talentInfo;
	}


	inline uint32 Player_TalentGetRankSpellId(lua_State* L, Player* me, const TalentEntry* talentInfo, uint32 talentID, uint32 rank)
	{
		if (rank >= MAX_TALENT_RANK)
			luaL_error(L, "Player_TalentGetRankSpellId: talent rank cannot exceed %d", MAX_TALENT_RANK - 1);

		// search specified rank
		uint32 spellid = talentInfo->RankID[rank];
		if (!spellid)                                       // ??? none spells in talent
			luaL_error(L, "Player_TalentGetRankSpellId: talent %d rank %d not found", talentID, rank);

		SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spellid);
		if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, me, false))
			luaL_error(L, "Player_TalentGetRankSpellId: talent %d spell %d is not valid for player or doesn't exist", talentID, spellid);

		return spellid;
	}
}


void LuaBindsAI::Player_CreateUD(Player* player, lua_State* L) {
	// create userdata on top of the stack pointing to a pointer of an AI object
	Player** playerud = static_cast<Player**>(lua_newuserdatauv(L, sizeof(Player*), 0));
	*playerud = player; // swap the AI object being pointed to to the current instance
	luaL_setmetatable(L, LuaBindsAI::PlayerMtName);
}


void LuaBindsAI::BindPlayer(lua_State* L) {
	Player_CreateMetatable(L);
}


Player* LuaBindsAI::Player_GetPlayerObject(lua_State* L, int idx) {
	return *((Player**) luaL_checkudata(L, idx, LuaBindsAI::PlayerMtName));
}


int LuaBindsAI_Player_CompareEquality(lua_State* L) {
	WorldObject* obj1 = *LuaBindsAI::WObj_GetWObjObject(L);
	WorldObject* obj2 = *LuaBindsAI::WObj_GetWObjObject(L, 2);
	lua_pushboolean(L, obj1 == obj2);
	return 1;
}


void LuaBindsAI::Player_CreateMetatable(lua_State* L) {
	luaL_newmetatable(L, LuaBindsAI::PlayerMtName);
	lua_pushvalue(L, -1); // copy mt cos setfield pops
	lua_setfield(L, -1, "__index"); // mt.__index = mt
	luaL_setfuncs(L, Unit_BindLib, 0); // copy funcs
	luaL_setfuncs(L, Player_BindLib, 0);
	lua_pushcfunction(L, LuaBindsAI_Player_CompareEquality);
	lua_setfield(L, -2, "__eq");
	lua_pushboolean(L, true);
	lua_setfield(L, -2, "isWorldObject");
	lua_pushboolean(L, true);
	lua_setfield(L, -2, "isUnit");
	lua_pop(L, 1); // pop mt
}


int LuaBindsAI::Player_HasSpell(lua_State* L)
{
	Player* me = Player_GetPlayerObject(L);
	lua_Integer spellID = luaL_checkinteger(L, 2);
	if (!sSpellMgr.GetSpellEntry(spellID))
		luaL_error(L, "Player_HasSpell: spell %d not found", spellID);
	lua_pushboolean(L, me->HasSpell(spellID));
	return 1;
}


int LuaBindsAI::Player_LearnSpell(lua_State* L)
{
	Player* me = Player_GetPlayerObject(L);
	lua_Integer spellID = luaL_checkinteger(L, 2);
	if (!sSpellMgr.GetSpellEntry(spellID))
		luaL_error(L, "Player_LearnSpell: spell %d not found", spellID);
	if (!me->HasSpell(spellID))
		me->LearnSpell(spellID, false);
	return 0;
}


int LuaBindsAI::Player_GetGroupMemberCount(lua_State* L) {
	Player* player = Player_GetPlayerObject(L);
	if (Group* grp = player->GetGroup())
		lua_pushinteger(L, grp->GetMembersCount());
	else
		lua_pushinteger(L, 1);
	return 1;
}


int LuaBindsAI::Player_IsInSameSubGroup(lua_State* L)
{
	Player* me = Player_GetPlayerObject(L);
	Player* other = Player_GetPlayerObject(L, 2);
	lua_pushboolean(L, me->IsInSameGroupWith(other));
	return 1;
}


int LuaBindsAI::Player_GetTalentTbl(lua_State* L) {
	Player* me = Player_GetPlayerObject(L);

	// from cs_learn.cpp
	Player* player = me;
	uint32 classMask = player->GetClassMask();

	lua_newtable(L);
	int tblIdx = 1;

	for (uint32 i = 0; i < sTalentStore.GetNumRows(); ++i)
	{
		TalentEntry const* talentInfo = sTalentStore.LookupEntry(i);
		if (!talentInfo)
			continue;

		TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
		if (!talentTabInfo)
			continue;

		if ((classMask & talentTabInfo->ClassMask) == 0)
			continue;

		uint32 spellId = 0;
		uint8 rankId = MAX_TALENT_RANK;
		for (int8 rank = MAX_TALENT_RANK - 1; rank >= 0; --rank)
		{
			if (talentInfo->RankID[rank] != 0)
			{
				rankId = rank;
				spellId = talentInfo->RankID[rank];
				break;
			}
		}

		if (!spellId || rankId == MAX_TALENT_RANK)
			continue;

		SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spellId);
		if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo))
			continue;

		lua_newtable(L);
		lua_newtable(L);
		for (lua_Integer rank = 0; rank <= rankId; ++rank)
			if (talentInfo->RankID[rank] != 0) {
				lua_pushinteger(L, talentInfo->RankID[rank]);
				lua_seti(L, -2, rank + 1);
			}
		lua_setfield(L, -2, "RankID");
		lua_pushinteger(L, spellId);
		lua_setfield(L, -2, "spellID");
		lua_pushinteger(L, rankId);
		lua_setfield(L, -2, "maxRankID");
		lua_pushinteger(L, talentInfo->TalentID);
		lua_setfield(L, -2, "TalentID");
		lua_pushinteger(L, talentInfo->Col);
		lua_setfield(L, -2, "Col");
		lua_pushinteger(L, talentInfo->Row);
		lua_setfield(L, -2, "Row");
		lua_pushinteger(L, talentInfo->TalentTab);
		lua_setfield(L, -2, "TalentTab");
		lua_pushinteger(L, talentInfo->DependsOn);
		lua_setfield(L, -2, "DependsOn");
		lua_pushinteger(L, talentInfo->DependsOnRank);
		lua_setfield(L, -2, "DependsOnRank");
		lua_pushstring(L, spellInfo->SpellName[0].c_str());
		lua_setfield(L, -2, "spellName");
		lua_pushinteger(L, talentTabInfo->TalentTabID);
		lua_setfield(L, -2, "tabID");
		lua_seti(L, -2, tblIdx);
		++tblIdx;
	}

	return 1;
}


int LuaBindsAI::Player_GetTalentRank(lua_State* L)
{
	Player* me = Player_GetPlayerObject(L);
	uint32 talentID = luaL_checkinteger(L, 2);
	const TalentEntry* talentInfo = Player_TalentGetInfo(L, me, talentID);

	// search for rank
	uint32 spellid = 0;
	int result = -1;
	for (int rank = MAX_TALENT_RANK - 1; rank >= 0; --rank)
	{
		spellid = talentInfo->RankID[rank];
		if (spellid != 0)
		{
			SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spellid);
			if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, me, false))
				continue;

			if (me->HasSpell(spellid))
			{
				result = rank;
				break;
			}
		}
	}

	lua_pushinteger(L, result);
	return 1;
}


int LuaBindsAI::Player_HasTalent(lua_State* L)
{
	Player* me = Player_GetPlayerObject(L);
	uint32 talentID = luaL_checkinteger(L, 2);
	uint32 spellid = Player_TalentGetRankSpellId(L, me, Player_TalentGetInfo(L, me, talentID), talentID, luaL_checkinteger(L, 3));
	lua_pushboolean(L, me->HasSpell(spellid));
	return 1;
}


int LuaBindsAI::Player_LearnTalent(lua_State* L)
{
	Player* me = Player_GetPlayerObject(L);
	uint32 talentID = luaL_checkinteger(L, 2);
	uint32 talentRank = luaL_checkinteger(L, 3);
	uint32 spellid = Player_TalentGetRankSpellId(L, me, Player_TalentGetInfo(L, me, talentID), talentID, talentRank);

	// learn highest rank of talent and learn all non-talent spell ranks (recursive by tree)
	me->LearnTalent(talentID, talentRank);
	return 0;
}


int LuaBindsAI::Player_ResetTalents(lua_State* L)
{
	Player* me = Player_GetPlayerObject(L);
	lua_pushboolean(L, me->ResetTalents(true));
	return 1;
}


int LuaBindsAI::Player_SendCastSpellUnit(lua_State* L)
{
	Player* me = Player_GetPlayerObject(L);
	Unit* target = Unit_GetUnitObject(L, 2);
	lua_Integer spellId = luaL_checkinteger(L, 3);
	if (const SpellEntry* spell = sSpellMgr.GetSpellEntry(spellId))
	{
		std::unique_ptr<WorldPacket> packet = std::make_unique<WorldPacket>(CMSG_CAST_SPELL);
		*packet << spell->Id;
		SpellCastTargets targets;
		targets.setUnitTarget(target);
		*packet << targets;
		me->GetSession()->QueuePacket(std::move(packet));
	}
	else
		luaL_error(L, "Player_SendCastSpellUnit: spell %d doesn't exist", spellId);
	return 0;
}


int LuaBindsAI::Player_GetComboPoints(lua_State* L)
{
	Player* player = Player_GetPlayerObject(L);
	lua_pushinteger(L, player->GetComboPoints());
	return 1;
}
