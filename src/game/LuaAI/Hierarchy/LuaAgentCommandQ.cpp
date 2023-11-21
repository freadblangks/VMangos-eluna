
#include "LuaAgentCommandQ.h"
#include "lua.hpp"
#include "LuaAI/LuaAgentLibWorldObj.h"


int AgentCmd::PushType(lua_State* L)
{
	lua_pushinteger(L, lua_Integer(tp));
	return 1;
}


int AgentCmdMove::Push(lua_State* L)
{
	lua_pushnumber(L, destination.x);
	lua_pushnumber(L, destination.y);
	lua_pushnumber(L, destination.z);
	return 3;
}


int AgentCmdFollow::Push(lua_State* L)
{
	LuaBindsAI::Guid_CreateUD(L, targetGuid);
	return 1;
}
