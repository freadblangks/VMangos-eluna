#ifndef NIER_ACTION_WARLOCK_H
#define NIER_ACTION_WARLOCK_H

#include "NierAction_Base.h"

enum WarlockCurseType :uint32
{
    WarlockCurseType_None = 0,
    WarlockCurseType_Element = 1,
    WarlockCurseType_Weakness = 2,
    WarlockCurseType_Tongues = 3,
};

class NierAction_Warlock :public NierAction_Base
{
public:
    NierAction_Warlock(Player* pmMe);

    uint32 curseType;
    int curseDelay;
    int manaCheckDelay;
    int soulstoneDelay;
    int soulLinkDelay;
    int felArmorDelay;
    int wardDelay;
    int summonDelay;
    int soulHarvestDelay;
    int soulburnDelay;
    int soulFireDelay;
    int soulshatterDelay;
    int shadowfuryDelay;
    int conflagrateDelay;
    int immolateDelay;
    int shadowburnDelay;
    int chaosBoltDelay;
    int baneOfHavocDelay;
    int empoweredImpDelay;
};
#endif
