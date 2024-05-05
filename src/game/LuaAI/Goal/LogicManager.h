#pragma once

struct lua_State;
class LuaAgent;

struct LogicInfo {
	std::string logicFunc;
	std::string logicInit;
	LogicInfo() : logicFunc("BAD") {}
	LogicInfo(const char* logicFunc, const char* logicInit) : logicFunc(logicFunc), logicInit(logicInit) {}
};

class LogicManager {
	static std::unordered_map<int, LogicInfo> logicInfoData;
	std::string logicFunc;
	std::string logicInit;
	int logicId;

public:
	static void RegisterLogic(int logic_id, const char* logic_func, const char* logic_init);

	LogicManager(int logic_id);

	void Init(lua_State* L, LuaAgent* ai);
	void Execute(lua_State* L, LuaAgent* ai);

	static void ClearRegistered() { logicInfoData.clear(); }
	void Print();

};

// ---------------------------------------------------------
//    LUA BINDS
// ---------------------------------------------------------

namespace LuaBindsAI {
	int REGISTER_LOGIC_FUNC(lua_State* L);
	void BindLogicManager(lua_State* L);
}