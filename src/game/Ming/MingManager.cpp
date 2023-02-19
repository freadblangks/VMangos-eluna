#include "MingManager.h"
#include "AuctionHouseMgr.h"
#include "AccountMgr.h"

MingManager::MingManager()
{
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
	auctionHouseIDSet.insert(1);
	auctionHouseIDSet.insert(2);
	auctionHouseIDSet.insert(3);

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
				uint32 mingAccountId = fields[0].GetUInt32();
				sAccountMgr.DeleteAccount(mingAccountId);
			} while (qrAccount->NextRow());
		}
		delete qrAccount;
	}
}

void MingManager::Clean()
{
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
				if (eachAE->owner == 0)
				{
					eachAE->expireTime = time(nullptr) - 1 * TimeConstants::MINUTE;
					sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "Auction %d removed for auctionhouse %d", eachAE->itemTemplate, ahID);					
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
		Clean();
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
				const ItemPrototype* proto = sObjectMgr.GetItemPrototype(toSellItemID);
				if (proto)
				{
					if (proto->SellPrice > 0 || proto->BuyPrice > 0)
					{
						sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "%s added to to sell items", proto->Name1);
						sellingItemIDMap[sellingItemIDMap.size()] = toSellItemID;
					}
				}
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
				Item* item = Item::CreateItem(proto->ItemId, stackCount, 0);
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
						//auctionEntry->owner = mingCharacterId;
						auctionEntry->owner = 0;
						auctionEntry->startbid = finalPrice / 2;
						auctionEntry->buyout = finalPrice;
						auctionEntry->bidder = 0;
						auctionEntry->bid = 0;
						auctionEntry->deposit = dep;
						auctionEntry->depositTime = time(nullptr);
						auctionEntry->expireTime = (time_t)(4 * TimeConstants::HOUR) + time(nullptr);
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
		//sellerCheckDelay = 1 * TimeConstants::MINUTE * IN_MILLISECONDS;
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

bool MingManager::StringEndWith(const std::string& str, const std::string& tail)
{
	return str.compare(str.size() - tail.size(), tail.size(), tail) == 0;
}

bool MingManager::StringStartWith(const std::string& str, const std::string& head)
{
	return str.compare(0, head.size(), head) == 0;
}

std::vector<std::string> MingManager::SplitString(std::string srcStr, std::string delimStr, bool repeatedCharIgnored)
{
	std::vector<std::string> resultStringVector;
	std::replace_if(srcStr.begin(), srcStr.end(), [&](const char& c) {if (delimStr.find(c) != std::string::npos) { return true; } else { return false; }}, delimStr.at(0));
	size_t pos = srcStr.find(delimStr.at(0));
	std::string addedString = "";
	while (pos != std::string::npos) {
		addedString = srcStr.substr(0, pos);
		if (!addedString.empty() || !repeatedCharIgnored) {
			resultStringVector.push_back(addedString);
		}
		srcStr.erase(srcStr.begin(), srcStr.begin() + pos + 1);
		pos = srcStr.find(delimStr.at(0));
	}
	addedString = srcStr;
	if (!addedString.empty() || !repeatedCharIgnored) {
		resultStringVector.push_back(addedString);
	}
	return resultStringVector;
}

std::string MingManager::TrimString(std::string srcStr)
{
	std::string result = srcStr;
	if (!result.empty())
	{
		result.erase(0, result.find_first_not_of(" "));
		result.erase(result.find_last_not_of(" ") + 1);
	}

	return result;
}
