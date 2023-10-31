#include "LuaAgentUtils.h"
#include "LuaAgentLibPlayer.h"
#include "LuaAgentLibUnit.h"


// lua_pcall wrapper that adds stack traceback to error msg
int lua_dopcall(lua_State* L, int narg, int nres) {
	int status;
	int base = lua_gettop(L) - narg;
	lua_pushcfunction(L, lua_errormsghandler);
	lua_insert(L, base);
	status = lua_pcall(L, narg, nres, base);
	lua_remove(L, base);
	return status;
}


// message function for lua_pcall to add stacktrace
int lua_errormsghandler(lua_State* L) {
	const char* msg = lua_tostring(L, 1);
	if (msg == NULL)
		if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)
			return 1;
		else
			msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
	luaL_traceback(L, L, msg, 1);
	return 1;
}


bool luaL_checkboolean(lua_State* L, int idx) {
	bool result = false;
	if (lua_isboolean(L, idx))
		result = lua_toboolean(L, idx);
	else
		luaL_error(L, "Invalid argument %d type. Boolean expected, got %s", idx, lua_typename(L, lua_type(L, idx)));
	return result;
}


void lua_pushplayerornil(lua_State* L, Player* u) {
	if (u)
		LuaBindsAI::Player_CreateUD(u, L);
	else
		lua_pushnil(L);
}


void lua_pushunitornil(lua_State* L, Unit* u) {
	if (u)
		LuaBindsAI::Unit_CreateUD(u, L);
	else
		lua_pushnil(L);
}


void* luaL_checkudwithfield(lua_State* L, int idx, const char* fieldName) {
	void* p = lua_touserdata(L, idx);
	if (p != NULL)  /* value is a userdata? */
		if (lua_getmetatable(L, idx)) {  /* does it have a metatable? */
			if (lua_getfield(L, -1, fieldName))
				if (lua_toboolean(L, -1)) {
					lua_pop(L, 2);  /* remove metatable and field value */
					return p;
				}
			lua_pop(L, 2);  /* remove metatable and field value */
		}
	luaL_error(L, "Invalid argument type. Userdata expected, got %s", lua_typename(L, lua_type(L, idx)));
}

