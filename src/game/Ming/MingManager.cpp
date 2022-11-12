#include "MingManager.h"
#include "AuctionHouseMgr.h"
#include "AccountMgr.h"

MingManager::MingManager()
{
	mingAccountId = 0;
	mingCharacterId = 0;
	sellingIndex = 0;
	selling = false;
	buyerCheckDelay = 0;
	sellerCheckDelay = 0;
	auctionHouseIDSet.clear();
	vendorUnlimitItemSet.clear();
	sellingItemIDMap.clear();
	sellableItemIDMap.clear();
	exceptionEntrySet.clear();
	equipsMap.clear();
}

MingManager* MingManager::instance()
{
	static MingManager instance;
	return &instance;
}

void MingManager::InitializeManager()
{
	sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Initialize ming manager");

	buyerCheckDelay = TimeConstants::HOUR * TimeConstants::IN_MILLISECONDS;
	sellerCheckDelay = 1 * TimeConstants::MINUTE * TimeConstants::IN_MILLISECONDS;

	auctionHouseIDSet.clear();
	auctionHouseIDSet.insert(2);
	//auctionHouseIDSet.insert(1);
	//auctionHouseIDSet.insert(6);
	//auctionHouseIDSet.insert(7);

	vendorUnlimitItemSet.clear();
	QueryResult* vendorItemQR = WorldDatabase.Query("SELECT distinct item FROM npc_vendor where maxcount = 0");
	if (vendorItemQR)
	{
		do
		{
			Field* fields = vendorItemQR->Fetch();
			uint32 eachItemEntry = fields[0].GetUInt32();
			vendorUnlimitItemSet.insert(eachItemEntry);
		} while (vendorItemQR->NextRow());
		delete vendorItemQR;
	}
	exceptionEntrySet.clear();
	exceptionEntrySet.insert(24358);
	exceptionEntrySet.insert(5558);
	exceptionEntrySet.insert(20370);
	exceptionEntrySet.insert(20372);

	sellingItemIDMap.clear();
	ResetSellableItems();
	sellingIndex = 0;
	selling = false;

	if (sMingConfig.Reset > 0)
	{
		std::ostringstream sqlAccountStream;
		sqlAccountStream << "SELECT id FROM account where username = '" << MING_OWNER_ACCOUNT_NAME << "'";
		std::string sqlAccount = sqlAccountStream.str();
		QueryResult* qrAccount = LoginDatabase.Query(sqlAccount.c_str());

		if (qrAccount)
		{
			do
			{
				Field* fields = qrAccount->Fetch();
				mingAccountId = fields[0].GetUInt32();
			} while (qrAccount->NextRow());
		}
		delete qrAccount;
		if (mingAccountId > 0)
		{
			sAccountMgr.DeleteAccount(mingAccountId);
			mingAccountId = 0;
		}
	}
}

bool MingManager::EnsureOwner()
{
	if (mingAccountId == 0)
	{
		std::ostringstream sqlAccountStream;
		sqlAccountStream << "SELECT id FROM account where username = '" << MING_OWNER_ACCOUNT_NAME << "'";
		std::string sqlAccount = sqlAccountStream.str();
		QueryResult* qrAccount = LoginDatabase.Query(sqlAccount.c_str());

		if (qrAccount)
		{
			do
			{
				Field* fields = qrAccount->Fetch();
				mingAccountId = fields[0].GetUInt32();
			} while (qrAccount->NextRow());
		}
		delete qrAccount;
		if (mingAccountId == 0)
		{
			AccountOpResult aor = sAccountMgr.CreateAccount(MING_OWNER_ACCOUNT_NAME, MING_OWNER_ACCOUNT_NAME);
			if (aor != AccountOpResult::AOR_OK)
			{
				sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Ming account error");
				sMingConfig.Enable = false;
				return false;
			}
			qrAccount = LoginDatabase.Query(sqlAccount.c_str());
			if (qrAccount)
			{
				do
				{
					Field* fields = qrAccount->Fetch();
					mingAccountId = fields[0].GetUInt32();
				} while (qrAccount->NextRow());
			}
			delete qrAccount;
			if (mingAccountId == 0)
			{
				sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Ming account error");
				sMingConfig.Enable = false;
				return false;
			}
		}
	}
	if (mingCharacterId == 0)
	{
		std::ostringstream sqlCharacterStream;
		sqlCharacterStream << "SELECT guid FROM characters where account = " << mingAccountId << " and name = '" << MING_OWNER_CHARACTER_NAME << "'";
		std::string sqlCharacter = sqlCharacterStream.str();
		QueryResult* qrCharacter = CharacterDatabase.Query(sqlCharacter.c_str());

		if (qrCharacter)
		{
			do
			{
				Field* fields = qrCharacter->Fetch();
				mingCharacterId = fields[0].GetUInt32();
			} while (qrCharacter->NextRow());
		}
		delete qrCharacter;
		if (mingCharacterId > 0)
		{
			sNierManager->LoginNier(mingAccountId, mingCharacterId);
		}
		else
		{
			WorldSession* eachSession = new WorldSession(mingAccountId, NULL, SEC_PLAYER, 0, LOCALE_enUS);
			uint32 const guidLow = sObjectMgr.GeneratePlayerLowGuid();
			if (!Player::SaveNewPlayer(eachSession, guidLow, MING_OWNER_CHARACTER_NAME, Races::RACE_HUMAN, Classes::CLASS_MAGE, Gender::GENDER_FEMALE, 0, 0, 0, 0, 0))
			{
				delete eachSession;
				sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Character create failed, %s", MING_OWNER_CHARACTER_NAME);
				sMingConfig.Enable = false;
				return false;
			}
			qrCharacter = CharacterDatabase.Query(sqlCharacter.c_str());
			if (qrCharacter)
			{
				do
				{
					Field* fields = qrCharacter->Fetch();
					mingCharacterId = fields[0].GetUInt32();
				} while (qrCharacter->NextRow());
			}
			delete qrCharacter;
			if (mingCharacterId == 0)
			{
				sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Character create failed, %s", MING_OWNER_CHARACTER_NAME);
				sMingConfig.Enable = false;
				return false;
			}
			sNierManager->LoginNier(mingAccountId, mingCharacterId);
		}
	}

	return true;
}

