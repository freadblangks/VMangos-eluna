#ifndef MING_MANAGER_H
#define MING_MANAGER_H

#ifndef MING_OWNER_ACCOUNT_NAME
#define MING_OWNER_ACCOUNT_NAME "MING"
#endif

#ifndef MING_OWNER_CHARACTER_NAME
#define MING_OWNER_CHARACTER_NAME "ming"
#endif

#include <string>
#include "Log.h"
#include "AuctionHouse/AuctionHouseMgr.h"
#include "Item.h"

#include "MingConfig.h"

class MingManager
{
	MingManager();
	MingManager(MingManager const&) = delete;
	MingManager& operator=(MingManager const&) = delete;
	~MingManager() = default;

public:
	void InitializeManager();
	void Clean();
	bool UpdateMing(uint32 pmDiff);

	bool StringEndWith(const std::string& str, const std::string& tail);
	bool StringStartWith(const std::string& str, const std::string& head);
	std::vector<std::string> SplitString(std::string srcStr, std::string delimStr, bool repeatedCharIgnored);
	std::string TrimString(std::string srcStr);

	static MingManager* instance();

private:
	bool UpdateSeller();
	bool UpdateBuyer();
	void ResetSellableItems();

public:
	std::unordered_set<uint32> vendorUnlimitItemSet;

	int32 buyerCheckDelay;
	int32 sellerCheckDelay;

	/// <summary>
	/// class, subclass, inventory, level, entry set
	/// </summary>
	std::unordered_map < uint32, std::unordered_map<uint32, std::unordered_map<uint32, std::unordered_map<uint32, std::unordered_set<uint32>>>>> equipsMap;

private:
	std::unordered_set<uint32> exceptionEntrySet;
	std::unordered_map<uint32, uint32> sellableItemIDMap;
	bool selling;
	uint32 sellingIndex;
	std::unordered_map<uint32, uint32> sellingItemIDMap;
	std::set<uint32> auctionHouseIDSet;

};

#define sMingManager MingManager::instance()

#endif