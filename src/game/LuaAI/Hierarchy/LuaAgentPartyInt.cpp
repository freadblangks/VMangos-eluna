
#include "LuaAgentPartyInt.h"
#include "LuaAI/LuaAgentUtils.h"
#include "LuaAI/LuaAgentMgr.h"
#include "LuaAI/LuaAgent.h"
#include "LuaAI/LuaAgentLibAI.h"
#include "LuaAI/LuaAgentLibWorldObj.h"
#include "LuaAI/LuaAgentLibUnit.h"
#include "LuaAI/Libs/CLine.h"


const char* PartyIntelligence::PI_MTNAME = "Object.PartyInt";


namespace
{
	void AddAttackersFrom(lua_State* L, Unit* unit, lua_Integer& idx)
	{
		for (const auto pAttacker : unit->GetAttackers())
		{
			//printf("Additing target to tbl %s attacking %s\n", pAttacker->GetName(), pMember->GetName());
			LuaBindsAI::Unit_CreateUD(pAttacker, L); // pushes pAttacker userdata on top of stack
			lua_seti(L, -2, idx); // stack[-2][tblIdx] = stack[-1], pops pAttacker
			++idx;
		}
	}
}


PartyIntelligence::PartyIntelligence(std::string name, ObjectGuid owner) :
	m_name(name),
	m_userDataRef(LUA_NOREF),
	m_bCeaseUpdates(false),
	m_owner(owner),
	m_updateInterval(50),
	m_dungeon(nullptr)
{
	m_init = m_name + "_Init";
	m_update = m_name + "_Update";
	m_updateTimer.Reset(m_updateInterval);
}


PartyIntelligence::~PartyIntelligence() noexcept
{
	if (lua_State* L = sLuaAgentMgr.Lua())
		Unref(L);
}


void PartyIntelligence::Init(lua_State* L)
{
	CreateUD(L);
	lua_getglobal(L, m_init.c_str());
	PushUD(L);
	if (lua_dopcall(L, 1, 0) != LUA_OK)
	{
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Error party init: %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		SetCeaseUpdates(true);
		return;
	}
}


void PartyIntelligence::Reset(lua_State* L, bool dropRefs)
{
	SetCeaseUpdates(false);
	if (!L || dropRefs)
		m_userDataRef = LUA_NOREF;
	else
		Unref(L);
	m_agentInfos.clear();
	m_agents.clear();
}


void PartyIntelligence::LoadInfoFromLuaTbl(lua_State* L)
{
	if (!m_agentInfos.empty())
		luaL_error(L, "PartyIntelligence::LoadInfoFromLuaTbl attempt to reinitialize info");
	if (!lua_istable(L, -1))
		luaL_error(L, "PartyIntelligence::LoadInfoFromLuaTbl expected table at the top of the stack");

	lua_Integer length = luaL_len(L, -1);
	for (lua_Integer i = 1; i <= length; ++i)
	{
		if (lua_geti(L, -1, i) == LUA_TTABLE)
		{
			lua_geti(L, -1, 1);
			lua_geti(L, -2, 2);
			lua_geti(L, -3, 3);
			std::string name = luaL_checkstring(L, -3);
			lua_Integer logic = luaL_checkinteger(L, -2);
			std::string spec = luaL_checkstring(L, -1);
			lua_pop(L, 3);
			m_agentInfos.emplace_back(name, logic, spec);
		}
		else
			luaL_error(L, "PartyIntelligence::LoadInfoFromLuaTbl value at idx %lld is not a table for party %s", i, m_name.c_str());
		lua_pop(L, 1); // pop table
	}
}


void PartyIntelligence::Update(uint32 diff, lua_State* L)
{
	if (m_userDataRef == LUA_NOREF || m_bCeaseUpdates)
		return;

	m_updateTimer.Update(diff);
	if (m_updateTimer.Passed())
		m_updateTimer.Reset(m_updateInterval);
	else
		return;

	// process login
	if (m_agents.size() != m_agentInfos.size())
	{
		if (m_agents.size() > m_agentInfos.size())
		{
			sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "PartyIntelligence::Update m_agents = %d m_agentInfos = %d", m_agents.size(), m_agentInfos.size());
			SetCeaseUpdates(true);
			return;
		}
		LoadAgents();
		return;
	}

	lua_getglobal(L, m_update.c_str());
	PushUD(L);
	if (lua_dopcall(L, 1, 0) != LUA_OK)
	{
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Error party update: %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		SetCeaseUpdates(true);
		return;
	}

	if (Player* owner = sObjectAccessor.FindPlayer(m_owner))
		if (!m_dungeon || m_dungeon->mapId != owner->GetMapId())
			if (DungeonData* data = sLuaAgentMgr.GetDungeonData(owner->GetMapId()))
				m_dungeon = data;
			else
				m_dungeon = nullptr;
}


