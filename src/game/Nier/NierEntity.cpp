#include "NierEntity.h"
#include "NierManager.h"
#include "NierConfig.h"
#include "MotionMaster.h"
#include "NierStrategies/NierStrategy_Base.h"
#include "NierActions/NierAction_Base.h"
#include "NierActions/NierAction_Druid.h"
#include "NierActions/NierAction_Hunter.h"
#include "NierActions/NierAction_Mage.h"
#include "NierActions/NierAction_Paladin.h"
#include "NierActions/NierAction_Priest.h"
#include "NierActions/NierAction_Rogue.h"
#include "NierActions/NierAction_Shaman.h"
#include "NierActions/NierAction_Warlock.h"
#include "NierActions/NierAction_Warrior.h"

#include "AccountMgr.h"
#include "Player.h"
#include "Group.h"
#include "World.h"
#include "Bag.h"

NierEntity::NierEntity()
{
	nier_id = 0;
	account_id = 0;
	account_name = "";
	character_id = 0;
	target_level = 0;
	target_specialty = 0;
	checkDelay = 5 * IN_MILLISECONDS;
	entityState = NierEntityState::NierEntityState_OffLine;
	offlineDelay = 0;
}

void NierEntity::Update(uint32 pmDiff)
{
	if (offlineDelay >= 0)
	{
		offlineDelay -= pmDiff;
	}
	if (checkDelay >= 0)
	{
		checkDelay -= pmDiff;
	}
	if (checkDelay < 0)
	{
		checkDelay = urand(2 * IN_MILLISECONDS, 10 * IN_MILLISECONDS);
		switch (entityState)
		{
		case NierEntityState::NierEntityState_None:
		{
			checkDelay = urand(5 * MINUTE * IN_MILLISECONDS, 10 * MINUTE * IN_MILLISECONDS);
			break;
		}
		case NierEntityState::NierEntityState_OffLine:
		{
			break;
		}
		case NierEntityState::NierEntityState_Enter:
		{
			entityState = NierEntityState::NierEntityState_CheckAccount;
			sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, "Nier %d is ready to go online.", account_id);
			break;
		}
		case NierEntityState::NierEntityState_CheckAccount:
		{
			if (account_name.empty())
			{
				if (account_id > 0)
				{
					sAccountMgr.SetSecurity(account_id, AccountTypes::SEC_MODERATOR);
					sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, "Nier %s is ready.", account_name.c_str());
					entityState = NierEntityState::NierEntityState_CheckCharacter;
				}
				else
				{
					entityState = NierEntityState::NierEntityState_None;
				}
			}
			else
			{
				uint32 queryAccountId = 0;
				std::ostringstream accountQueryStream;
				accountQueryStream << "SELECT id FROM account where username = '" << account_name << "'";
				QueryResult* nierAccountQR = LoginDatabase.Query(accountQueryStream.str().c_str());
				if (nierAccountQR)
				{
					Field* fields = nierAccountQR->Fetch();
					queryAccountId = fields[0].GetUInt32();
				}
				delete nierAccountQR;
				if (queryAccountId > 0)
				{
					account_id = queryAccountId;
					sAccountMgr.SetSecurity(account_id, AccountTypes::SEC_MODERATOR);
					std::ostringstream sqlStream;
					sqlStream << "update nier set account_id = " << account_id << " where nier_id = " << nier_id;
					std::string sql = sqlStream.str();
					CharacterDatabase.DirectExecute(sql.c_str());
					sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, "Nier %s is ready.", account_name.c_str());
					entityState = NierEntityState::NierEntityState_CheckCharacter;
				}
				else
				{
					sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, "Nier %s is not ready.", account_name.c_str());
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
				if (sAccountMgr.CreateAccount(account_name, NIER_MARK) == AccountOpResult::AOR_OK)
				{
					entityState = NierEntityState::NierEntityState_CheckAccount;
				}
				else
				{
					sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, "Nier id %d account creation failed.", nier_id);
					entityState = NierEntityState::NierEntityState_None;
				}
			}
			break;
		}
		case NierEntityState::NierEntityState_CheckCharacter:
		{
			std::ostringstream queryStream;
			queryStream << "SELECT guid FROM characters where account = " << account_id;
			QueryResult* characterQR = CharacterDatabase.Query(queryStream.str().c_str());
			if (characterQR)
			{
				Field* characterFields = characterQR->Fetch();
				character_id = characterFields[0].GetUInt32();
				if (character_id > 0)
				{
					std::ostringstream sqlStream;
					sqlStream << "update nier set character_id = " << character_id << " where nier_id = " << nier_id;
					std::string sql = sqlStream.str();
					CharacterDatabase.DirectExecute(sql.c_str());
					sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, "Nier account_id %d character_id %d is ready.", account_id, character_id);
					//entityState = NierEntityState::NierEntityState_DoEnum;
					entityState = NierEntityState::NierEntityState_DoLogin;
					break;
				}
			}
			delete characterQR;
			sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, "Nier account_id %d character_id is not ready.", account_id);
			entityState = NierEntityState::NierEntityState_CreateCharacter;
			break;
		}
		case NierEntityState::NierEntityState_CreateCharacter:
		{
			std::string currentName = "";
			bool nameValid = false;
			while (sNierManager->nierNameMap.find(sNierManager->nameIndex) != sNierManager->nierNameMap.end())
			{
				currentName = sNierManager->nierNameMap[sNierManager->nameIndex];
				std::ostringstream queryStream;
				queryStream << "SELECT count(*) FROM characters where name = '" << currentName << "'";
				QueryResult* checkNameQR = CharacterDatabase.Query(queryStream.str().c_str());
				if (!checkNameQR)
				{
					sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, "Name %s is available", currentName.c_str());
					nameValid = true;
				}
				else
				{
					Field* nameCountFields = checkNameQR->Fetch();
					uint32 nameCount = nameCountFields[0].GetUInt32();
					if (nameCount == 0)
					{
						nameValid = true;
					}
				}
				sNierManager->nameIndex++;
				if (nameValid)
				{
					break;
				}
				delete checkNameQR;
			}
			if (!nameValid)
			{
				sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_ERROR, "No available names");
				entityState = NierEntityState::NierEntityState_None;
				return;
			}
			uint8 gender = 0, skin = 0, face = 0, hairStyle = 0, hairColor = 0, facialHair = 0;
			while (true)
			{
				gender = urand(0, 100);
				if (gender < 50)
				{
					gender = 0;
				}
				else
				{
					gender = 1;
				}
				face = urand(0, 5);
				hairStyle = urand(0, 5);
				hairColor = urand(0, 5);
				facialHair = urand(0, 5);

				WorldSession* createSession = new WorldSession(account_id, NULL, AccountTypes::SEC_PLAYER, 0, LocaleConstant::LOCALE_enUS);
				Player* newPlayer = new Player(createSession);
				if (!Player::SaveNewPlayer(createSession, sObjectMgr.GeneratePlayerLowGuid(), currentName, target_race, target_class, gender, skin, face, hairStyle, hairColor, facialHair))
				{
					newPlayer->CleanupsBeforeDelete();
					delete createSession;
					delete newPlayer;
					sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_ERROR, "Character create failed, %s %d %d ", currentName.c_str(), target_race, target_class);
					sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, "Try again");
					continue;
				}
				//newPlayer->GetMotionMaster()->Initialize();
				//newPlayer->SetAtLoginFlag(AT_LOGIN_NONE);
				//newPlayer->SaveToDB(true, true);
				//character_id = newPlayer->GetGUIDLow();
				//if (character_id > 0)
				//{
				//	createSession->isNier = true;
				//	sWorld.AddSession(createSession);
				//	std::ostringstream replyStream;
				//	replyStream << "nier character created : account - " << account_id << " character - " << newPlayer->GetGUIDLow() << " " << currentName;
				//	std::string replyString = replyStream.str();
				//	sWorld.SendServerMessage(ServerMessageType::SERVER_MSG_CUSTOM, replyString.c_str());
				//	sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, replyString.c_str());
				//	break;
				//}
				delete createSession;
				break;
			}
			entityState = NierEntityState::NierEntityState_CheckCharacter;
			break;
		}
		case NierEntityState::NierEntityState_DoEnum:
		{
			WorldSession* loginSession = sWorld.FindSession(account_id);
			if (!loginSession)
			{
				loginSession = new WorldSession(account_id, NULL, AccountTypes::SEC_PLAYER, 0, LocaleConstant::LOCALE_enUS);
				sWorld.AddSession(loginSession);
			}
			loginSession->isNier = true;
			WorldPacket wpEnum(CMSG_CHAR_ENUM, 4);
			loginSession->HandleCharEnumOpcode(wpEnum);
			sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, "Enum character %d %d ", account_id, character_id);
			checkDelay = urand(5 * IN_MILLISECONDS, 10 * IN_MILLISECONDS);
			entityState = NierEntityState::NierEntityState_CheckEnum;
			break;
		}
		case NierEntityState::NierEntityState_CheckEnum:
		{
			checkDelay = urand(5 * IN_MILLISECONDS, 10 * IN_MILLISECONDS);
			break;
		}
		case NierEntityState::NierEntityState_DoLogin:
		{
			WorldSession* loginSession = sWorld.FindSession(account_id);
			if (!loginSession)
			{
				loginSession = new WorldSession(account_id, NULL, AccountTypes::SEC_PLAYER, 0, LocaleConstant::LOCALE_enUS);
				sWorld.AddSession(loginSession);
			}
			loginSession->isNier = true;
			ObjectGuid playerGuid = ObjectGuid(HIGHGUID_PLAYER, character_id);
			loginSession->HandlePlayerLogin_Simple(playerGuid);
			std::ostringstream replyStream;
			replyStream << "log in character : account - " << account_id << " character - " << character_id;
			std::string replyString = replyStream.str();
			sWorld.SendServerMessage(ServerMessageType::SERVER_MSG_CUSTOM, replyString.c_str());
			sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, replyString.c_str());
			checkDelay = urand(5 * IN_MILLISECONDS, 10 * IN_MILLISECONDS);
			entityState = NierEntityState::NierEntityState_CheckLogin;
			break;
		}
		case NierEntityState::NierEntityState_CheckLogin:
		{
			ObjectGuid playerGuid = ObjectGuid(HIGHGUID_PLAYER, character_id);
			if (Player* me = ObjectAccessor::FindPlayer(playerGuid))
			{
				if (me->IsInWorld())
				{
					me->isNier = true;
					std::ostringstream replyStream;
					replyStream << "nier character logged in : account - " << account_id << " character - " << character_id;
					std::string replyString = replyStream.str();
					sWorld.SendServerMessage(ServerMessageType::SERVER_MSG_CUSTOM, replyString.c_str());
					sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, replyString.c_str());
					entityState = NierEntityState::NierEntityState_Initialize;
					break;
				}
			}
			checkDelay = urand(5 * IN_MILLISECONDS, 10 * IN_MILLISECONDS);
			break;
		}
		case NierEntityState::NierEntityState_Initialize:
		{
			ObjectGuid playerGuid = ObjectGuid(HIGHGUID_PLAYER, character_id);
			if (Player* me = ObjectAccessor::FindPlayer(playerGuid))
			{
				if (me->IsInWorld())
				{
					me->activeStrategyIndex = 0;
					me->nierStrategyMap.clear();
					me->nierStrategyMap[me->activeStrategyIndex] = new NierStrategy_Base();
					me->nierStrategyMap[me->activeStrategyIndex]->me = me;
					me->nierStrategyMap[me->activeStrategyIndex]->Reset();
					switch (target_class)
					{
						//case Classes::CLASS_DEATH_KNIGHT:
						//{
						//    me->nierAction = new NierAction_Base(me);
						//    break;
						//}
					case Classes::CLASS_DRUID:
					{
						me->nierAction = new NierAction_Druid(me);
						break;
					}
					case Classes::CLASS_HUNTER:
					{
						me->nierAction = new NierAction_Hunter(me);
						break;
					}
					case Classes::CLASS_MAGE:
					{
						me->nierAction = new NierAction_Mage(me);
						break;
					}
					case Classes::CLASS_PALADIN:
					{
						me->nierAction = new NierAction_Paladin(me);
						break;
					}
					case Classes::CLASS_PRIEST:
					{
						me->nierAction = new NierAction_Priest(me);
						break;
					}
					case Classes::CLASS_ROGUE:
					{
						me->nierAction = new NierAction_Rogue(me);
						break;
					}
					case Classes::CLASS_SHAMAN:
					{
						me->nierAction = new NierAction_Shaman(me);
						break;
					}
					case Classes::CLASS_WARLOCK:
					{
						me->nierAction = new NierAction_Warlock(me);
						break;
					}
					case Classes::CLASS_WARRIOR:
					{
						me->nierAction = new NierAction_Warrior(me);
						break;
					}
					default:
					{
						me->nierAction = new NierAction_Base(me);
						break;
					}
					}
					me->nierAction->InitializeCharacter(target_level, target_specialty);
					me->SaveToDB(true, true);
					offlineDelay = urand(2 * HOUR * IN_MILLISECONDS, 4 * HOUR * IN_MILLISECONDS);
					std::ostringstream replyStream;
					replyStream << "nier initialized : account - " << account_id << " character - " << character_id << " " << me->GetName();
					std::string replyString = replyStream.str();
					sWorld.SendServerMessage(ServerMessageType::SERVER_MSG_CUSTOM, replyString.c_str());
					sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, replyString.c_str());
					entityState = NierEntityState::NierEntityState_Equip;
					break;
				}
			}
			entityState = NierEntityState::NierEntityState_OffLine;
			break;
		}
		case NierEntityState::NierEntityState_Equip:
		{
			ObjectGuid playerGuid = ObjectGuid(HIGHGUID_PLAYER, character_id);
			if (Player* me = ObjectAccessor::FindPlayer(playerGuid))
			{
				if (me->IsInWorld())
				{
					if (me->nierAction->InitializeEquipments())
					{
						me->SaveToDB(true, true);
						for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
						{
							me->DestroyItem(INVENTORY_SLOT_BAG_0, i, true);
						}
						me->SaveToDB(true, true);
						//for (int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
						//{
						//	me->AutoUnequipItemFromSlot(i);
						//}
						//me->SaveToDB(true, true);
						//for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
						//{
						//	if (Item* pItem = me->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
						//	{
						//		uint16 dest = 0;
						//		InventoryResult msg = me->CanEquipItem(NULL_SLOT, dest, pItem, false);
						//		if (msg == EQUIP_ERR_OK)
						//		{
						//			me->EquipItem(dest, pItem, true);
						//		}
						//	}
						//}
						//me->SaveToDB(true, true);

						for (std::unordered_map<uint32, NierStrategy_Base*>::iterator nsbIt = me->nierStrategyMap.begin(); nsbIt != me->nierStrategyMap.end(); nsbIt++)
						{
							if (NierStrategy_Base* nsb = nsbIt->second)
							{
								nsb->me = me;
								nsb->initialized = true;
							}
						}
						me->nierStrategyMap[me->activeStrategyIndex]->randomTeleportDelay = urand(sNierConfig.RandomTeleportDelay_Min, sNierConfig.RandomTeleportDelay_Max);
						entityState = NierEntityState::NierEntityState_Online;
						std::ostringstream msgStream;
						msgStream << me->GetName() << " Equiped all slots";
						sWorld.SendServerMessage(ServerMessageType::SERVER_MSG_CUSTOM, msgStream.str().c_str());
						break;
					}
				}
			}
			offlineDelay = 100;
			break;
		}
		case NierEntityState::NierEntityState_Online:
		{
			if (offlineDelay > 0)
			{
				checkDelay = urand(10 * MINUTE * IN_MILLISECONDS, 20 * MINUTE * IN_MILLISECONDS);
				ObjectGuid playerGuid = ObjectGuid(HIGHGUID_PLAYER, character_id);
				if (Player* me = ObjectAccessor::FindPlayer(playerGuid))
				{
					if (me->IsInWorld())
					{
						me->CinematicEnd();
						me->nierAction->Prepare();
						if (Group* myGroup = me->GetGroup())
						{
							if (Player* leader = ObjectAccessor::FindPlayer(myGroup->GetLeaderGuid()))
							{
								if (leader->GetSession()->isNier)
								{
									me->RemoveFromGroup();
								}
							}
						}
					}
					else
					{
						entityState = NierEntityState::NierEntityState_Exit;
					}
				}
			}
			else
			{
				entityState = NierEntityState::NierEntityState_Exit;
			}
			break;
		}
		case NierEntityState::NierEntityState_Exit:
		{
			ObjectGuid playerGuid = ObjectGuid(HIGHGUID_PLAYER, character_id);
			if (Player* me = ObjectAccessor::FindPlayer(playerGuid))
			{
				if (me->IsInWorld())
				{
					if (me->GetGroup())
					{
						entityState = NierEntityState::NierEntityState_Online;
						break;
					}
				}
			}
			sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, "Nier %d is ready to go offline.", nier_id);
			entityState = NierEntityState::NierEntityState_DoLogoff;
			break;
		}
		case NierEntityState::NierEntityState_DoLogoff:
		{
			checkDelay = urand(10 * IN_MILLISECONDS, 20 * IN_MILLISECONDS);
			ObjectGuid playerGuid = ObjectGuid(HIGHGUID_PLAYER, character_id);
			if (Player* me = ObjectAccessor::FindPlayer(playerGuid))
			{
				if (me->IsInWorld())
				{
					if (WorldSession* ws = me->GetSession())
					{
						//me->RemoveFromGroup();
						ws->LogoutPlayer(true);
						entityState = NierEntityState::NierEntityState_CheckLogoff;
						break;
					}
				}
			}
			entityState = NierEntityState::NierEntityState_OffLine;
			break;
		}
		case NierEntityState::NierEntityState_CheckLogoff:
		{
			ObjectGuid playerGuid = ObjectGuid(HIGHGUID_PLAYER, character_id);
			if (Player* me = ObjectAccessor::FindPlayer(playerGuid))
			{
				sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_ERROR, "Log out nier %s failed", me->GetName());
				entityState = NierEntityState::NierEntityState_None;
				break;
			}
			entityState = NierEntityState::NierEntityState_OffLine;
			break;
		}
		case NierEntityState::NierEntityState_RedoLogin:
		{
			entityState = NierEntityState::NierEntityState_OffLine;
			break;
		}
		case NierEntityState::NierEntityState_CheckRedoLogin:
		{
			entityState = NierEntityState::NierEntityState_OffLine;
			break;
		}
		default:
		{
			checkDelay = urand(5 * MINUTE * IN_MILLISECONDS, 10 * MINUTE * IN_MILLISECONDS);
			break;
		}
		}
	}
}
