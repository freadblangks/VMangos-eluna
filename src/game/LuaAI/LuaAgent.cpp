
#include "LuaAgent.h"
#include "LuaAgentMgr.h"
#include "LuaAgentLibPlayer.h"
#include "lua.hpp"

const char* LuaAgent::AI_MTNAME = "Object.AI";

LuaAgent::LuaAgent(Player* me, ObjectGuid masterGuid, int logicID) :
	me(me),
	m_masterGuid(masterGuid),
	m_logic(logicID),
	m_spec(""),
	m_roleId(LuaAgentRoles::Invalid),

	m_updateInterval(50),
	m_bCeaseUpdates(false),
	m_bInitialized(false),

	m_userDataRef(LUA_NOREF),
	m_userDataRefPlayer(LUA_NOREF),
	m_userTblRef(LUA_NOREF),

	m_logicManager(logicID),
	m_goalManager(),
	m_topGoal(-1, 0, Goal::NOPARAMS, nullptr, nullptr)
{
	m_updateTimer.Reset(2000);
	m_topGoal.SetTerminated(true);
}

LuaAgent::~LuaAgent()
{
	if (lua_State* L = sLuaAgentMgr.Lua())
	{
		Unref(L);
		UnrefPlayerUD(L);
		UnrefUserTbl(L);
	}
}


void LuaAgent::Update(uint32 diff)
{

	if (m_bCeaseUpdates || m_logic == -1)
		return;

	m_updateTimer.Update(diff);
	if (m_updateTimer.Passed())
		m_updateTimer.Reset(m_updateInterval);
	else
		return;

	// not safe to update
	if (!me || me->IsTaxiFlying())
		return;

	if (!IsInitialized())
	{
		Init();
		return;
	}

	if (!IsReady())
		return;

	// Not initialized
	if (m_userDataRef == LUA_NOREF || m_userDataRefPlayer == LUA_NOREF || m_userTblRef == LUA_NOREF)
	{
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "LuaAgent: Attempt to update bot with uninitialized reference... [%d, %d, %d]. Ceasing.\n", m_userDataRef, m_userDataRefPlayer, m_userTblRef);
		SetCeaseUpdates();
		return;
	}

	// Did we corrupt the registry
	if (m_userDataRef == m_userDataRefPlayer || m_userDataRef == m_userTblRef || m_userTblRef == m_userDataRefPlayer)
	{
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "LuaAI Core: Lua registry error... [%d, %d, %d]. Ceasing.\n", m_userDataRef, m_userDataRefPlayer, m_userTblRef);
		SetCeaseUpdates();
		return;
	}

	lua_State* L = sLuaAgentMgr.Lua();
	m_logicManager.Execute(L, this);
	if (!m_topGoal.GetTerminated()) {
		m_goalManager.Activate(L, this);
		m_goalManager.Update(L, this);
		m_goalManager.Terminate(L, this);
	}

	// a manager called error state
	if (GetCeaseUpdates()) {
		m_goalManager = GoalManager();
		Reset(false);
	}
}


void LuaAgent::Reset(bool dropRefs) {

	// clear goals
	m_topGoal = Goal(-1, 0, Goal::NOPARAMS, nullptr, nullptr);
	m_topGoal.SetTerminated(true);

	m_goalManager = GoalManager();
	m_logicManager = LogicManager(m_logic);

	// stop moving
	me->GetMotionMaster()->Clear();
	me->GetMotionMaster()->MoveIdle();

	// unmount
	if (me->IsMounted())
		me->RemoveSpellsCausingAura(AuraType::SPELL_AURA_MOUNTED);

	// stand up
	if (me->GetStandState() != UnitStandStateType::UNIT_STAND_STATE_STAND)
		me->SetStandState(UnitStandStateType::UNIT_STAND_STATE_STAND);

	// unshapeshift
	if (me->HasAuraType(AuraType::SPELL_AURA_MOD_SHAPESHIFT))
		me->RemoveSpellsCausingAura(AuraType::SPELL_AURA_MOD_SHAPESHIFT);

	// reset speed
	me->SetSpeedRate(UnitMoveType::MOVE_RUN, 1.0f);

	// stop attacking
	me->AttackStop();

	if (dropRefs) {
		m_userTblRef = LUA_NOREF;
		m_userDataRef = LUA_NOREF;
		m_userDataRefPlayer = LUA_NOREF;
	}
	else if (lua_State* L = sLuaAgentMgr.Lua()) {
		// delete all refs
		Unref(L);
		UnrefPlayerUD(L);
		UnrefUserTbl(L);
	}
	m_bInitialized = false;

}


