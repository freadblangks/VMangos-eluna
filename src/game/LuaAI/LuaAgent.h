
#ifndef MANGOS_LUAAGENT_H
#define MANGOS_LUAAGENT_H

#include "Goal/Goal.h"
#include "Goal/GoalManager.h"
#include "Goal/LogicManager.h"

class PartyIntelligence;

enum class LuaAgentRoles {
	Invalid = 0,
	Mdps = 1,
	Rdps = 2,
	Tank = 3,
	Healer = 4,
	Max = 4
};


class LuaAgent
{
	ShortTimeTracker m_updateTimer;
	uint32 m_updateInterval;

	int m_logic;
	std::string m_spec;
	LuaAgentRoles m_roleId;

	PartyIntelligence* m_party;
	Player* me;
	ObjectGuid m_masterGuid;

	bool m_bCeaseUpdates;
	bool m_bInitialized;

	int m_userDataRef;
	int m_userDataRefPlayer;
	int m_userTblRef;

	Goal m_topGoal;
	GoalManager m_goalManager;
	LogicManager m_logicManager;

public:

	static const char* AI_MTNAME;

	LuaAgent(Player* me, ObjectGuid masterGuid, int logicID);
	~LuaAgent();

	void Init();
	void Update(uint32 diff);
	void UpdateSession(uint32 diff);
	void Reset(bool droprefs);

	void OnPacketReceived(const WorldPacket& pck);

	std::string GetSpec() { return m_spec; }
	void SetSpec(std::string spec) { m_spec = spec; }
	LuaAgentRoles GetRole() { return m_roleId; }
	void SetRole(LuaAgentRoles role) { m_roleId = role; }

	bool IsInitialized() { return m_bInitialized; }
	bool IsReady();

	Goal* AddTopGoal(int goalId, double life, std::vector<GoalParamP>& goalParams, lua_State* L);
	Goal* GetTopGoal() { return &m_topGoal; };

	void SetRef(int v) { m_userDataRef = v; };
	int GetRef() { return m_userDataRef; };
	int GetUserTblRef() { return m_userTblRef; }
	void CreateUserTbl();
	bool GetCeaseUpdates() { return m_bCeaseUpdates; }
	void SetCeaseUpdates(bool value = true) { m_bCeaseUpdates = value; }

	// lua bits
	void CreateUD(lua_State* L);
	void PushUD(lua_State* L);
	void CreatePlayerUD(lua_State* L);
	void PushPlayerUD(lua_State* L);
	void Unref(lua_State* L);
	void UnrefPlayerUD(lua_State* L);
	void UnrefUserTbl(lua_State* L);

	const ObjectGuid& GetMasterGuid() { return m_masterGuid; }
	Player* GetPlayer() { return me; }
	PartyIntelligence* GetPartyIntelligence() { return m_party; }
	void SetPartyIntelligence(PartyIntelligence* party) { m_party = party; }

};

#endif