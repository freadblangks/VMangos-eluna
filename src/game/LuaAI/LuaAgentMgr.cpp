
#include "Hierarchy/LuaAgentPartyInt.h"
#include "LuaAgent.h"
#include "LuaAgentMgr.h"
#include "AccountMgr.h"
#include "Database/DatabaseImpl.h"
#include "Group.h"
#include "lua.hpp"
#include "Goal/GoalManager.h"
#include "Goal/LogicManager.h"
#include "LuaAgentBindsCommon.h"
#include "LuaAgentUtils.h"
#include <experimental/filesystem>
#include <fstream>
#include "Libs/CLine.h"

class LuaAgentLoginQueryHolder : public LoginQueryHolder
{
public:
	int logicID;
	std::string spec;
	ObjectGuid masterGuid;
	LuaAgentLoginQueryHolder(uint32 accountId, ObjectGuid guid, ObjectGuid masterGuid, int logicID, std::string spec)
		: LoginQueryHolder(accountId, guid), masterGuid(masterGuid), logicID(logicID), spec(spec) { }
};


class LuaAgentCharacterHandler
{
public:
	void HandleLoginCallback(QueryResult* /*dummy*/, SqlQueryHolder* holder)
	{
		if (!holder)
			return;

		LuaAgentLoginQueryHolder* lqh = dynamic_cast<LuaAgentLoginQueryHolder*>(holder);
		if (!lqh) {
			sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "LuaAgent - login callback failed to cast SqlQueryHolder");
			delete holder;
			return;
		}

		// copy holder values
		ObjectGuid masterGuid = lqh->masterGuid;
		ObjectGuid myGuid = lqh->GetGuid();
		int logicID = lqh->logicID;
		std::string spec = lqh->spec;

		// already logged in or master's gone
		if (!ObjectAccessor::FindPlayerNotInWorld(lqh->masterGuid) || sObjectMgr.GetPlayer(lqh->GetGuid()))
		{
			delete holder;
			return;
		}

		// Deleted in LuaAgentMgr::__RemoveAgents
		WorldSession* session = new WorldSession(lqh->GetAccountId(), nullptr, sAccountMgr.GetSecurity(lqh->GetAccountId()), 0, LOCALE_enUS);
		session->SetToLoading();
		session->HandlePlayerLogin(lqh); // will delete lqh
		// added?
		if (Player* pPlayer = sObjectAccessor.FindPlayerNotInWorld(myGuid))
			sLuaAgentMgr.OnAgentLogin(session, myGuid, masterGuid, logicID, spec);
		else
		{
			sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Failed to add bot %s", myGuid.GetString());
			delete session;
			sLuaAgentMgr.EraseLoginInfo(myGuid);
		}

	}
} luaChrHandler;


LuaAgentMgr::LuaAgentMgr() : m_bLuaCeaseUpdates(false), m_bLuaReload(false), L(nullptr), m_bGroupAllInProgress(false)
{
	LuaLoadAll();
}


LuaAgentMgr::~LuaAgentMgr() noexcept
{
	if (L) lua_close(L);
}


// ********************************************************
// **                  Lua basics                        **
// ********************************************************


bool LuaAgentMgr::LuaDofile(const std::string& filename) {
	if ((luaL_loadfile(L, filename.c_str()) || lua_dopcall(L, 0, LUA_MULTRET)) != LUA_OK) {
		m_bLuaCeaseUpdates = true;
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "LuaAgentMgr: Lua error executing file %s: %s\n", filename.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1); // pop the error object
		return false;
	}
	return true;
}


void LuaAgentMgr::LuaLoadFiles(const std::string& fpath) {
	if (!L) return;

	// logic and goal list must be loaded before all other files.
	if (!LuaDofile((fpath + "/logic_list.lua")))
		return;
	if (!LuaDofile((fpath + "/goal_list.lua")))
		return;
	if (!LuaDofile((fpath + "/ai_define.lua")))
		return;

	// do all files recursively
	for (const auto& entry : std::experimental::filesystem::recursive_directory_iterator(fpath))
		if (entry.path().extension().generic_string() == ".lua")
			if (!LuaDofile(entry.path().generic_string()))
				return;

	for (const auto& entry : std::experimental::filesystem::recursive_directory_iterator(fpath + "/data"))
		if (entry.path().extension().generic_string() == ".txt")
			CLineLoad(entry.path().generic_string());

}


void LuaAgentMgr::LuaLoadAll() {
	std::string fpath = "ai";
	if (L) lua_close(L); // kill old state
	L = nullptr;

	GoalManager::ClearRegistered();
	LogicManager::ClearRegistered();

	L = luaL_newstate();
	luaL_openlibs(L); // replace with individual libraries later

	LuaBindsAI::BindAll(L);

	LuaLoadFiles(fpath);
}