void LuaAgent::Init()
{
	lua_State* L = sLuaAgentMgr.Lua();

	if (m_userDataRef == LUA_NOREF)
		CreateUD(L);
	if (m_userDataRefPlayer == LUA_NOREF)
		CreatePlayerUD(L);
	if (m_userTblRef == LUA_NOREF)
		CreateUserTbl();

	m_logicManager.Init(L, this);
	m_bInitialized = true;
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


void LuaAgent::OnPacketReceived(const WorldPacket& pck)
{
	if (!me || pck.empty())
		return;

	switch (pck.GetOpcode())
	{
	case SMSG_GROUP_INVITE:

		WorldPacket pck(pck);
		std::string name;
		pck >> name;

		Player* leader = sObjectAccessor.FindPlayerByNameNotInWorld(name.c_str());
		if (!leader)
			return;

		if (!leader->GetGroup() && sLuaAgentMgr.IsGroupAllInProgress())
			// must assign group in the same frame
			me->GetSession()->HandleGroupAcceptOpcode(WorldPacket(CMSG_GROUP_ACCEPT));
		else
			me->GetSession()->QueuePacket(std::move(std::make_unique<WorldPacket>(CMSG_GROUP_ACCEPT)));

		break;
	}
}


bool LuaAgent::IsReady()
{
	return IsInitialized() && me->IsInWorld() && !me->IsBeingTeleported() && !me->GetSession()->PlayerLoading();
}


Goal* LuaAgent::AddTopGoal(int goalId, double life, std::vector<GoalParamP>& goalParams, lua_State* L)
{
	m_topGoal = Goal(goalId, life, goalParams, &m_goalManager, L);
	m_goalManager = GoalManager(); // top goal owns all of the goals, we could just nuke the entire manager
	m_goalManager.PushGoalOnActivationStack(&m_topGoal);
	return &m_topGoal;
}


// Lua bits


void LuaAgent::CreateUserTbl()
{
	lua_State* L = sLuaAgentMgr.Lua();
	if (m_userTblRef == LUA_NOREF)
	{
		lua_newtable(L);
		m_userTblRef = luaL_ref(L, LUA_REGISTRYINDEX);
	}
}


void LuaAgent::CreateUD(lua_State* L)
{
	// create userdata on top of the stack pointing to a pointer of an AI object
	LuaAgent** aiud = static_cast<LuaAgent**>(lua_newuserdatauv(L, sizeof(LuaAgent*), 0));
	*aiud = this; // swap the AI object being pointed to to the current instance
	luaL_setmetatable(L, AI_MTNAME);
	// save this userdata in the registry table.
	m_userDataRef = luaL_ref(L, LUA_REGISTRYINDEX); // pops
}


void LuaAgent::PushUD(lua_State* L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, m_userDataRef);
}


void LuaAgent::Unref(lua_State* L)
{
	if (m_userDataRef != LUA_NOREF && m_userDataRef != LUA_REFNIL)
	{
		luaL_unref(L, LUA_REGISTRYINDEX, m_userDataRef);
		m_userDataRef = LUA_NOREF;
	}
}


void LuaAgent::CreatePlayerUD(lua_State* L)
{
	LuaBindsAI::Player_CreateUD(me, L);
	// save this userdata in the registry table.
	m_userDataRefPlayer = luaL_ref(L, LUA_REGISTRYINDEX); // pops
}


void LuaAgent::PushPlayerUD(lua_State* L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, m_userDataRefPlayer);
}


void LuaAgent::UnrefPlayerUD(lua_State* L)
{
	if (m_userDataRefPlayer != LUA_NOREF && m_userDataRefPlayer != LUA_REFNIL)
	{
		luaL_unref(L, LUA_REGISTRYINDEX, m_userDataRefPlayer);
		m_userDataRefPlayer = LUA_NOREF;
	}
}

void LuaAgent::UnrefUserTbl(lua_State* L)
{
	if (m_userTblRef != LUA_NOREF && m_userTblRef != LUA_REFNIL)
	{
		luaL_unref(L, LUA_REGISTRYINDEX, m_userTblRef);
		m_userTblRef = LUA_NOREF;
	}
}

