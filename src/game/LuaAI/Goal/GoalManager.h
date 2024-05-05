#pragma once

#include <stack>

struct lua_State;
class Goal;
class LuaAgent;

const std::string GOAL_ACTIVATE_POSTFIX = "_Activate";
const std::string GOAL_UPDATE_POSTFIX = "_Update";
const std::string GOAL_TERMINATE_POSTFIX = "_Terminate";

// doesn't matter much if we optimize anything here, only affects the initial load.

struct GoalInfo {
	std::string name;
	std::string fActivate;
	std::string fUpdate;
	std::string fTerminate;
	GoalInfo() : name( "BAD" ), fActivate( "BAD" ), fUpdate( "BAD" ), fTerminate( "BAD" ) {}
	GoalInfo( std::string name, std::string fActivate, std::string fUpdate, std::string fTerminate )
		: name( name ), fActivate( fActivate ), fUpdate( fUpdate ), fTerminate( fTerminate ) {}
};

class GoalManager {

	static std::unordered_map<int, GoalInfo> goalInfoData;
	std::stack<Goal*> activationStack;
	std::deque<Goal*> terminationQueue;

public:
	static void RegisterGoal( int goalId, std::string name );

	GoalManager();

	void Activate( lua_State* L, LuaAgent* ai );
	void ClearActivationStack() { while (!activationStack.empty()) activationStack.pop(); }

	void UpdateRecursive( lua_State* L, LuaAgent* ai, Goal* goal );
	void Update( lua_State* L, LuaAgent* ai );

	void TerminateRecursive( lua_State* L, LuaAgent* ai, Goal* goal );
	void Terminate( lua_State* L, LuaAgent* ai );

	void PushGoalOnActivationStack( Goal* goal );
	void PushGoalOnTerminationQueue( Goal* goal );

	static void ClearRegistered() { goalInfoData.clear(); }
	void PrintRegistered();

};

// ---------------------------------------------------------
//    LUA BINDS
// ---------------------------------------------------------

namespace LuaBindsAI {

	int REGISTER_GOAL( lua_State* L );
	void BindGoalManager( lua_State* L );

}