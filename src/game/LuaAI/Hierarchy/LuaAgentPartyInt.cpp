
#include "LuaAgentPartyInt.h"
#include "LuaAI/LuaAgentUtils.h"
#include "LuaAI/LuaAgentMgr.h"


const char* PartyIntelligence::PI_MTNAME = "Object.PartyInt";


PartyIntelligence::PartyIntelligence(std::string name, ObjectGuid owner) :
	m_name(name),
	m_userDataRef(LUA_NOREF),
	m_bCeaseUpdates(false),
	m_owner(owner),
	m_updateInterval(50)
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


void PartyIntelligence::CreateUD(lua_State* L)
{
	// create userdata on top of the stack pointing to a pointer of a PI object
	PartyIntelligence** piud = static_cast<PartyIntelligence**>(lua_newuserdatauv(L, sizeof(PartyIntelligence*), 0));
	*piud = this; // swap the PI object being pointed to to the current instance
	luaL_setmetatable(L, PI_MTNAME);
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

