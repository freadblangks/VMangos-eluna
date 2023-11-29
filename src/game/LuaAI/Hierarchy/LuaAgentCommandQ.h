
#ifndef MANGOS_LUAAGENTCOMMANDQUEUE_H
#define MANGOS_LUAAGENTCOMMANDQUEUE_H

struct lua_State;

enum class AgentCmdType : uint8
{
	Move,
	Follow,
	Engage,
	Tank,
	Max,
};


class AgentCmd
{
public:
	enum class State : uint8
	{
		Waiting,
		InProgress,
		Complete,
		Failed,
		Max,
	};

protected:
	AgentCmdType tp;
	State state;

public:
	AgentCmd(AgentCmdType tp) : tp(tp), state(State::Waiting) {}
	virtual int Push(lua_State* L) { return 0; }
	int PushType(lua_State* L);
	AgentCmdType GetType() { return tp; }
	State GetState() { return state; }
	void SetState(State state) { this->state = state; }
};


class AgentCmdMove : public AgentCmd
{
	G3D::Vector3 destination;

public:
	AgentCmdMove(float x, float y, float z) : destination(x,y,z), AgentCmd(AgentCmdType::Move) {}
	int Push(lua_State* L) override;
};


class AgentCmdFollow : public AgentCmd
{
	ObjectGuid targetGuid;
	float angle;
	float dist;

public:
	AgentCmdFollow(const ObjectGuid& targetGuid, float dist, float angle) : targetGuid(targetGuid), AgentCmd(AgentCmdType::Follow), angle(angle), dist(dist) {}
	int Push(lua_State* L) override;
};


class AgentCmdEngage : public AgentCmd
{
	float angle;

public:
	AgentCmdEngage(float angle) : AgentCmd(AgentCmdType::Engage), angle(angle) {}
	int Push(lua_State* L) override;
};


class AgentCmdTank : public AgentCmd
{
	ObjectGuid targetGuid;

public:
	AgentCmdTank(const ObjectGuid& targetGuid) : AgentCmd(AgentCmdType::Tank), targetGuid(targetGuid) {}
	int Push(lua_State* L) override;
};


class CmdQueue
{
	std::vector<AgentCmd> commands;

public:
	CmdQueue() {}

	auto begin() { return commands.begin(); }
	auto end() { return commands.end(); }
	size_t size() { return commands.size(); }
	AgentCmd& operator[](int i) { return commands[i]; }
};

#endif
