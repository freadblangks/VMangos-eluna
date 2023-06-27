#ifndef GROUP_STRATEGY_H
#define GROUP_STRATEGY_H

#ifndef ENTRY_HUNGARFEN
#define ENTRY_HUNGARFEN 17770
#endif

#ifndef ENTRY_HUNGARFEN_1
#define ENTRY_HUNGARFEN_1 20169
#endif

#ifndef ENTRY_UNDERBOG_MUSHROOM
#define ENTRY_UNDERBOG_MUSHROOM 17990
#endif

#ifndef ENTRY_UNDERBOG_MUSHROOM_1
#define ENTRY_UNDERBOG_MUSHROOM_1 20189
#endif

#include "Nier/NierConfig.h"

class NierAction_Base;

class GroupStrategy_Base
{
public:
	GroupStrategy_Base();
	virtual void Reset();

	virtual void Update(uint32 pmDiff);

	Unit* GetNeareastOTUnit(Unit* pmTank);
	Unit* GetNeareastAttacker(Unit* pmTank);

public:
	Group* me;

	int interruptDelay;
};
#endif
