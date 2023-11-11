
#ifndef MANGOS_LUAAGENTPARTYINT_H
#define MANGOS_LUAAGENTPARTYINT_H

#include "lua.hpp"

class LuaAgent;

class PartyIntelligence
{

public:
	static const char* PI_MTNAME;
	struct AgentInfo
	{
		AgentInfo(std::string name, int logicId, std::string spec) : name(name), logicId(logicId), spec(spec) {}
		int logicId;
		std::string name;
		std::string spec;
	};

	PartyIntelligence(std::string name, ObjectGuid owner);
	~PartyIntelligence();

	void LoadInfoFromLuaTbl(lua_State* L);
	void LoadAgents();
	void Reset(lua_State* L, bool dropRefs);
	void Init(lua_State* L);
	void Update(uint32 diff, lua_State* L);

	void CreateUD(lua_State* L);
	void PushUD(lua_State* L);
	void Unref(lua_State* L);

	void SetCeaseUpdates(bool v) { m_bCeaseUpdates = v; }
	bool GetCeaseUpdates() { return m_bCeaseUpdates; }
	std::string GetName() { return m_name; }
	ObjectGuid GetOwnerGuid() { return m_owner; }

private:
	bool m_bCeaseUpdates;
	int m_userDataRef;
	ObjectGuid m_owner;

	std::vector<AgentInfo> m_agentInfos;
	std::vector<LuaAgent*> m_agents;

	std::string m_name;
	std::string m_init;
	std::string m_update;

};


namespace LuaBindsAI {
	void BindPartyIntelligence(lua_State* L);
	PartyIntelligence* PartyInt_GetPIObject(lua_State* L);
	void PartyInt_CreateMetatable(lua_State* L);
	int PartyInt_LoadInfoFromLuaTbl(lua_State* L);

	static const struct luaL_Reg PartyInt_BindLib[]{
		{"LoadInfoFromLuaTbl", PartyInt_LoadInfoFromLuaTbl},

		{NULL, NULL}
	};

}

#endif