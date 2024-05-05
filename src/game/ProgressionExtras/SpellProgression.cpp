
#include "SpellProgression.h"
#include "SpellAuras.h"
#include "Spell.h"

SpellCastResult Spell::ProgCheckCast(SpellCastResult result, bool strict)
{
    if (!m_caster)
        return result;

    if (Unit* target = m_targets.getUnitTarget())
    {
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_11_2
        // World of Warcraft Client Patch 1.12.0 (2006-08-22)
        // - Rip: Lesser potency Rips will no longer overwrite greater potency ones.
        if (m_spellInfo->IsFitToFamily<SPELLFAMILY_DRUID, CF_DRUID_RIP_BITE>() && m_spellInfo->SpellIconID == 108)
        {
            if (Aura* aura = target->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, uint64(1) << CF_DRUID_RIP_BITE, m_caster->GetObjectGuid()))
            {
                float newStr = m_caster->CalculateSpellEffectValue(target, m_spellInfo, EFFECT_INDEX_0);
                // $AP * min(0.06*$cp, 0.24)/6 [Yes, there is no difference, whether 4 or 5 CPs are being used]
                if (Player* caster = m_caster->ToPlayer())
                {
                    uint8 cp = caster->GetComboPoints();
                    if (cp > 4) cp = 4;
                    newStr += caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100;
                }
                newStr = m_caster->MeleeDamageBonusDone(target, newStr, m_spellInfo->GetWeaponAttackType(), m_spellInfo, EFFECT_INDEX_0, DOT, aura->GetStackAmount());
                if (aura->GetModifier()->m_amount > newStr)
                    return SPELL_FAILED_AURA_BOUNCED;
            }
        }
#endif
    }

    return result;
}