Player* PartyIntelligence::GetAgent(const ObjectGuid& guid)
{
	LuaAgentMap::const_iterator it = m_agents.find(guid);
	return (it == m_agents.end()) ? nullptr : it->second;
}


void PartyIntelligence::LoadAgents()
{
	for (auto& info : m_agentInfos)
	{
		ObjectGuid& agentGuid = sObjectMgr.GetPlayerGuidByName(info.name);
		if (agentGuid.IsEmpty())
		{
			sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "PartyIntelligence::LoadAgents player with name %s doesn't exist", info.name.c_str());
			SetCeaseUpdates(true);
			return;
		}
		if (GetAgent(agentGuid))
			continue;
		LuaAgentMgr::CheckResult result = sLuaAgentMgr.AddAgent(info.name, m_owner, info.logicId, info.spec);
		switch (result)
		{
		case LuaAgentMgr::CHAR_OK:
			break;
		case LuaAgentMgr::CHAR_ALREADY_EXISTS:
		{
			// if is already a bot add as our own
			Player* agent = sLuaAgentMgr.GetAgent(agentGuid);
			LuaAgent* agentAI = agent->GetLuaAI();
			if (!agentAI || agentAI->GetMasterGuid() != m_owner)
			{
				sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "PartyIntelligence::LoadAgents player with name %s is already owned", info.name.c_str());
				SetCeaseUpdates(true);
				return;
			}
			m_agents[agentGuid] = agent;
			agentAI->SetPartyIntelligence(this);
			break;
		}
		case LuaAgentMgr::CHAR_ALREADY_IN_QUEUE:
		{
			// if already queued as bot if owners match we can carry on
			const LuaAgentInfoHolder* loginInfo = sLuaAgentMgr.GetLoginInfo(agentGuid);
			if (loginInfo && loginInfo->masterGuid == m_owner)
				break;
		}
		default:
		{
			sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "PartyIntelligence::LoadAgents player with name %s can not be added. Reason = %d", info.name.c_str(), result);
			SetCeaseUpdates(true);
			return;
		}
		}
	}
}


float PartyIntelligence::GetAngleForTank(LuaAgent* ai, Unit* target, bool allowFlip)
{
	Player* agent = ai->GetPlayer();
	if (!HasCLineFor(agent))
		return agent->GetOrientation();

	int resultS;
	float resultD;
	G3D::Vector3 result;
	G3D::Vector3 from(agent->GetPositionX(), agent->GetPositionY(), agent->GetPositionZ());

	CLine& line = m_dungeon->lines[m_dungeon->ClosestP(from, result, resultD, resultS)];

	G3D::Vector3& AB = line.pts[resultS + 1].pos - line.pts[resultS].pos;

	float a = std::atan2(AB.y, AB.x);
	a = (a >= 0.f) ? a : (2.f * M_PI_F + a); // 0 .. 360

	if (allowFlip && !m_owner.IsEmpty())
		if (Player* owner = sObjectAccessor.FindPlayer(m_owner))
		{
			G3D::Vector3 ownerPos(owner->GetPositionX(), owner->GetPositionY(), owner->GetPositionZ());
			G3D::Vector3 targetPos(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
			G3D::Vector3& ownerToTarget = targetPos - ownerPos;
			// flip direction if owner is opposite side
			if (AB.dot(ownerToTarget) < 0.f)
				a = (a < M_PI_F) ? (a + M_PI_F) : (a - M_PI_F);
		}

	return a;
}


void PartyIntelligence::CreateUD(lua_State* L)
{
	// create userdata on top of the stack pointing to a pointer of a PI object
	PartyIntelligence** piud = static_cast<PartyIntelligence**>(lua_newuserdatauv(L, sizeof(PartyIntelligence*), 1));
	*piud = this; // swap the PI object being pointed to to the current instance
	luaL_setmetatable(L, PI_MTNAME);
	lua_newtable(L);
	lua_setiuservalue(L, -2, 1);
	// save this userdata in the registry table.
	m_userDataRef = luaL_ref(L, LUA_REGISTRYINDEX); // pops
}


void PartyIntelligence::PushUD(lua_State* L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, m_userDataRef);
}


void PartyIntelligence::Unref(lua_State* L)
{
	if (m_userDataRef != LUA_NOREF && m_userDataRef != LUA_REFNIL)
	{
		luaL_unref(L, LUA_REGISTRYINDEX, m_userDataRef);
		m_userDataRef = LUA_NOREF;
	}
}


