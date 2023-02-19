#include "NierAction_Paladin.h"

NierAction_Paladin::NierAction_Paladin(Player* pmMe) :NierAction_Base(pmMe)
{
    blessingType = PaladinBlessingType::PaladinBlessingType_Might;
    auraType = PaladinAuraType::PaladinAuraType_Retribution;
    sealType = PaladinSealType::PaladinSealType_Righteousness;

    judgementDelay = 0;
    hammerOfJusticeDelay = 0;
    sealDelay = 0;
}