void MingManager::Clean()
{
	if (!EnsureOwner())
	{
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Ming account error");
		sMingConfig.Enable = false;
		return;
	}
	sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Ready to clean ming");

	for (std::set<uint32>::iterator ahIDIT = auctionHouseIDSet.begin(); ahIDIT != auctionHouseIDSet.end(); ahIDIT++)
	{
		uint32 ahID = *ahIDIT;
		AuctionHouseEntry const* ahEntry = sAuctionHouseStore.LookupEntry(*ahIDIT);
		AuctionHouseObject* aho = sAuctionMgr.GetAuctionsMap(ahEntry);
		if (!aho)
		{
			sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "AuctionHouseObject is null");
			return;
		}
		std::set<uint32> auctionIDSet;
		auctionIDSet.clear();
		std::map<uint32, AuctionEntry*>* aem = aho->GetAuctions();
		for (std::map<uint32, AuctionEntry*>::iterator aeIT = aem->begin(); aeIT != aem->end(); aeIT++)
		{
			auctionIDSet.insert(aeIT->first);
		}
		for (std::set<uint32>::iterator auctionIDIT = auctionIDSet.begin(); auctionIDIT != auctionIDSet.end(); auctionIDIT++)
		{
			AuctionEntry* eachAE = aho->GetAuction(*auctionIDIT);
			if (eachAE)
			{
				if (eachAE->owner == mingCharacterId)
				{
					eachAE->DeleteFromDB();
					sAuctionMgr.RemoveAItem(eachAE->itemGuidLow);
					aho->RemoveAuction(eachAE);
					sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Auction %d removed for auctionhouse %d", eachAE->itemTemplate, ahID);
					delete eachAE;
					eachAE = nullptr;
				}
			}
		}
	}

	sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Ming cleaned");
}

bool MingManager::UpdateMing(uint32 pmDiff)
{
	if (!sMingConfig.Enable)
	{
		return false;
	}
	if (!EnsureOwner())
	{
		sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Ming account error");
		sMingConfig.Enable = false;
		return false;
	}
	sellerCheckDelay -= pmDiff;
	if (sellerCheckDelay < 0)
	{
		UpdateSeller();
	}
	buyerCheckDelay -= pmDiff;
	if (buyerCheckDelay < 0)
	{
		UpdateBuyer();
	}

	return true;
}

