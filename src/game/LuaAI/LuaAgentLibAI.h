#ifndef MANGOS_LuaAgentLibAI_H
#define MANGOS_LuaAgentLibAI_H

#include "lua.hpp"

class LuaAgent;

namespace LuaBindsAI {

	// Creates metatable for the AI userdata with name specified by AI::AI_MTNAME
	void BindAI(lua_State* L);
	// Creates metatable for the AI userdata with name specified by AI::AI_MTNAME.
	// Registers all the functions listed in LuaBindsAI::AI_BindLib with that metatable.
	void AI_CreateMetatable(lua_State* L);
	LuaAgent* AI_GetAIObject(lua_State* L, int idx = 1);

	int AI_AddTopGoal(lua_State* L);
	int AI_GetUserTbl(lua_State* L);

	int AI_GetStdThreat(lua_State* L);
	int AI_SetStdThreat(lua_State* L);
	int AI_GetHealTarget(lua_State* L);
	int AI_SetHealTarget(lua_State* L);

	int AI_IsFollowing(lua_State* L);

	// equip

	int AI_EquipCopyFromMaster(lua_State* L);
	int AI_EquipItem(lua_State* L);
	int AI_EquipDestroyAll(lua_State* L);
	int AI_EquipGetEnchantId(lua_State* L);
	int AI_EquipGetRandomProp(lua_State* L);
	int AI_EquipPrint(lua_State* L);
	int AI_UpdateVisibilityForMaster(lua_State* L);

	// commands

	int AI_CmdAddProgress(lua_State* L);
	int AI_CmdGetProgress(lua_State* L);
	int AI_CmdSetProgress(lua_State* L);
	int AI_CmdGetArgs(lua_State* L);
	int AI_CmdGetState(lua_State* L);
	int AI_CmdGetType(lua_State* L);
	int AI_CmdGetQMode(lua_State* L);
	int AI_CmdComplete(lua_State* L);
	int AI_CmdIsRequirementMet(lua_State* L);
	int AI_CmdSetInProgress(lua_State* L);
	int AI_CmdFail(lua_State* L);
	int AI_CmdPrintAll(lua_State* L);

	int AI_GetSpec(lua_State* L);
	int AI_GetRole(lua_State* L);
	int AI_GetPartyIntelligence(lua_State* L);
	int AI_GetPlayer(lua_State* L);
	int AI_GetMaster(lua_State* L);
	int AI_GetMasterGuid(lua_State* L);

	int AI_SetRole(lua_State* L);

	// spell

	int AI_GetSpellChainFirst(lua_State* L);
	int AI_GetSpellChainPrev(lua_State* L);
	int AI_GetSpellName(lua_State* L);
	int AI_GetSpellLevel(lua_State* L);
	int AI_GetSpellRank(lua_State* L);
	int AI_GetSpellOfRank(lua_State* L);
	int AI_GetSpellMaxRankForLevel(lua_State* L);
	int AI_GetSpellMaxRankForMe(lua_State* L);

	static const struct luaL_Reg AI_BindLib[]{
		{"AddTopGoal", AI_AddTopGoal},
		{"GetData", AI_GetUserTbl},

		{"GetStdThreat", AI_GetStdThreat},
		{"SetStdThreat", AI_SetStdThreat},
		{"GetHealTarget", AI_GetHealTarget},
		{"SetHealTarget", AI_SetHealTarget},

		{"IsFollowing", AI_IsFollowing},

		// equip
		{"EquipCopyFromMaster", AI_EquipCopyFromMaster},
		{"EquipItem", AI_EquipItem},
		{"EquipDestroyAll", AI_EquipDestroyAll},
		{"EquipGetEnchantId", AI_EquipGetEnchantId},
		{"EquipGetRandomProp", AI_EquipGetRandomProp},
		{"EquipPrint", AI_EquipPrint},
		{"UpdateVisibilityForMaster", AI_UpdateVisibilityForMaster},

		// commands
		{"CmdAddProgress", AI_CmdAddProgress},
		{"CmdGetProgress", AI_CmdGetProgress},
		{"CmdSetProgress", AI_CmdSetProgress},
		{"CmdArgs", AI_CmdGetArgs},
		{"CmdState", AI_CmdGetState},
		{"CmdType", AI_CmdGetType},
		{"CmdIsQMode", AI_CmdGetQMode},
		{"CmdComplete", AI_CmdComplete},
		{"CmdIsRequirementMet", AI_CmdIsRequirementMet},
		{"CmdSetInProgress", AI_CmdSetInProgress},
		{"CmdFail", AI_CmdFail},
		{"CmdPrintAll", AI_CmdPrintAll},

		{"GetPartyIntelligence", AI_GetPartyIntelligence},
		{"GetRole", AI_GetRole},
		{"GetSpec", AI_GetSpec},
		{"GetMaster", AI_GetMaster},
		{"GetMasterGuid", AI_GetMasterGuid},
		{"GetPlayer", AI_GetPlayer},

		{"SetRole", AI_SetRole},

		{NULL, NULL}
	};

}

#endif
