
#include "LuaAgentLibPlayer.h"
#include "LuaAgentLibAux.h"
#include "LuaAgentLibWorldObj.h"
#include "LuaAgentUtils.h"


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
