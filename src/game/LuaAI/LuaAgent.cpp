
#include "LuaAgent.h"

LuaAgent::LuaAgent(Player* me, ObjectGuid masterGuid, int logicID) :
	me(me),
	m_masterGuid(masterGuid),
	m_logic(logicID),

	m_updateInterval(50),
	m_ceaseUpdates(false)
{
	m_updateTimer.Reset(2000);
}


void LuaAgent::Update(uint32 diff)
{

	if (m_ceaseUpdates || m_logic == -1)
		return;

	m_updateTimer.Update(diff);
	if (m_updateTimer.Passed())
		m_updateTimer.Reset(m_updateInterval);
	else
		return;

	// master?
	Player* master = ObjectAccessor::FindPlayer(m_masterGuid);

	// bad pointers
	if (!me || !master) {
		return;
	}

	if (!me->IsInWorld() || me->IsBeingTeleported())
		return;

	if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE)
	{
		me->GetMotionMaster()->Clear();
		me->GetMotionMaster()->MoveFollow(master, 3.f, 3.14f);
	}

}


void LuaAgent::UpdateSession(uint32 diff)
{
	WorldSession* session = me->GetSession();
	for (int i = 0; i < PACKET_PROCESS_MAX_TYPE; i++)
	{
		WorldSessionFilter filter(session);
		filter.SetProcessType(PacketProcessing(i));
		session->Update(filter);
	}
}