bool PartyIntelligence::HasCLineFor(Unit* agent)
{
	return m_dungeon ? m_dungeon->mapId == agent->GetMapId() : false;
}


// ---------------------------------------------------------
//    LUA BINDS
// ---------------------------------------------------------


PartyIntelligence* LuaBindsAI::PartyInt_GetPIObject(lua_State* L)
{
	return *((PartyIntelligence**) luaL_checkudata(L, 1, PartyIntelligence::PI_MTNAME));
}


void LuaBindsAI::BindPartyIntelligence(lua_State* L)
{
	PartyInt_CreateMetatable(L);
}


void LuaBindsAI::PartyInt_CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, PartyIntelligence::PI_MTNAME);
	lua_pushvalue(L, -1); // copy mt cos setfield pops
	lua_setfield(L, -1, "__index"); // mt.__index = mt
	luaL_setfuncs(L, PartyInt_BindLib, 0); // copy funcs
	lua_pop(L, 1); // pop mt
}


int LuaBindsAI::PartyInt_CanPullTarget(lua_State* L)
{
	PartyIntelligence* intelligence = PartyInt_GetPIObject(L);
	Unit* target = Unit_GetUnitObject(L, 2);
	if (DungeonData* data = intelligence->GetDungeonData())
		if (auto encounter = data->GetEncounter(target->GetName()))
		{
			lua_pushboolean(L, false);
			return 1;
		}
	lua_pushboolean(L, true);
	return 1;
}


int LuaBindsAI::PartyInt_CmdEngage(lua_State* L)
{
	LuaAgent* ai = AI_GetAIObject(L, 2);
	//LuaObjectGuid* guid = Guid_GetGuidObject(L, 3);
	lua_Number angle = luaL_checknumber(L, 3);
	AgentCmdEngage* cmd = new AgentCmdEngage(angle);
	lua_pushinteger(L, ai->CommandsAdd(cmd));
	return 1;
}


int LuaBindsAI::PartyInt_CmdFollow(lua_State* L)
{
	LuaAgent* ai = AI_GetAIObject(L, 2);
	LuaObjectGuid* guid = Guid_GetGuidObject(L, 3);
	lua_Number dist = luaL_checknumber(L, 4);
	lua_Number angle = luaL_checknumber(L, 5);
	AgentCmdFollow* cmd = new AgentCmdFollow(guid->guid, dist, angle);
	lua_pushinteger(L, ai->CommandsAdd(cmd));
	return 1;
}


int LuaBindsAI::PartyInt_CmdHeal(lua_State* L)
{
	LuaAgent* ai = AI_GetAIObject(L, 2);
	LuaObjectGuid* guid = Guid_GetGuidObject(L, 3);
	lua_Integer numHeals = luaL_checkinteger(L, 4);
	AgentCmdHeal* cmd = new AgentCmdHeal(guid->guid, numHeals);
	lua_pushinteger(L, ai->CommandsAdd(cmd));
	return 1;
}


int LuaBindsAI::PartyInt_CmdTank(lua_State* L)
{
	LuaAgent* ai = AI_GetAIObject(L, 2);
	LuaObjectGuid* guid = Guid_GetGuidObject(L, 3);
	lua_Number desiredThreat = luaL_checknumber(L, 4);
	AgentCmdTank* cmd = new AgentCmdTank(guid->guid, desiredThreat);
	lua_pushinteger(L, ai->CommandsAdd(cmd));
	return 1;
}


int LuaBindsAI::PartyInt_CmdPull(lua_State* L)
{
	LuaAgent* ai = AI_GetAIObject(L, 2);
	LuaObjectGuid* guid = Guid_GetGuidObject(L, 3);
	AgentCmdPull* cmd = new AgentCmdPull(guid->guid);
	lua_pushinteger(L, ai->CommandsAdd(cmd));
	return 1;
}


int LuaBindsAI::PartyInt_GetOwnerGuid(lua_State* L)
{
	PartyIntelligence* intelligence = PartyInt_GetPIObject(L);
	Guid_CreateUD(L, intelligence->GetOwnerGuid());
	return 1;
}


int LuaBindsAI::PartyInt_GetData(lua_State* L)
{
	PartyIntelligence* intelligence = PartyInt_GetPIObject(L);
	lua_getiuservalue(L, -1, 1);
	return 1;
}


int LuaBindsAI::PartyInt_LoadInfoFromLuaTbl(lua_State* L)
{
	PartyIntelligence* intelligence = PartyInt_GetPIObject(L);
	intelligence->LoadInfoFromLuaTbl(L);
	return 0;
}


