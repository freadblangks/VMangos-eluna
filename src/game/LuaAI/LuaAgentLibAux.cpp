
#include "LuaAgentLibPlayer.h"
#include "LuaAgentLibAux.h"
#include "LuaAgentLibWorldObj.h"
#include "LuaAgentUtils.h"


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
