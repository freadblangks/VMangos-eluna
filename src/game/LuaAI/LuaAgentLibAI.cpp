
#include "LuaAgentLibPlayer.h"
#include "LuaAgentLibAI.h"
#include "LuaAgentUtils.h"
#include "LuaAgent.h"

void LuaBindsAI::BindAI( lua_State* L )
{
	AI_CreateMetatable( L );
}


LuaAgent** LuaBindsAI::AI_GetAIObject( lua_State* L )
{
	return (LuaAgent**) luaL_checkudata( L, 1, LuaAgent::AI_MTNAME );
}


void LuaBindsAI::AI_CreateMetatable( lua_State* L )
{
	luaL_newmetatable( L, LuaAgent::AI_MTNAME );
	lua_pushvalue( L, -1 ); // copy mt cos setfield pops
	lua_setfield( L, -1, "__index" ); // mt.__index = mt
	luaL_setfuncs( L, AI_BindLib, 0 ); // copy funcs
	lua_pop( L, 1 ); // pop mt
}


int LuaBindsAI::AI_GetMaster(lua_State* L)
{
	LuaAgent* agent = *AI_GetAIObject(L);
	Player* master = sObjectAccessor.FindPlayer(agent->GetMasterGuid());
	if (master && (!master->IsInWorld() || master->IsBeingTeleported()))
		master = nullptr;
	lua_pushplayerornil(L, master);
	return 1;
}


int LuaBindsAI::AI_GetPlayer(lua_State* L)
{
	LuaAgent* agent = *AI_GetAIObject(L);
	agent->PushPlayerUD(L);
	return 1;
}

