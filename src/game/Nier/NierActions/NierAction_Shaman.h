#ifndef NIER_ACTION_SHAMAN_H
#define NIER_ACTION_SHAMAN_H

#include "NierAction_Base.h"

enum ShamanEarthTotemType :uint32
{
    ShamanEarthTotemType_EarthbindTotem = 0,
    ShamanEarthTotemType_StoneskinTotem,
    ShamanEarthTotemType_StoneclawTotem,
    ShamanEarthTotemType_StrengthOfEarthTotem
};

class NierAction_Shaman :public NierAction_Base
{
public:
    NierAction_Shaman(Player* pmMe);
    void Prepare();

    uint32 earthTotemType;
};
#endif
