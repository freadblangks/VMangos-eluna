#ifndef NIER_ACTION_PALADIN_H
#define NIER_ACTION_PALADIN_H

#include "NierAction_Base.h"

enum PaladinAuraType :uint32
{
    PaladinAuraType_Concentration = 0,
    PaladinAuraType_Devotion,
    PaladinAuraType_Retribution,
    PaladinAuraType_FireResistant,
    PaladinAuraType_FrostResistant,
    PaladinAuraType_ShadowResistant,
};

enum PaladinBlessingType :uint32
{
    PaladinBlessingType_Kings = 0,
    PaladinBlessingType_Might = 1,
    PaladinBlessingType_Wisdom = 2,
    PaladinBlessingType_Salvation = 3,
};

enum PaladinSealType :uint32
{
    PaladinSealType_Righteousness = 0,
    PaladinSealType_Justice = 1,
    PaladinSealType_Crusader = 2,
};

class NierAction_Paladin :public NierAction_Base
{
public:
    NierAction_Paladin(Player* pmMe);
    void InitializeCharacter(uint32 pmTargetLevel, uint32 pmSpecialtyTabIndex);
    void ResetTalent();
    bool InitializeEquipments(bool pmReset);
    void Prepare();
    void Update(uint32 pmDiff);
    bool Tank(Unit* pmTarget, bool aoe = false);
    bool Attack(Unit* pmTarget);
    bool Interrupt(Unit* pmTarget);
    bool Revive(Player* pmTarget);
    bool DPS(Unit* pmTarget, bool pmRushing, bool pmChasing, float pmDistanceMax = DEFAULT_COMBAT_REACH, float pmDistanceMin = CONTACT_DISTANCE);
    bool Buff(Unit* pmTarget);
    uint32 Caution();

public:
    uint32 spell_ConcentrationAura;
    uint32 spell_FrostResistanceAura;
    uint32 spell_RetributionAura;
    uint32 spell_ShadowResistanceAura;
    uint32 spell_FireResistanceAura;
    uint32 spell_DevotionAura;
    uint32 spell_BlessingOfWisdom;
    uint32 spell_BlessingOfSalvation;
    uint32 spell_BlessingOfMight;
    uint32 spell_SealOfJustice;
    uint32 spell_SealOfRighteousness;
    uint32 spell_SealOfTheCrusader;
    uint32 spell_Judgement;
    uint32 spell_HammerOfJustice;
    uint32 spell_Exorcism;
    uint32 spell_LayOnHands;
    uint32 spell_DivineIntervention;
    uint32 spell_Consecration;
    uint32 spell_Blessing_Of_Sanctuary;
    uint32 spell_HolyShield;
    uint32 spell_RighteousFury;

    uint32 spell_Redemption;

    uint32 auraType;
    uint32 blessingType;
    uint32 sealType;

    int judgementDelay;
    int hammerOfJusticeDelay;
    int sealDelay;
    int exorcismDelay;
    int layOnHandsDelay;
    int interventionDelay;
    int consecrationDelay;
    int holyShieldDelay;
};
#endif
