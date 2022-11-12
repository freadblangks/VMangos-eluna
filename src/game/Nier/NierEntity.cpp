#include "NierEntity.h"
#include "NierManager.h"
#include "NierConfig.h"
#include "Strategy_Base.h"
#include "Script_Base.h"
#include "Group.h"

NierEntity::NierEntity(uint32 pmNierID)
{
	nier_id = pmNierID;
	account_id = 0;
	account_name = "";
	character_id = 0;
	target_level = 0;
	checkDelay = 5 * TimeConstants::IN_MILLISECONDS;
	entityState = NierEntityState::NierEntityState_OffLine;
}

void NierEntity::Update(uint32 pmDiff)
{
	checkDelay -= pmDiff;
	if (checkDelay < 0)
	{
		checkDelay = urand(2 * TimeConstants::IN_MILLISECONDS, 10 * TimeConstants::IN_MILLISECONDS);
		switch (entityState)
		{
		case NierEntityState::NierEntityState_None:
		{
			checkDelay = urand(5 * TimeConstants::MINUTE * TimeConstants::IN_MILLISECONDS, 10 * TimeConstants::MINUTE * TimeConstants::IN_MILLISECONDS);
			break;
		}
		case NierEntityState::NierEntityState_OffLine:
		{
			checkDelay = urand(5 * TimeConstants::MINUTE * TimeConstants::IN_MILLISECONDS, 10 * TimeConstants::MINUTE * TimeConstants::IN_MILLISECONDS);
			break;
		}
		case NierEntityState::NierEntityState_Enter:
		{
			entityState = NierEntityState::NierEntityState_CheckAccount;
			sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Nier %s is ready to go online.", account_name.c_str());
			break;
		}
		case NierEntityState::NierEntityState_CheckAccount:
		{
			if (account_name.empty())
			{
				entityState = NierEntityState::NierEntityState_None;
			}
			else
			{
				account_id = sNierManager->CheckNierAccount(account_name);
				if (account_id > 0)
				{					
					sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Nier %s is ready.", account_name.c_str());
					entityState = NierEntityState::NierEntityState_CheckCharacter;
				}
				else
				{
					sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Nier %s is not ready.", account_name.c_str());
					entityState = NierEntityState::NierEntityState_CreateAccount;
				}
			}
			break;
		}
		case NierEntityState::NierEntityState_CreateAccount:
		{
			if (account_name.empty())
			{
				entityState = NierEntityState::NierEntityState_None;
			}
			else
			{
				if (!sNierManager->CreateNierAccount(account_name))
				{
					sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Nier id %d account creation failed.", nier_id);
					entityState = NierEntityState::NierEntityState_None;
				}
				else
				{
					entityState = NierEntityState::NierEntityState_CheckAccount;
				}
			}
			break;
		}
		case NierEntityState::NierEntityState_CheckCharacter:
		{
			character_id = sNierManager->CheckAccountCharacter(account_id);
			if (character_id > 0)
			{
				sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Nier account_id %d character_id %d is ready.", account_id, character_id);
				entityState = NierEntityState::NierEntityState_DoLogin;
			}
			else
			{
				sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Nier account_id %d character_id is not ready.", account_id);
				entityState = NierEntityState::NierEntityState_CreateCharacter;
			}
			break;
		}
		case NierEntityState::NierEntityState_CreateCharacter:
		{
			uint32  targetClass = Classes::CLASS_PALADIN;
			uint32 raceIndex = urand(0, sNierManager->availableRaces[targetClass].size() - 1);
			uint32 targetRace = sNierManager->availableRaces[targetClass][raceIndex];
			character_id = sNierManager->CreateNierCharacter(account_id);
			if (character_id > 0)
			{
				std::ostringstream sqlStream;
				sqlStream << "update nier set character_id = " << character_id << " where nier_id = " << nier_id;
				std::string sql = sqlStream.str();
				CharacterDatabase.DirectExecute(sql.c_str());
				entityState = NierEntityState::NierEntityState_CheckCharacter;
			}
			else
			{
				entityState = NierEntityState::NierEntityState_None;
			}
			break;
		}
		case NierEntityState::NierEntityState_DoLogin:
		{
			sNierManager->LoginNier(account_id, character_id);
			checkDelay = urand(5 * TimeConstants::IN_MILLISECONDS, 10 * TimeConstants::IN_MILLISECONDS);
			entityState = NierEntityState::NierEntityState_CheckLogin;
			break;
		}
		case NierEntityState::NierEntityState_CheckLogin:
		{
			Player* me = sNierManager->CheckLogin(account_id, character_id);
			if (me)
			{
				account_id = account_id;
				character_id = character_id;
				entityState = NierEntityState::NierEntityState_Initialize;
			}
			else
			{
				entityState = NierEntityState::NierEntityState_None;
			}
			break;
		}
		case NierEntityState::NierEntityState_Initialize:
		{
			ObjectGuid guid = ObjectGuid(HighGuid::HIGHGUID_PLAYER, character_id);
			if (Player* me = ObjectAccessor::FindPlayer(guid))
			{
				sNierManager->InitializeCharacter(me, target_level);
				for (std::unordered_map<uint32, Strategy_Base*>::iterator aiIT = me->strategyMap.begin(); aiIT != me->strategyMap.end(); aiIT++)
				{
					if (Strategy_Base* eachAI = aiIT->second)
					{
						eachAI->sb->Initialize();
						eachAI->Reset();
					}
				}
				entityState = NierEntityState::NierEntityState_Online;
			}
			else
			{
				entityState = NierEntityState::NierEntityState_None;
			}
			break;
		}
		case NierEntityState::NierEntityState_Online:
		{
			checkDelay = urand(10 * TimeConstants::MINUTE * TimeConstants::IN_MILLISECONDS, 20 * TimeConstants::MINUTE * TimeConstants::IN_MILLISECONDS);
			ObjectGuid guid = ObjectGuid(HighGuid::HIGHGUID_PLAYER, character_id);
			if (Player* me = ObjectAccessor::FindPlayer(guid))
			{
				sNierManager->PrepareNier(me);
			}
			break;
		}
		case NierEntityState::NierEntityState_Exit:
		{
			ObjectGuid guid = ObjectGuid(HighGuid::HIGHGUID_PLAYER, character_id);
			if (Player* me = ObjectAccessor::FindPlayer(guid))
			{
				if (me->GetGroup())
				{
					entityState = NierEntityState::NierEntityState_Online;
					break;
				}
			}
			sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Nier %d is ready to go offline.", nier_id);
			entityState = NierEntityState::NierEntityState_DoLogoff;
			break;
		}
		case NierEntityState::NierEntityState_DoLogoff:
		{
			sNierManager->LogoutNier(character_id);
			entityState = NierEntityState::NierEntityState_CheckLogoff;
			break;
		}
		case NierEntityState::NierEntityState_CheckLogoff:
		{
			ObjectGuid guid = ObjectGuid(HighGuid::HIGHGUID_PLAYER, character_id);
			if (Player* me = ObjectAccessor::FindPlayer(guid))
			{
				sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Log out nier %s failed", me->GetName());
				entityState = NierEntityState::NierEntityState_None;
				break;
			}
			entityState = NierEntityState::NierEntityState_OffLine;
			break;
		}
		default:
		{
			checkDelay = urand(5 * TimeConstants::MINUTE * TimeConstants::IN_MILLISECONDS, 10 * TimeConstants::MINUTE * TimeConstants::IN_MILLISECONDS);
			break;
		}
		}
	}
}
