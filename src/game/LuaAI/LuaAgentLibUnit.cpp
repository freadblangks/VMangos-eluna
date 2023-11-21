
#include "LuaAgentUtils.h"
#include "LuaAgentLibUnit.h"
#include "LuaAgentLibWorldObj.h"


void LuaBindsAI::BindUnit(lua_State* L) {
	Unit_CreateMetatable(L);
}


Unit** LuaBindsAI::Unit_GetUnitObject(lua_State* L, int idx) {
	return (Unit**) luaL_checkudwithfield(L, idx, "isUnit");//(Unit**) luaL_checkudata(L, idx, LuaBindsAI::UnitMtName);
}


void LuaBindsAI::Unit_CreateUD(Unit* unit, lua_State* L) {
	// create userdata on top of the stack pointing to a pointer of an AI object
	Unit** unitud = static_cast<Unit**>(lua_newuserdatauv(L, sizeof(Unit*), 0));
	*unitud = unit; // swap the AI object being pointed to to the current instance
	luaL_setmetatable(L, LuaBindsAI::UnitMtName);
}


int LuaBindsAI_Unit_CompareEquality(lua_State* L) {
	WorldObject* obj1 = *LuaBindsAI::WObj_GetWObjObject(L);
	WorldObject* obj2 = *LuaBindsAI::WObj_GetWObjObject(L, 2);
	lua_pushboolean(L, obj1 == obj2);
	return 1;
}


void LuaBindsAI::Unit_CreateMetatable(lua_State* L) {
	luaL_newmetatable(L, LuaBindsAI::UnitMtName);
	lua_pushvalue(L, -1); // copy mt cos setfield pops
	lua_setfield(L, -1, "__index"); // mt.__index = mt
	luaL_setfuncs(L, Unit_BindLib, 0); // copy funcs
	lua_pushcfunction(L, LuaBindsAI_Unit_CompareEquality);
	lua_setfield(L, -2, "__eq");
	lua_pushboolean(L, true);
	lua_setfield(L, -2, "isWorldObject");
	lua_pushboolean(L, true);
	lua_setfield(L, -2, "isUnit");
	lua_pop(L, 1); // pop mt
}


// ---------------------------------------------------------
// --                General Info
// ---------------------------------------------------------


int LuaBindsAI::Unit_GetClass(lua_State* L)
{
	Unit* unit = *Unit_GetUnitObject(L);
	lua_pushinteger(L, unit->GetClass());
	return 1;
}


int LuaBindsAI::Unit_GetGuid(lua_State* L)
{
	Unit* unit = *Unit_GetUnitObject(L);
	Guid_CreateUD(L, unit->GetObjectGuid());
	return 1;
}


int LuaBindsAI::Unit_GetLevel(lua_State* L)
{
	Unit* unit = *Unit_GetUnitObject(L);
	lua_pushinteger(L, unit->GetLevel());
	return 1;
}


int LuaBindsAI::Unit_GetName(lua_State* L)
{
	Unit* unit = *Unit_GetUnitObject(L);
	lua_pushstring(L, unit->GetName());
	return 1;
}


// ---------------------------------------------------------
// --                Movement Control
// ---------------------------------------------------------


int LuaBindsAI::Unit_ClearMotion(lua_State* L)
{
	Unit* unit = *Unit_GetUnitObject(L);
	unit->GetMotionMaster()->Clear();
	return 0;
}


int LuaBindsAI::Unit_GetMotionType(lua_State* L)
{
	Unit* unit = *Unit_GetUnitObject(L);
	lua_pushnumber(L, unit->GetMotionMaster()->GetCurrentMovementGeneratorType());
	return 1;
}


int LuaBindsAI::Unit_MoveFollow(lua_State* L)
{
	Unit* unit = *Unit_GetUnitObject(L);
	Unit* target = *Unit_GetUnitObject(L, 2);
	double offset = luaL_checknumber(L, 3);
	double angle = luaL_checknumber(L, 4);
	unit->GetMotionMaster()->LuaAIMoveFollow(target, offset, angle);
	return 0;
}