// ********************************************************
// **                  Bot Management                    **
// ********************************************************


void LuaAgentMgr::Update(uint32 diff)
{

	// reload requested
	if (m_bLuaReload)
	{
		sWorld.SendServerMessage(SERVER_MSG_CUSTOM, "Lua reload started");

		LuaLoadAll();

		for (auto& it : m_agents)
		{
			// reset all
			if (LuaAgent* agent = it.second->GetLuaAI())
			{
				agent->Reset(true);
				agent->SetCeaseUpdates(false);
			}
		}

		for (auto& party : m_parties)
		{
			party->Reset(L, true);
			party->Init(L);
		}

		m_bLuaReload = false;
		m_bLuaCeaseUpdates = false;

		sWorld.SendServerMessage(SERVER_MSG_CUSTOM, "Lua reload finished");
	}

	// lua error on initialization
	if (m_bLuaCeaseUpdates)
		return;

	for (auto& party : m_parties)
		party->Update(diff, L);

	for (auto& it : m_agents)
	{
		LuaAgent* agent = it.second->GetLuaAI();
		if (!agent)
		{
			LogoutAgent(it.first);
			continue;
		}

		agent->Update(diff);
		agent->UpdateSession(diff);
	}

	__RemoveAgents(); // process logout queue
	__AddAgents(); // process login queue

}


Player* LuaAgentMgr::GetAgent(ObjectGuid guid)
{
	LuaAgentMap::const_iterator it = m_agents.find(guid);
	return (it == m_agents.end()) ? nullptr : it->second;
}


const LuaAgentInfoHolder* LuaAgentMgr::GetLoginInfo(ObjectGuid guid)
{
	const std::lock_guard<std::mutex> lock(loginMutex);
	auto it = m_toAdd.find(guid);
	return (it == m_toAdd.end()) ? nullptr : &(it->second);
}


void LuaAgentMgr::EraseLoginInfo(ObjectGuid guid)
{
	const std::lock_guard<std::mutex> lock(loginMutex);
	auto it = m_toAdd.find(guid);
	if (it != m_toAdd.end())
		it->second.status = LuaAgentInfoHolder::TODELETE;
}


void LuaAgentMgr::SetLoggedIn(ObjectGuid guid)
{
	const std::lock_guard<std::mutex> lock(loginMutex);
	auto it = m_toAdd.find(guid);
	if (it != m_toAdd.end())
		it->second.status = LuaAgentInfoHolder::LOGGEDIN;
}


void LuaAgentMgr::AddParty(std::string name, ObjectGuid owner)
{
	for (auto& party : m_parties)
		if (party->GetName() == name && party->GetOwnerGuid() == owner)
			return;

	std::unique_ptr<PartyIntelligence> party = std::make_unique<PartyIntelligence>(name, owner);
	party->Init(L);
	m_parties.push_back(std::move(party));
}


LuaAgentMgr::CheckResult LuaAgentMgr::CheckAgentValid(std::string charName, ObjectGuid masterGuid)
{
	ObjectGuid guid = sObjectMgr.GetPlayerGuidByName(charName);
	// character with name doesn't exist
	if (guid.IsEmpty())
		return CHAR_DOESNT_EXIST;
	// bot's account doesn't exist
	if (sObjectMgr.GetPlayerAccountIdByGUID(guid) == 0)
		return CHAR_ACCOUNT_DOESNT_EXIST;
	// already a bot
	if (GetAgent(guid))
		return CHAR_ALREADY_EXISTS;
	// already queued to add as bot
	if (GetLoginInfo(guid))
		return CHAR_ALREADY_IN_QUEUE;
	// already logged in
	if (sObjectAccessor.FindPlayerNotInWorld(guid))
		return CHAR_ALREADY_LOGGED_IN;
	// not all bots need master
	if (!masterGuid.IsEmpty())
	{
		// master player doesn't exist
		if (!ObjectAccessor::FindPlayerNotInWorld(masterGuid))
			return CHAR_MASTER_PLAYER_NOT_FOUND;
	}
	return CHAR_OK;
}


LuaAgentMgr::CheckResult LuaAgentMgr::AddAgent(std::string charName, ObjectGuid masterGuid, int logicID, std::string spec)
{
	CheckResult check = CheckAgentValid(charName, masterGuid);
	if (check == CHAR_OK)
	{
		const std::lock_guard<std::mutex> lock(loginMutex);
		m_toAdd.emplace(sObjectMgr.GetPlayerGuidByName(charName), LuaAgentInfoHolder(charName, masterGuid, logicID, spec));
	}
	return check;
}


