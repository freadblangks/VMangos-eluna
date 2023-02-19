#ifndef NIER_ACTION_ROGUE_H
#define NIER_ACTION_ROGUE_H

#include "NierAction_Base.h"

class NierAction_Rogue :public NierAction_Base
{
public:    
    NierAction_Rogue(Player* pmMe);
    void InitializeCharacter(uint32 pmTargetLevel, uint32 pmSpecialtyTabIndex);
    void ResetTalent();
    bool InitializeEquipments(bool pmReset);
    void Prepare();
    bool DPS(Unit* pmTarget, bool pmRushing, float pmDistanceMax, float pmDistanceMin, bool pmHolding, bool pmInstantOnly, bool pmChasing);
    bool Buff(Unit* pmTarget);
    uint32 Caution();

public:
    uint32 spell_Dismantle;
    uint32 spell_Backstab;
    uint32 spell_Eviscerate;
    uint32 spell_FanofKnives;
    uint32 spell_Sprint;
    uint32 spell_TricksoftheTrade;
    uint32 spell_Kick;
    uint32 spell_Sap;
    uint32 spell_Stealth;
    uint32 spell_SliceandDice;
    uint32 spell_Evasion;
    uint32 spell_KidneyShot;
    uint32 spell_CheapShot;
    uint32 spell_Vanish;
    uint32 spell_Feint;
    uint32 spell_SinisterStrike;
    uint32 spell_DeadlyThrow;

    uint32 spell_BladeFlurry;
    uint32 spell_AdrenalineRush;

    uint32 item_InstantPoison;
    uint32 item_SlowPoison;
};
#endif
