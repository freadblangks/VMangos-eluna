
#ifndef MANGOS_LUAAGENTMANAGER_H
#define MANGOS_LUAAGENTMANAGER_H

struct lua_State;
class PartyIntelligence;

typedef std::unordered_map<ObjectGuid, Player*> LuaAgentMap;


struct LuaAgentInfoHolder
{
	enum LBIHStatus
	{
		OFFLINE,
		LOADING,
		LOGGEDIN,
		TODELETE
	};
public:
	std::string name;
	const ObjectGuid masterGuid;
	int logicID;
	std::string spec;
	LBIHStatus status;
	LuaAgentInfoHolder(std::string name, const ObjectGuid& masterGuid, int logicID, std::string spec)
		: name(name), masterGuid(masterGuid), logicID(logicID), spec(spec), status(OFFLINE) {}
};


class LuaAgentMgr
{
	std::mutex loginMutex;

	friend class LuaAgentCharacterHandler;

	lua_State* L;

	bool m_bGroupAllInProgress;

	bool m_bLuaCeaseUpdates;
	bool m_bLuaReload;

	std::vector<std::unique_ptr<PartyIntelligence>> m_parties;
	LuaAgentMap m_agents;
	std::set<ObjectGuid> m_toRemove;
	std::map<ObjectGuid, LuaAgentInfoHolder> m_toAdd;

	LuaAgentMgr();

	void __AddAgents();
	void __RemoveAgents();

	void SetGroupAllInProgress(bool value) { m_bGroupAllInProgress = value; }

	bool LuaDofile(const std::string& filename);
	void LuaLoadAll();
	void LuaLoadFiles(const std::string& fpath);

	void SetLoggedIn(ObjectGuid guid);

protected:
	void EraseLoginInfo(ObjectGuid guid);

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
	const LuaAgentInfoHolder* GetLoginInfo(ObjectGuid guid);

	void AddParty(std::string name, ObjectGuid owner);
	CheckResult AddAgent(std::string charName, ObjectGuid masterGuid, int logicID, std::string spec);
	CheckResult CheckAgentValid(std::string charName, ObjectGuid masterGuid);

	void OnAgentLogin(WorldSession* session, ObjectGuid guid, ObjectGuid masterGuid, int logicID, std::string spec);
	void LogoutAgent(ObjectGuid guid);
	void LogoutAllAgents();
	void LogoutAllImmediately();

	void Update(uint32 diff);

	void GroupAll(Player* owner);
	void ReviveAll(Player* owner, float hp = 1.0, bool sickness = false);
	bool IsGroupAllInProgress() { return m_bGroupAllInProgress; }

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