bool MingManager::UpdateSeller()
{
	if (sellingItemIDMap.empty())
	{
		int maxCount = 100;
		if (maxCount > sellableItemIDMap.size())
		{
			maxCount = sellableItemIDMap.size();
		}
		sellingIndex = 0;
		while (sellingItemIDMap.size() < maxCount)
		{
			uint32 toSellItemID = urand(0, sellableItemIDMap.size() - 1);
			toSellItemID = sellableItemIDMap[toSellItemID];
			if (sellingItemIDMap.find(toSellItemID) == sellingItemIDMap.end())
			{
				sellingItemIDMap[sellingItemIDMap.size()] = toSellItemID;
			}
		}
	}
	if (sellingIndex < sellingItemIDMap.size())
	{
		int itemEntry = sellingItemIDMap[sellingIndex];
		const ItemPrototype* proto = sObjectMgr.GetItemPrototype(itemEntry);
		if (proto)
		{
			for (std::set<uint32>::iterator ahIDIT = auctionHouseIDSet.begin(); ahIDIT != auctionHouseIDSet.end(); ahIDIT++)
			{
				uint32 ahID = *ahIDIT;
				AuctionHouseEntry const* ahEntry = sAuctionHouseStore.LookupEntry(*ahIDIT);
				AuctionHouseObject* aho = sAuctionMgr.GetAuctionsMap(ahEntry);
				if (!aho)
				{
					sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "AuctionHouseObject is null");
					return false;
				}
				uint32 stackCount = urand(1, proto->Stackable);
				uint32 priceMultiple = urand(10, 15);
				Item* item = Item::CreateItem(proto->ItemId, stackCount, mingCharacterId);
				if (item)
				{
					if (uint32 randomPropertyId = Item::GenerateItemRandomPropertyId(itemEntry))
					{
						item->SetItemRandomProperties(randomPropertyId);
					}
					uint32 finalPrice = 0;
					finalPrice = proto->SellPrice * stackCount * priceMultiple;
					if (finalPrice == 0)
					{
						finalPrice = proto->BuyPrice * stackCount * priceMultiple / 4;
					}
					if (finalPrice == 0)
					{
						break;
					}
					if (finalPrice > 100)
					{
						uint32 dep = sAuctionMgr.GetAuctionDeposit(ahEntry, 2 * TimeConstants::HOUR, item);

						AuctionEntry* auctionEntry = new AuctionEntry;
						auctionEntry->Id = sObjectMgr.GenerateAuctionID();
						auctionEntry->auctionHouseEntry = ahEntry;
						auctionEntry->itemGuidLow = item->GetGUIDLow();
						auctionEntry->itemTemplate = item->GetEntry();
						auctionEntry->owner = mingCharacterId;
						auctionEntry->startbid = finalPrice / 2;
						auctionEntry->buyout = finalPrice;
						auctionEntry->bidder = 0;
						auctionEntry->bid = 0;
						auctionEntry->deposit = dep;
						auctionEntry->depositTime = time(nullptr);
						auctionEntry->expireTime = (time_t)(1 * TimeConstants::HOUR) + time(nullptr);
						item->SaveToDB();
						sAuctionMgr.AddAItem(item);
						aho->AddAuction(auctionEntry);
						auctionEntry->SaveToDB();

						sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Auction %s added for auctionhouse %d", proto->Name1, ahID);
					}
				}
			}
		}
		sellingIndex++;
		sellerCheckDelay = 1 * IN_MILLISECONDS;
	}
	else
	{
		ResetSellableItems();
		sellingIndex = 0;
		sellingItemIDMap.clear();
		sellerCheckDelay = 1 * TimeConstants::HOUR * IN_MILLISECONDS;
	}

	return false;
}

bool MingManager::UpdateBuyer()
{
	buyerCheckDelay = HOUR * IN_MILLISECONDS;

	sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Ready to update ming buyer");

	std::set<uint32> toBuyAuctionIDSet;
	for (std::set<uint32>::iterator ahIDIT = auctionHouseIDSet.begin(); ahIDIT != auctionHouseIDSet.end(); ahIDIT++)
	{
		uint32 ahID = *ahIDIT;
		AuctionHouseEntry const* ahEntry = sAuctionHouseStore.LookupEntry(*ahIDIT);
		AuctionHouseObject* aho = sAuctionMgr.GetAuctionsMap(ahEntry);
		if (!aho)
		{
			sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "AuctionHouseObject is null");
			return false;
		}
		toBuyAuctionIDSet.clear();
		std::map<uint32, AuctionEntry*>* aem = aho->GetAuctions();
		for (std::map<uint32, AuctionEntry*>::iterator aeIT = aem->begin(); aeIT != aem->end(); aeIT++)
		{
			Item* checkItem = sAuctionMgr.GetAItem(aeIT->second->itemGuidLow);
			if (!checkItem)
			{
				continue;
			}
			if (aeIT->second->owner == 0)
			{
				continue;
			}
			const ItemPrototype* destIT = sObjectMgr.GetItemPrototype(aeIT->second->itemTemplate);
			if (!destIT)
			{
				continue;
			}
			if (destIT->Quality < 1)
			{
				continue;
			}
			if (destIT->Quality > 4)
			{
				continue;
			}
			uint32 basePrice = destIT->SellPrice;
			if (basePrice == 0)
			{
				basePrice = destIT->BuyPrice / 4;
			}
			if (basePrice == 0)
			{
				continue;
			}
			float buyRate = sMingConfig.BuyRate;
			if (vendorUnlimitItemSet.find(aeIT->second->itemTemplate) != vendorUnlimitItemSet.end())
			{
				buyRate = buyRate / 2.0f;
			}
			float priceRate = (float)aeIT->second->buyout / (float)basePrice;
			buyRate = buyRate / priceRate;
			float buyPower = frand(0.0f, 10000.0f);
			if (buyPower < buyRate)
			{
				toBuyAuctionIDSet.insert(aeIT->first);
			}
		}

		for (std::set<uint32>::iterator toBuyIT = toBuyAuctionIDSet.begin(); toBuyIT != toBuyAuctionIDSet.end(); toBuyIT++)
		{
			AuctionEntry* destAE = aho->GetAuction(*toBuyIT);
			if (destAE)
			{
				destAE->bid = destAE->buyout;

				sAuctionMgr.SendAuctionSuccessfulMail(destAE);
				sAuctionMgr.SendAuctionWonMail(destAE);
				sAuctionMgr.RemoveAItem(destAE->itemGuidLow);
				aho->RemoveAuction(destAE);
				destAE->DeleteFromDB();
				delete destAE;
				sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Auction %d was bought by ming buyer", *toBuyIT);
			}
		}
	}

	sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Ming buyer updated");
	return true;
}

