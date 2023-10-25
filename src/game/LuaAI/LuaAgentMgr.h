
#ifndef MANGOS_LUAAGENTMANAGER_H
#define MANGOS_LUAAGENTMANAGER_H

struct lua_State;

typedef std::unordered_map<ObjectGuid, Player*> LuaAgentMap;


struct LuaAgentInfoHolder
{
	enum LBIHStatus
	{
		OFFLINE,
		LOADING
	};
public:
	std::string name;
	uint32 masterAccountId;
	int logicID;
	std::string spec;
	LBIHStatus status;
	LuaAgentInfoHolder(std::string name, uint32 masterAccountId, int logicID, std::string spec)
		: name(name), masterAccountId(masterAccountId), logicID(logicID), spec(spec), status(OFFLINE) {}
};


class LuaAgentMgr
{

	lua_State* L;

	bool m_bLuaCeaseUpdates;
	bool m_bLuaReload;

	LuaAgentMap m_agents;
	std::set<ObjectGuid> m_toRemove;
	std::map<ObjectGuid, LuaAgentInfoHolder> m_toAdd;

	LuaAgentMgr();

	void __AddAgents();
	void __RemoveAgents();

	const LuaAgentInfoHolder* GetLoginInfo(ObjectGuid guid);
	void EraseLoginInfo(ObjectGuid guid) { m_toAdd.erase(guid); }

public:
	enum CheckResult
	{
		CHAR_DOESNT_EXIST,
		CHAR_ALREADY_LOGGED_IN,
		CHAR_ACCOUNT_DOESNT_EXIST,
		CHAR_ALREADY_EXISTS,
		CHAR_ALREADY_IN_QUEUE,
		CHAR_MASTER_SESSION_NOT_FOUND,
		CHAR_MASTER_PLAYER_NOT_FOUND,
		CHAR_OK,
	};

	~LuaAgentMgr();

	lua_State* Lua() { return L; }

	void LuaReload() { m_bLuaReload = true; }

	Player* GetAgent(ObjectGuid guid);

	CheckResult AddAgent(std::string charName, uint32 masterAccountId, int logicID, std::string spec);
	CheckResult CheckAgentValid(std::string charName, uint32 masterAccountId);

	void OnAgentLogin(WorldSession* session, ObjectGuid guid, ObjectGuid masterGuid, int logicID, std::string spec);
	void LogoutAgent(ObjectGuid guid);
	void LogoutAllAgents();

	void Update(uint32 diff);

	static LuaAgentMgr& getInstance()
	{
		static LuaAgentMgr instance;
		return instance;
	}

	LuaAgentMgr(LuaAgentMgr const&) = delete;
	void operator=(LuaAgentMgr const&) = delete;

};

#define sLuaAgentMgr LuaAgentMgr::getInstance()

#endif