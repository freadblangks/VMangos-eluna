
#include "LuaAgentMgr.h"
#include "Language.h"
#include "Chat.h"

bool ChatHandler::HandleLuabAddCommand(char* args)
{
	// part from PartyBotMgr.cpp ChatHandler::HandlePartyBotLoadCommand
	Player* pPlayer = GetSession()->GetPlayer();
	if (!pPlayer)
		return false;

	std::string name = ExtractPlayerNameFromLink(&args);
	if (name.empty())
	{
		SendSysMessage(LANG_PLAYER_NOT_FOUND);
		SetSentErrorMessage(true);
		return false;
	}

	int logicID = -1;
	if (!ExtractInt32(&args, logicID))
	{
		SendSysMessage("Incorrect syntax. Expected logic id. Usage: .luab add name logic spec");
		SetSentErrorMessage(true);
		return false;
	}

	std::string spec = "";
	if (char* specCstr = ExtractArg(&args))
		spec = std::string(specCstr);

	LuaAgentMgr::CheckResult result = sLuaAgentMgr.AddAgent(name, pPlayer->GetObjectGuid(), logicID, spec);
	switch (result)
	{
	case LuaAgentMgr::CHAR_DOESNT_EXIST:
		PSendSysMessage("Character %s doesn't exist", name.c_str());
		return true;
	case LuaAgentMgr::CHAR_ACCOUNT_DOESNT_EXIST:
		PSendSysMessage("Character %s has no account", name.c_str());
		return true;
	case LuaAgentMgr::CHAR_ALREADY_LOGGED_IN:
		PSendSysMessage("Character %s already logged in", name.c_str());
		return true;
	case LuaAgentMgr::CHAR_ALREADY_IN_QUEUE:
		PSendSysMessage("Character %s already queued as a bot", name.c_str());
		return true;
	case LuaAgentMgr::CHAR_ALREADY_EXISTS:
		PSendSysMessage("Character %s already exists as a bot", name.c_str());
		return true;
	case LuaAgentMgr::CHAR_OK:
		PSendSysMessage("Character %s added as a bot", name.c_str());
		return true;
	}
	return true;
}


bool ChatHandler::HandleLuabAddPartyCommand(char* args)
{
	Player* owner = GetSession()->GetPlayer();
	if (!owner)
		return false;

	std::string name;
	if (char* nameCstr = ExtractArg(&args))
		name = std::string(nameCstr);
	else
	{
		SendSysMessage("Incorrect syntax. Expected name. Usage: .luab addparty name");
		SetSentErrorMessage(true);
		return false;
	}

	sLuaAgentMgr.AddParty(name, owner->GetObjectGuid());

	return true;
}


bool ChatHandler::HandleLuabRemoveCommand(char* args)
{
	if (Player* selection = GetSelectedPlayer())
		if (LuaAgent* agent = selection->GetLuaAI())
			if (!agent->GetPartyIntelligence())
				sLuaAgentMgr.LogoutAgent(selection->GetObjectGuid());
			else
			{
				SendSysMessage("Unable to remove this bot. Use .luab remparty");
				SetSentErrorMessage(true);
				return false;
			}
	return true;
}


bool ChatHandler::HandleLuabRemoveAllCommand(char* args)
{
	sLuaAgentMgr.LogoutAllAgents();
	return true;
}


bool ChatHandler::HandleLuabResetCommand(char* args)
{
	sLuaAgentMgr.LuaReload();
	return true;
}


bool ChatHandler::HandleLuabGroupAllCommand(char* args)
{
	sLuaAgentMgr.GroupAll(GetSession()->GetPlayer());
	return true;
}


bool ChatHandler::HandleLuabReviveAllCommand(char* args)
{
	float health = .0f;
	int32 sickness = 0;
	if (!ExtractFloat(&args, health))
		health = 1.0f;
	else if (!ExtractInt32(&args, sickness))
		sickness = 1;
	else if (sickness > 0)
		sickness = 1;
	sLuaAgentMgr.ReviveAll(GetSession()->GetPlayer(), health, sickness == 1);
	return true;
}