void LuaAgentMgr::__AddAgents()
{
	const std::lock_guard<std::mutex> lock(loginMutex);
	for (std::map<ObjectGuid, LuaAgentInfoHolder>::iterator it = m_toAdd.begin(); it != m_toAdd.end();)
	{
		LuaAgentInfoHolder& info = it->second;
		if (info.status == LuaAgentInfoHolder::LOGGEDIN)
		{
			if (Player* player = ObjectAccessor::FindPlayerByNameNotInWorld(info.name.c_str()))
				m_agents[player->GetObjectGuid()] = player;
			info.status = LuaAgentInfoHolder::TODELETE;
		}

		if (info.status == LuaAgentInfoHolder::TODELETE)
		{
			it = m_toAdd.erase(it);
			continue;
		}

		if (info.status != LuaAgentInfoHolder::OFFLINE)
		{
			++it;
			continue;
		}

		// not all bots need to be owned
		if (info.masterGuid.IsEmpty() || ObjectAccessor::FindPlayerNotInWorld(info.masterGuid))
		{
			ObjectGuid guid = sObjectMgr.GetPlayerGuidByName(info.name);
			uint32 accountId = sObjectMgr.GetPlayerAccountIdByGUID(guid);
			LuaAgentLoginQueryHolder* holder = new LuaAgentLoginQueryHolder(accountId, guid, info.masterGuid, info.logicID, info.spec);
			if (!holder->Initialize())
			{
				delete holder;                                      // delete all unprocessed queries
				it = m_toAdd.erase(it);
				continue;
			}

			CharacterDatabase.DelayQueryHolder(&luaChrHandler, &LuaAgentCharacterHandler::HandleLoginCallback, holder);
			info.status = LuaAgentInfoHolder::LOADING;
		}
		else
		{
			it = m_toAdd.erase(it);
			continue;
		}
		++it;
	}
}


void LuaAgentMgr::OnAgentLogin(WorldSession* session, ObjectGuid guid, ObjectGuid masterGuid, int logicID, std::string spec)
{
	Player* player = session->GetPlayer();
	if (!player)
	{
		LogoutAgent(guid);
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "LuaAgent - player ptr is null after log in. Removing agent %s.", guid.GetString());
		return;
	}

	player->CreateLuaAI(player, masterGuid, logicID);
	LuaAgent* ai = player->GetLuaAI();
	if (!ai)
	{
		LogoutAgent(guid);
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "LuaAgent - ai failed to create after log in. Removing agent %s.", guid.GetString());
		return;
	}
	ai->SetSpec(spec);
	SetLoggedIn(player->GetObjectGuid());
}


void LuaAgentMgr::__RemoveAgents()
{
	for (auto& guid : m_toRemove)
		if (Player* player = GetAgent(guid))
		{
			if (WorldSession* session = player->GetSession())
			{
				// remove myself from group as LogoutPlayer won't do it without socket
				if (Group* group = player->GetGroup())
					session->HandleGroupDisbandOpcode(WorldPacket(CMSG_GROUP_DISBAND));
				session->LogoutPlayer(false);
				delete session;
			}
			m_agents.erase(guid);
		}
	m_toRemove.clear();

	for (auto itr = m_toRemoveParties.rbegin(); itr != m_toRemoveParties.rend(); ++itr)
		m_parties.erase(m_parties.begin() + *itr);
	m_toRemoveParties.clear();
}


void LuaAgentMgr::LogoutAgent(ObjectGuid guid)
{
	m_toRemove.insert(guid);
}


void LuaAgentMgr::LogoutAllAgents()
{
	for (auto& itr : m_agents)
		m_toRemove.insert(itr.first);
	for (size_t i = 0; i < m_parties.size(); ++i)
		m_toRemoveParties.insert(i);
}


void LuaAgentMgr::LogoutAllImmediately()
{
	LogoutAllAgents();
	__RemoveAgents();
}


void LuaAgentMgr::ReviveAll(Player* owner, float hp, bool sickness)
{
	for (auto& itr : m_agents)
	{
		Player* player = itr.second;
		if (!player || player->IsAlive() || !player->IsInWorld() || player->IsBeingTeleported()) continue;

		LuaAgent* agent = player->GetLuaAI();
		Player* master = nullptr;
		if (agent)
			master = ObjectAccessor::FindPlayer(agent->GetMasterGuid());

		// owned bots only
		if (master && owner->GetObjectGuid() == agent->GetMasterGuid())
			player->ResurrectPlayer(hp, sickness);
	}
}


