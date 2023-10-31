#include "LuaAgentLibPlayer.h"
#include "LuaAgentLibWorldObj.h"
#include "LuaAgentLibUnit.h"


void LuaBindsAI::Player_CreateUD(Player* player, lua_State* L) {
	// create userdata on top of the stack pointing to a pointer of an AI object
	Player** playerud = static_cast<Player**>(lua_newuserdatauv(L, sizeof(Player*), 0));
	*playerud = player; // swap the AI object being pointed to to the current instance
	luaL_setmetatable(L, LuaBindsAI::PlayerMtName);
}


void LuaBindsAI::BindPlayer(lua_State* L) {
	Player_CreateMetatable(L);
}


Player** LuaBindsAI::Player_GetPlayerObject(lua_State* L, int idx) {
	return (Player**) luaL_checkudata(L, idx, LuaBindsAI::PlayerMtName);
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