int LuaBindsAI::PartyInt_GetAgents(lua_State* L)
{
	PartyIntelligence* intelligence = PartyInt_GetPIObject(L);
	lua_newtable(L);
	lua_Integer idx = 1;
	for (auto& it : intelligence->GetAgentMap())
	{
		Player* agent = it.second;
		if (LuaAgent* agentAI = agent->GetLuaAI())
		{
			agentAI->PushUD(L);
			lua_seti(L, -2, idx);
			++idx;
		}
	}
	return 1;
}


int LuaBindsAI::PartyInt_GetAttackers(lua_State* L)
{
	PartyIntelligence* intelligence = PartyInt_GetPIObject(L);
	lua_newtable(L);
	lua_Integer idx = 1;
	for (auto& it : intelligence->GetAgentMap())
		AddAttackersFrom(L, it.second->ToUnit(), idx);

	if (Player* owner = sObjectAccessor.FindPlayer(intelligence->GetOwnerGuid()))
		AddAttackersFrom(L, owner->ToUnit(), idx);

	return 1;
}


int LuaBindsAI::PartyInt_GetAngleForTank(lua_State* L)
{
	PartyIntelligence* intelligence = PartyInt_GetPIObject(L);
	LuaAgent* ai = AI_GetAIObject(L, 2);
	Unit* target = Unit_GetUnitObject(L, 3);
	bool allowFlip = luaL_checkboolean(L, 4);
	lua_pushnumber(L, intelligence->GetAngleForTank(ai, target, allowFlip));
	return 1;
}


int LuaBindsAI::PartyInt_HasCLineFor(lua_State* L)
{
	PartyIntelligence* intelligence = PartyInt_GetPIObject(L);
	Unit* unit = Unit_GetUnitObject(L, 2);
	lua_pushboolean(L, intelligence->HasCLineFor(unit));
	return 1;
}


int LuaBindsAI::PartyInt_GetNearestCLineP(lua_State* L)
{
	PartyIntelligence* intelligence = PartyInt_GetPIObject(L);
	Unit* unit = Unit_GetUnitObject(L, 2);
	DungeonData* cline = intelligence->GetDungeonData();
	if (!cline)
		luaL_error(L, "PartyInt_GetNearestCLineP: cline doesn't exist");
	if (cline->mapId != unit->GetMapId())
		luaL_error(L, "PartyInt_GetNearestCLineP: cline map mismatch");
	G3D::Vector3 P;
	float D;
	int S;
	int line = cline->ClosestP(G3D::Vector3(unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ()), P, D, S);
	lua_pushnumber(L, P.x);
	lua_pushnumber(L, P.y);
	lua_pushnumber(L, P.z);
	lua_pushnumber(L, D);
	lua_pushinteger(L, S);
	lua_pushinteger(L, line);
	return 6;
}


int LuaBindsAI::PartyInt_GetPrevCLineS(lua_State* L)
{
	PartyIntelligence* intelligence = PartyInt_GetPIObject(L);
	Unit* unit = Unit_GetUnitObject(L, 2);
	lua_Integer lineIdx = luaL_checkinteger(L, 3);
	lua_Integer S = luaL_checkinteger(L, 4);
	DungeonData* cline = intelligence->GetDungeonData();
	if (!cline)
		luaL_error(L, "PartyInt_GetPrevCLineS: cline doesn't exist");
	if (cline->mapId != unit->GetMapId())
		luaL_error(L, "PartyInt_GetPrevCLineS: cline map mismatch");
	if (cline->lines.size() <= lineIdx || lineIdx < 0)
		luaL_error(L, "PartyInt_GetPrevCLineS: cline only has %d lines, got %d", cline->lines.size(), lineIdx);
	if (cline->lines[lineIdx].pts.size() <= S + 1 || S < 0)
		luaL_error(L, "PartyInt_GetPrevCLineS: cline %d only has %d pts, got %d", lineIdx, cline->lines[lineIdx].pts.size(), S);
	G3D::Vector3* P = &cline->lines[lineIdx].pts[0].pos;
	if (S == 0 && lineIdx == 0);
	else
	{
		CLine& line = cline->lines[lineIdx];
		//if (S == 0)
		//{
		//	--lineIdx;
		//	line = cline->lines[lineIdx];
		//	S = line.pts.size() - 1;
		//	P = &line.pts.back().pos;
		//}
		//else
		{
			if (S > 0) --S;
			P = &line.pts[S].pos;
		}
	}
	lua_pushnumber(L, P->x);
	lua_pushnumber(L, P->y);
	lua_pushnumber(L, P->z);
	lua_pushinteger(L, S);
	lua_pushinteger(L, lineIdx);
	return 5;
}