void LuaAgentMgr::GroupAll(Player* owner)
{
	SetGroupAllInProgress(true);
	for (auto& itr : m_agents)
	{
		Player* player = itr.second;
		if (!player || !player->IsInWorld() || player->IsBeingTeleported() || player->GetGroupInvite()) continue;

		if (LuaAgent* agent = player->GetLuaAI())
		{
			Player* master = ObjectAccessor::FindPlayer(agent->GetMasterGuid());

			if (master && owner->GetObjectGuid() == agent->GetMasterGuid())
			{
				// already grouped
				if (player->IsInSameRaidWith(master))
					continue;

				// leave my group if any
				player->GetSession()->HandleGroupDisbandOpcode(WorldPacket(CMSG_GROUP_DISBAND));

				if (Group* group = master->GetGroup())
				{
					std::unique_ptr<WorldPacket> packet = std::make_unique<WorldPacket>(CMSG_GROUP_INVITE);
					*packet << std::string(player->GetName());
					master->GetSession()->QueuePacket(std::move(packet));
				}
				else
				{
					// Adds agent and master to group in the same logic frame
					// Necessary as group isn't truly created until at least 2 players join
					// failing to invite more members in the same frame
					WorldPacket packet(CMSG_GROUP_INVITE);
					packet << std::string(player->GetName());
					master->GetSession()->HandleGroupInviteOpcode(packet);
				}
			}
		}
	}
	SetGroupAllInProgress(false);
}

// ********************************************************
// **                       CLINE                        **
// ********************************************************

namespace
{
	CLineNet currentLines;
	uint32 helperId = VISUAL_WAYPOINT;
}


void LuaAgentMgr::CLineSaveSeg(G3D::Vector3& point, Player* gm)
{
	int mapId = gm->GetMap()->GetId();
	if (currentLines.mapId == -1)
		currentLines.mapId = mapId;

	if (currentLines.lines.empty())
		currentLines.lines.emplace_back();
	CLine& currentCLine = currentLines.lines.back();

	if (mapId != currentLines.mapId)
	{
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "LuaAgentMgr::CLineSaveSeg: attempt to start a new cline before current is finished");
		return;
	}

	currentCLine.Point(point, ObjectGuid());
	currentCLine.pts.back().SummonHelper(gm, helperId);
}


void LuaAgentMgr::CLineMoveSeg(G3D::Vector3& newpos, Player* gm, const ObjectGuid& helper)
{
	int mapId = gm->GetMap()->GetId();
	if (mapId != currentLines.mapId)
	{
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "LuaAgentMgr::CLineMoveSeg: attempt to edit cline from a different map");
		return;
	}

	if (CLinePoint* point = currentLines.FindPoint(helper))
		point->Move(newpos, gm, helperId);
	else
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "LuaAgentMgr::CLineMoveSeg: point not found");
}


void LuaAgentMgr::CLineDelLastSeg(Player* gm)
{
	if (currentLines.lines.size())
	{
		CLine& line = currentLines.lines.back();
		if (line.pts.size())
		{
			CLinePoint& cpt = line.pts.back();
			if (!cpt.helper.IsEmpty())
				if (Creature* c = gm->GetMap()->GetCreature(cpt.helper))
					c->Kill(c, nullptr, false);
			line.pts.pop_back();
		}
	}
}


void LuaAgentMgr::CLineWrite()
{
	currentLines.Write("out_cline.txt");
}


void LuaAgentMgr::CLineFinish(Player* gm)
{
	while (currentLines.lines.size())
	{
		CLine& line = currentLines.lines.back();
		while (line.pts.size())
			CLineDelLastSeg(gm);
		currentLines.lines.pop_back();
	}
	currentLines = CLineNet();
}


void LuaAgentMgr::CLineNewLine()
{
	if (currentLines.lines.empty())
		currentLines.lines.emplace_back();
	else if (currentLines.lines.back().pts.empty())
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "LuaAgentMgr::CLineNewLine: latest line has no points");
	else
		currentLines.lines.emplace_back();
}


void LuaAgentMgr::CLineLoadFrom(const std::string& fname, Player* gm)
{
	CLineFinish(gm);
	currentLines.Load(fname, gm, helperId);
}


void LuaAgentMgr::CLineLoad(const std::string& fname)
{
	std::unique_ptr<CLineNet> net = std::make_unique<CLineNet>();
	net->Load(fname, nullptr, 0u);
	if (net->mapId)
		m_clines.emplace(net->mapId, std::move(net));
}


CLineNet* LuaAgentMgr::CLineGet(uint32 key)
{
	auto it = m_clines.find(key);
	return (it == m_clines.end()) ? nullptr : it->second.get();
}
