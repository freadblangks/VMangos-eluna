#ifndef MANGOS_LuaAgentLibAux_H
#define MANGOS_LuaAgentLibAux_H

struct lua_State;

namespace LuaBindsAI {
	void BindLibAux(lua_State* L);

	int isinteger(lua_State* L);
	int GetUnitByGuid(lua_State* L);
	int GetUnitByGuidEx(lua_State* L);
	int GetPlayerByGuid(lua_State* L);
	int GetPlayerById(lua_State* L);
	int GetSpellName(lua_State* L);
	int GetUnitsWithEntryNear(lua_State* L);
	int GetObjectsWithEntryNear(lua_State* L);
	int GetObjectsWithEntryAround(lua_State* L);

	int GetObjectIsSpawned(lua_State* L);
	int GetObjectLootState(lua_State* L);
	int GetObjectGOState(lua_State* L);

	// instance scripts
	int Gnomeregan_GetFaceData(lua_State* L);
	int Gnomeregan_GetBombs(lua_State* L);
}

#endif
