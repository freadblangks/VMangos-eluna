
#include "LuaAgentLibPlayer.h"
#include "LuaAgentLibUnit.h"
#include "LuaAgentLibAux.h"
#include "LuaAgentLibWorldObj.h"
#include "LuaAgentUtils.h"
#include "GridSearchers.h"
#include "ScriptedInstance.h"
#include "eastern_kingdoms/dun_morogh/gnomeregan/gnomeregan.h"


enum
{
	NPC_WALKING_BOMB = 7915
};


void LuaBindsAI::BindLibAux(lua_State* L)
{
	lua_register(L, "GetUnitByGuid", GetUnitByGuid);
	lua_register(L, "GetUnitByGuidEx", GetUnitByGuidEx);
	lua_register(L, "GetPlayerByGuid", GetPlayerByGuid);
	lua_register(L, "GetPlayerById", GetPlayerById);
	lua_register(L, "GetSpellName", GetSpellName);
	lua_register(L, "isinteger", isinteger);
	lua_register(L, "Gnomeregan_GetFaceData", Gnomeregan_GetFaceData);
	lua_register(L, "Gnomeregan_GetBombs", Gnomeregan_GetBombs);
}


int LuaBindsAI::isinteger(lua_State* L)
{
	lua_pushboolean(L, lua_isinteger(L, 1));
	return 1;
}


int LuaBindsAI::GetUnitByGuid(lua_State* L) {
	Player* player = Player_GetPlayerObject(L);
	const ObjectGuid& guid = Guid_GetGuidObject(L, 2)->guid;
	if (guid.IsEmpty())
		luaL_error(L, "GetUnitByGuid: guid was empty");
	lua_pushunitornil(L, player->GetMap()->GetUnit(guid));
	return 1;
}


int LuaBindsAI::GetUnitByGuidEx(lua_State* L) {
	Player* player = Player_GetPlayerObject(L);
	lua_Integer e = luaL_checkinteger(L, 2);
	lua_Integer c = luaL_checkinteger(L, 3);
	if (e < 0 || c < 0)
		luaL_error(L, "GetUnitByGuidEx: got negative number");
	ObjectGuid guid(HighGuid::HIGHGUID_UNIT, uint32(e), uint32(c));
	lua_pushunitornil(L, player->GetMap()->GetUnit(guid));
	return 1;
}


int LuaBindsAI::GetPlayerByGuid(lua_State* L) {
	const ObjectGuid& guid = Guid_GetGuidObject(L)->guid;
	if (guid.IsEmpty())
		luaL_error(L, "GetPlayerByGuid: guid was empty");
	lua_pushplayerornil(L, sObjectAccessor.FindPlayer(guid));
	return 1;
}


int LuaBindsAI::GetPlayerById(lua_State* L) {
	lua_Integer id = luaL_checkinteger(L, 1);
	if (id <= 0 || id > std::numeric_limits<uint32>::max())
		luaL_error(L, "GetPlayerById: id must be a valid uint32 != 0, got %I", id);
	ObjectGuid guid(HighGuid::HIGHGUID_PLAYER, uint32(id));
	lua_pushplayerornil(L, sObjectAccessor.FindPlayer(guid));
	return 1;
}


int LuaBindsAI::GetSpellName(lua_State* L) {
	lua_Integer spellID = luaL_checkinteger(L, 1);
	auto info_orig = sSpellMgr.GetSpellEntry(spellID);
	// spell not found
	if (!info_orig)
		luaL_error(L, "GetSpellName: spell %d not found", spellID);
	lua_pushstring(L, info_orig->SpellName[0].c_str());
	return 1;
}


int LuaBindsAI::Gnomeregan_GetFaceData(lua_State* L)
{
	Player* player = Player_GetPlayerObject(L);
	if (player->GetZoneId() == 721u)
	{
		if (instance_gnomeregan* gnome = (instance_gnomeregan*) player->GetInstanceData())
		{
			const uint32 btnGuids[6] = {6582u, 6726u, 6748u, 11977u, 14140u, 15766u};
			const uint32 btnEntry[6] = {GO_BUTTON_1, GO_BUTTON_2, GO_BUTTON_3, GO_BUTTON_4, GO_BUTTON_5, GO_BUTTON_6};

			sBombFace* faces = gnome->GetBombFaces();
			lua_newtable(L);
			for (int i = 0; i < MAX_GNOME_FACES; ++i)
			{
				lua_newtable(L);
				lua_pushboolean(L, faces[i].m_bActivated);
				lua_setfield(L, -2, "active");
				lua_pushinteger(L, faces[i].m_uiBombTimer);
				lua_setfield(L, -2, "timer");
				Guid_CreateUD(L, ObjectGuid(faces[i].m_uiGnomeFaceGUID));
				lua_setfield(L, -2, "guid");
				ObjectGuid btnGuid(HIGHGUID_GAMEOBJECT, btnEntry[i], btnGuids[i]);
				Guid_CreateUD(L, btnGuid);
				lua_setfield(L, -2, "btnGuid");
				lua_seti(L, -2, i + 1);
			}
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}


int LuaBindsAI::Gnomeregan_GetBombs(lua_State* L)
{
	Player* player = Player_GetPlayerObject(L);
	std::list<Creature*> bombList;
	GetCreatureListWithEntryInGrid(bombList, player, NPC_WALKING_BOMB, 250.0f);
	lua_newtable(L);
	lua_Integer idx = 1;
	for (auto& bomb : bombList)
	{
		lua_pushunitornil(L, bomb);
		lua_seti(L, -2, idx);
		++idx;
	}
	return 1;
}