void MingManager::ResetSellableItems()
{
	sellableItemIDMap.clear();
	for (const auto& item : sObjectMgr.GetItemPrototypeMap())
	{
		ItemPrototype const* proto = &item.second;
		if (!proto)
		{
			continue;
		}
		if (vendorUnlimitItemSet.find(proto->ItemId) != vendorUnlimitItemSet.end())
		{
			continue;
		}
		if (exceptionEntrySet.find(proto->ItemId) != exceptionEntrySet.end())
		{
			continue;
		}
		if (proto->ItemLevel < 1)
		{
			continue;
		}
		if (proto->Quality < 1)
		{
			continue;
		}
		if (proto->Quality > 4)
		{
			continue;
		}
		if (proto->Bonding == ItemBondingType::BIND_WHEN_PICKED_UP)
		{
			continue;
		}
		bool sellThis = false;
		switch (proto->Class)
		{
		case ItemClass::ITEM_CLASS_CONSUMABLE:
		{
			if (urand(0, 100) < 5)
			{
				sellThis = true;
			}
			break;
		}
		case ItemClass::ITEM_CLASS_CONTAINER:
		{
			if (proto->Quality >= 2)
			{
				if (urand(0, 100) < 10)
				{
					sellThis = true;
				}
			}
			break;
		}
		case ItemClass::ITEM_CLASS_WEAPON:
		{
			if (proto->Quality >= 2)
			{
				if (urand(0, 100) < 5)
				{
					sellThis = true;
				}
			}
			break;
		}
		case ItemClass::ITEM_CLASS_GEM:
		{
			sellThis = true;
			break;
		}
		case ItemClass::ITEM_CLASS_ARMOR:
		{
			if (proto->Quality >= 2)
			{
				if (urand(0, 100) < 5)
				{
					sellThis = true;
				}
			}
			break;
		}
		case ItemClass::ITEM_CLASS_REAGENT:
		{
			if (urand(0, 100) < 5)
			{
				sellThis = true;
			}
			break;
		}
		case ItemClass::ITEM_CLASS_PROJECTILE:
		{
			if (proto->Quality >= 2)
			{
				if (urand(0, 100) < 10)
				{
					sellThis = true;
				}
			}
			break;
		}
		case ItemClass::ITEM_CLASS_TRADE_GOODS:
		{
			if (urand(0, 100) < 20)
			{
				sellThis = true;
			}
			break;
		}
		case ItemClass::ITEM_CLASS_GENERIC:
		{
			if (urand(0, 100) < 5)
			{
				sellThis = true;
			}
			break;
		}
		case ItemClass::ITEM_CLASS_RECIPE:
		{
			if (urand(0, 100) < 5)
			{
				sellThis = true;
			}
			break;
		}
		case ItemClass::ITEM_CLASS_MONEY:
		{
			break;
		}
		case ItemClass::ITEM_CLASS_QUIVER:
		{
			if (proto->Quality >= 2)
			{
				if (urand(0, 100) < 10)
				{
					sellThis = true;
				}
			}
			break;
		}
		case ItemClass::ITEM_CLASS_QUEST:
		{
			sellThis = true;
			break;
		}
		case ItemClass::ITEM_CLASS_KEY:
		{
			break;
		}
		case ItemClass::ITEM_CLASS_PERMANENT:
		{
			break;
		}
		case ItemClass::ITEM_CLASS_JUNK:
		{
			if (proto->Quality > 0)
			{
				if (urand(0, 100) < 5)
				{
					sellThis = true;
				}
			}
			break;
		}
		default:
		{
			break;
		}
		}
		if (sellThis)
		{
			sellableItemIDMap[sellableItemIDMap.size()] = proto->ItemId;
		}
	}
}