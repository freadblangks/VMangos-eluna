
#ifndef MANGOS_LUAAGENT_H
#define MANGOS_LUAAGENT_H

class LuaAgent
{
	ShortTimeTracker m_updateTimer;
	uint32 m_updateInterval;

	int m_logic;
	std::string m_spec;

	Player* me;
	ObjectGuid m_masterGuid;

	bool m_ceaseUpdates;

public:

	LuaAgent(Player* me, ObjectGuid masterGuid, int logicID);

	void Update(uint32 diff);
	void UpdateSession(uint32 diff);

	void OnPacketReceived(const WorldPacket& pck);

	std::string GetSpec() { return m_spec; }
	void SetSpec(std::string spec) { m_spec = spec; }

	const ObjectGuid& GetMasterGuid() { return m_masterGuid; }


};

#endif