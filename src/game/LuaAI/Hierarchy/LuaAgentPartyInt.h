
#ifndef MANGOS_LUAAGENTPARTYINT_H
#define MANGOS_LUAAGENTPARTYINT_H

#include "lua.hpp"

typedef std::unordered_map<ObjectGuid, Player*> LuaAgentMap;
class LuaAgent;
class Unit;
struct DungeonData;

class PartyIntelligence
{

public:
	static const char* PI_MTNAME;
	struct CCInfo
	{
		const ObjectGuid& guid;
		const ObjectGuid& agentGuid;
		CCInfo(const ObjectGuid& guid, const ObjectGuid& agentGuid) : guid(guid), agentGuid(agentGuid) {}
		bool Check(PartyIntelligence* intelligence);
	};
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

	float GetAngleForTank(LuaAgent* tank, Unit* target, bool& flipped, bool allowFlip, bool forceFlip = false);
	DungeonData* GetDungeonData() { return m_dungeon; }
	bool HasCLineFor(Unit* agent);

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

	void AddCC(const ObjectGuid& guid, const ObjectGuid& agentGuid) { m_cc.emplace(guid, CCInfo(guid, agentGuid)); }
	bool IsCC(const ObjectGuid& guid) { return m_cc.find(guid) != m_cc.end(); }
	std::unordered_map<ObjectGuid, CCInfo>& GetCCMap() { return m_cc; }
	void RemoveCC(const ObjectGuid& guid) { m_cc.erase(guid); }
	void UpdateCC();

private:
	bool m_bCeaseUpdates;
	int m_userDataRef;
	ObjectGuid m_owner;

	int m_updateInterval;
	ShortTimeTracker m_updateTimer;

	DungeonData* m_dungeon;

	std::vector<AgentInfo> m_agentInfos;
	LuaAgentMap m_agents;

	std::string m_name;
	std::string m_init;
	std::string m_update;

	std::unordered_map<ObjectGuid, CCInfo> m_cc;
};


namespace LuaBindsAI {
	void BindPartyIntelligence(lua_State* L);
	PartyIntelligence* PartyInt_GetPIObject(lua_State* L);
	void PartyInt_CreateMetatable(lua_State* L);

	int PartyInt_CanPullTarget(lua_State* L);

	int PartyInt_CmdCC(lua_State* L);
	int PartyInt_CmdEngage(lua_State* L);
	int PartyInt_CmdFollow(lua_State* L);
	int PartyInt_CmdHeal(lua_State* L);
	int PartyInt_CmdTank(lua_State* L);
	int PartyInt_CmdPull(lua_State* L);

	int PartyInt_GetData(lua_State* L);
	int PartyInt_GetOwnerGuid(lua_State* L);

	// cline

	int PartyInt_HasCLineFor(lua_State* L);
	int PartyInt_GetAngleForTank(lua_State* L);
	int PartyInt_GetNearestCLineP(lua_State* L);
	int PartyInt_GetPrevCLineS(lua_State* L);

	// cc

	int PartyInt_AddCC(lua_State* L);
	int PartyInt_RemoveCC(lua_State* L);

	int PartyInt_LoadInfoFromLuaTbl(lua_State* L);

	int PartyInt_GetAgents(lua_State* L);
	int PartyInt_GetAttackers(lua_State* L);
	int PartyInt_GetCCTable(lua_State* L);

	static const struct luaL_Reg PartyInt_BindLib[]{
		{"CanPullTarget", PartyInt_CanPullTarget},

		{"CmdCC", PartyInt_CmdCC},
		{"CmdEngage", PartyInt_CmdEngage},
		{"CmdFollow", PartyInt_CmdFollow},
		{"CmdHeal", PartyInt_CmdHeal},
		{"CmdTank", PartyInt_CmdTank},
		{"CmdPull", PartyInt_CmdPull},

		{"GetData", PartyInt_GetData},
		{"GetOwnerGuid", PartyInt_GetOwnerGuid},

		// cline
		{"HasCLineFor", PartyInt_HasCLineFor},
		{"GetAngleForTank", PartyInt_GetAngleForTank},
		{"GetNearestCLineP", PartyInt_GetNearestCLineP},
		{"GetPrevCLineS", PartyInt_GetPrevCLineS},

		// cc
		{"AddCC", PartyInt_AddCC},
		{"RemoveCC", PartyInt_RemoveCC},

		{"LoadInfoFromLuaTbl", PartyInt_LoadInfoFromLuaTbl},

		{"GetAgents", PartyInt_GetAgents},
		{"GetAttackers", PartyInt_GetAttackers},
		{"GetCC", PartyInt_GetCCTable},

		{NULL, NULL}
	};

}

#endif
