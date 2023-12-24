
#include "LuaAgentLibWorldObj.h"
#include "LuaAgentUtils.h"
#include "lua.hpp"


void LuaBindsAI::BindWorldObject(lua_State* L)
{
	WObj_CreateMetatable(L);
	Guid_CreateMetatable(L);
}


void LuaBindsAI::WObj_CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, LuaBindsAI::WorldObjMtName);
	//lua_pushvalue(L, -1); // copy mt cos setfield pops
	//lua_setfield(L, -1, "__index"); // mt.__index = mt
	//luaL_setfuncs(L, Unit_BindLib, 0); // copy funcs
	lua_pop(L, 1); // pop mt
}


WorldObject** LuaBindsAI::WObj_GetWObjObject(lua_State* L, int idx)
{
	return (WorldObject**) luaL_checkudwithfield(L, idx, "isWorldObject");
}


int LuaBindsAI_Guid_CompareEquality(lua_State* L)
{
	LuaObjectGuid* guid1 = LuaBindsAI::Guid_GetGuidObject(L, 1);
	LuaObjectGuid* guid2 = LuaBindsAI::Guid_GetGuidObject(L, 2);
	lua_pushboolean(L, guid1->guid == guid2->guid);
	return 1;
}


void LuaBindsAI::Guid_CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, LuaBindsAI::GuidMtName);
	lua_pushvalue(L, -1); // copy mt cos setfield pops
	lua_setfield(L, -1, "__index"); // mt.__index = mt
	luaL_setfuncs(L, Guid_BindLib, 0); // copy funcs
	lua_pushcfunction(L, LuaBindsAI_Guid_CompareEquality);
	lua_setfield(L, -2, "__eq");
	lua_pop(L, 1); // pop mt
}


void LuaBindsAI::Guid_CreateUD(lua_State* L, const ObjectGuid& guid)
{
	LuaObjectGuid* pGuid = static_cast<LuaObjectGuid*>(lua_newuserdatauv(L, sizeof(LuaObjectGuid), 0));
	luaL_setmetatable(L, LuaBindsAI::GuidMtName);
	pGuid->Set(guid);
}


LuaObjectGuid* LuaBindsAI::Guid_GetGuidObject(lua_State* L, int idx)
{
	return (LuaObjectGuid*) luaL_checkudata(L, idx, LuaBindsAI::GuidMtName);
}


int LuaBindsAI::Guid_Empty(lua_State* L)
{
	LuaObjectGuid* guid = Guid_GetGuidObject(L);
	lua_pushboolean(L, guid->guid.IsEmpty());
	return 1;
}


int LuaBindsAI::Guid_GetId(lua_State* L)
{
	LuaObjectGuid* guid = Guid_GetGuidObject(L);
	if (!guid->guid.IsPlayer())
		luaL_error(L, "Guid_GetId: only player guid is supported");
	lua_pushinteger(L, guid->guid.GetCounter());
	return 1;
}


int LuaBindsAI::Guid_Print(lua_State* L)
{
	LuaObjectGuid* guid = Guid_GetGuidObject(L);
	printf("%llu\n", guid->guid.GetRawValue());
	return 0;
}
