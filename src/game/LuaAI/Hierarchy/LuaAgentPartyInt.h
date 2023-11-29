
#ifndef MANGOS_LUAAGENTPARTYINT_H
#define MANGOS_LUAAGENTPARTYINT_H

#include "lua.hpp"

typedef std::unordered_map<ObjectGuid, Player*> LuaAgentMap;
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
	~PartyIntelligence() noexcept;

	Player* GetAgent(const ObjectGuid& guid);
	LuaAgentMap& GetAgentMap() { return m_agents; }

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

	int m_updateInterval;
	ShortTimeTracker m_updateTimer;

	std::vector<AgentInfo> m_agentInfos;
	LuaAgentMap m_agents;

	std::string m_name;
	std::string m_init;
	std::string m_update;

};


namespace LuaBindsAI {
	void BindPartyIntelligence(lua_State* L);
	PartyIntelligence* PartyInt_GetPIObject(lua_State* L);
	void PartyInt_CreateMetatable(lua_State* L);

	int PartyInt_CmdEngage(lua_State* L);
	int PartyInt_CmdFollow(lua_State* L);
	int PartyInt_CmdTank(lua_State* L);

	int PartyInt_GetData(lua_State* L);
	int PartyInt_GetOwnerGuid(lua_State* L);

	int PartyInt_LoadInfoFromLuaTbl(lua_State* L);

	int PartyInt_GetAgents(lua_State* L);
	int PartyInt_GetAttackers(lua_State* L);

	static const struct luaL_Reg PartyInt_BindLib[]{
		{"CmdEngage", PartyInt_CmdEngage},
		{"CmdFollow", PartyInt_CmdFollow},
		{"CmdTank", PartyInt_CmdTank},

		{"GetData", PartyInt_GetData},
		{"GetOwnerGuid", PartyInt_GetOwnerGuid},

		{"LoadInfoFromLuaTbl", PartyInt_LoadInfoFromLuaTbl},

		{"GetAgents", PartyInt_GetAgents},
		{"GetAttackers", PartyInt_GetAttackers},

		{NULL, NULL}
	};

}

#endif
