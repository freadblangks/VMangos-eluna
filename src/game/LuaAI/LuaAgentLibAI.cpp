
#include "LuaAgentLibPlayer.h"
#include "LuaAgentLibAI.h"
#include "LuaAgentUtils.h"
#include "LuaAgent.h"

void LuaBindsAI::BindAI(lua_State* L)
{
	AI_CreateMetatable( L );
}


LuaAgent* LuaBindsAI::AI_GetAIObject(lua_State* L, int idx)
{
	return *((LuaAgent**) luaL_checkudata(L, idx, LuaAgent::AI_MTNAME));
}


void LuaBindsAI::AI_CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, LuaAgent::AI_MTNAME);
	lua_pushvalue(L, -1); // copy mt cos setfield pops
	lua_setfield(L, -1, "__index"); // mt.__index = mt
	luaL_setfuncs(L, AI_BindLib, 0); // copy funcs
	lua_pop(L, 1); // pop mt
}


int LuaBindsAI::AI_AddTopGoal(lua_State* L)
{
	int nArgs = lua_gettop(L);

	if (nArgs < 3)
		luaL_error(L, "AI.AddTopGoal - invalid number of arguments. 3 min, %d given", nArgs);

	LuaAgent* ai = AI_GetAIObject(L);
	int goalId = luaL_checknumber(L, 2);

	Goal* topGoal = ai->GetTopGoal();

	if (goalId != topGoal->GetGoalId() || topGoal->GetTerminated())
	{
		double life = luaL_checknumber(L, 3);

		std::vector<GoalParamP> params;
		Goal_GrabParams(L, nArgs, params);

		Goal** goalUserdata = Goal_CreateGoalUD(L, ai->AddTopGoal(goalId, life, params, L)); // ud on top of the stack
		// duplicate userdata for return result
		lua_pushvalue(L, -1);
		// save userdata
		(*goalUserdata)->SetRef(luaL_ref(L, LUA_REGISTRYINDEX)); // pops the object as well
		(*goalUserdata)->CreateUsertable();
	}
	else
		// leave topgoal's userdata as return result for lua
		lua_rawgeti(L, LUA_REGISTRYINDEX, topGoal->GetRef());

	return 1;
}


int LuaBindsAI::AI_EquipCopyFromMaster(lua_State* L)
{
	LuaAgent* ai = AI_GetAIObject(L);
	lua_pushboolean(L, ai->EquipCopyFromMaster());
	return 1;
}


int LuaBindsAI::AI_EquipItem(lua_State* L)
{
	LuaAgent* ai = AI_GetAIObject(L);
	int itemID = luaL_checkinteger(L, 2);
	int enchantId = 0;
	int randomPropertyId = 0;
	if (lua_gettop(L) > 2)
	{
		enchantId = luaL_checkinteger(L, 3);
		if (lua_gettop(L) > 3)
			randomPropertyId = luaL_checkinteger(L, 4);
	}
	lua_pushboolean(L, ai->EquipItem(itemID, enchantId, randomPropertyId));
	return 1;
}


int LuaBindsAI::AI_EquipDestroyAll(lua_State* L)
{
	LuaAgent* ai = AI_GetAIObject(L);
	ai->EquipDestroyAll();
	return 0;
}


int LuaBindsAI::AI_EquipGetEnchantId(lua_State* L)
{
	LuaAgent* ai = AI_GetAIObject(L);
	int islot = luaL_checkinteger(L, 2);
	int iitemSlot = luaL_checkinteger(L, 3);
	lua_pushinteger(L, ai->EquipGetEnchantId(EnchantmentSlot(islot), EquipmentSlots(iitemSlot)));
	return 1;
}


int LuaBindsAI::AI_EquipGetRandomProp(lua_State* L)
{
	LuaAgent* ai = AI_GetAIObject(L);
	int iitemSlot = luaL_checkinteger(L, 2);
	lua_pushinteger(L, ai->EquipGetRandomProp(EquipmentSlots(iitemSlot)));
	return 1;
}


int LuaBindsAI::AI_EquipPrint(lua_State* L)
{
	LuaAgent* ai = AI_GetAIObject(L);
	ai->EquipPrint();
	return 0;
}


int LuaBindsAI::AI_UpdateVisibilityForMaster(lua_State* L)
{
	AI_GetAIObject(L)->UpdateVisibilityForMaster();
	return 0;
}


int LuaBindsAI::AI_GetSpec(lua_State* L)
{
	LuaAgent* agent = AI_GetAIObject(L);
	lua_pushstring(L, agent->GetSpec().c_str());
	return 1;
}


int LuaBindsAI::AI_GetMaster(lua_State* L)
{
	LuaAgent* agent = AI_GetAIObject(L);
	Player* master = sObjectAccessor.FindPlayer(agent->GetMasterGuid());
	if (master && (!master->IsInWorld() || master->IsBeingTeleported()))
		master = nullptr;
	lua_pushplayerornil(L, master);
	return 1;
}


int LuaBindsAI::AI_GetPlayer(lua_State* L)
{
	LuaAgent* agent = AI_GetAIObject(L);
	agent->PushPlayerUD(L);
	return 1;
}

