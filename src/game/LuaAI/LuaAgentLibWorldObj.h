#ifndef MANGOS_LuaAgentLibWorldObj_H
#define MANGOS_LuaAgentLibWorldObj_H

#include "lua.hpp"

struct LuaObjectGuid
{
	ObjectGuid guid;
	LuaObjectGuid() : guid() {}
	LuaObjectGuid(const ObjectGuid& guid) : guid(guid) {}
	void Set(const ObjectGuid& guid) { this->guid = guid; }
};

namespace LuaBindsAI {
	static const char* WorldObjMtName = "LuaAI.WorldObject";
	static const char* GuidMtName = "LuaAI.ObjectGuid";
	// Creates metatable for the AI userdata with name specified by AI::Unit_MTNAME
	void BindWorldObject(lua_State* L);
	// Creates metatable for the AI userdata with name specified by AI::Unit_MTNAME.
	// Registers all the functions listed in LuaBindsBot::Unit_BindLib with that metatable.
	void WObj_CreateMetatable(lua_State* L);
	WorldObject** WObj_GetWObjObject(lua_State* L, int idx = 1);

	void Guid_CreateMetatable(lua_State* L);
	void Guid_CreateUD(lua_State* L, const ObjectGuid& guid = ObjectGuid());
	LuaObjectGuid* Guid_GetGuidObject(lua_State* L, int idx = 1);

	int Guid_Print(lua_State* L);
	int Guid_Empty(lua_State* L);

	static const struct luaL_Reg Guid_BindLib[]{
		{"Empty", Guid_Empty},
		{"Print", Guid_Print},

		{NULL, NULL}
	};

}

#endif
