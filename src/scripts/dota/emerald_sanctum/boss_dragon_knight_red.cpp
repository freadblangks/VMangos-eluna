#include "scriptPCH.h"
#include "emerald_sanctum.h"

enum
{
    SAY_AGGRO_RED           = -2000003,
    SAY_SHAPESHIFTING_RED   = -2000004,
    SAY_DEATH_RED           = -2000005,
    SAY_BKB                 = -2000009,
    SPELL_BKB               = 34210,
    SPELL_SLAM              = 34211,
    SPELL_BREATH_RED        = 34214,
    SPELL_AOE_RED           = 34215,
    MODEL_DRAGON_RED        = 12821,
    MODEL_HUMAN_RED         = 4137,
};

struct boss_dragon_knight_redAI : public ScriptedAI
{
    boss_dragon_knight_redAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 SLAM_TIMER;
    uint32 BREATH_RED_TIMER;
    uint32 AOE_RED_TIMER;
    bool shapeshifting_red;
    bool bkb_red;
    bool bkb_again_red;

    void Reset() override
    {
        SLAM_TIMER = 9000;
        BREATH_RED_TIMER = 12000;
        AOE_RED_TIMER = 10000;
        m_creature->LoadEquipment(m_creature->GetCreatureInfo()->equipment_id, true);
        m_creature->SetDisplayId(MODEL_HUMAN_RED);
        shapeshifting_red = false;
        bkb_red = false;
        bkb_again_red = false;
    }

    void JustDied(Unit* Killer) override
    {
        m_creature->LoadEquipment(m_creature->GetCreatureInfo()->equipment_id, true);
        m_creature->SetDisplayId(MODEL_HUMAN_RED);
        DoScriptText(SAY_DEATH_RED, Killer);
    }

    void Aggro(Unit* pWho) override
    {
        DoScriptText(SAY_AGGRO_RED, m_creature);
    }

    void UpdateAI(uint32 const diff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_creature->GetHealthPercent() < 75.0f && !shapeshifting_red)
        {
            m_creature->SetVirtualItem(BASE_ATTACK, 0);
            m_creature->SetVirtualItem(OFF_ATTACK, 0);
            m_creature->SetVirtualItem(RANGED_ATTACK, 0);
            m_creature->SetDisplayId(MODEL_DRAGON_RED);
            DoScriptText(SAY_SHAPESHIFTING_RED, m_creature);
            shapeshifting_red = true;
        }

        if (m_creature->GetHealthPercent() < 50.0f && !bkb_red)
        {
            DoCastSpellIfCan(m_creature, SPELL_BKB);
            DoScriptText(SAY_BKB, m_creature);
            bkb_red = true;
        }

        if (m_creature->GetHealthPercent() < 25.0f && !bkb_again_red)
        {
            DoCastSpellIfCan(m_creature, SPELL_BKB);
            DoScriptText(SAY_BKB, m_creature);
            bkb_again_red = true;
        }

        //SLAM
        if (SLAM_TIMER < diff)
        {
            DoCastSpellIfCan(m_creature->GetVictim(), SPELL_SLAM);
            if(shapeshifting_red)
                SLAM_TIMER = urand(6000,8000);
            else
                SLAM_TIMER = urand(9000,12000);
        }
        else SLAM_TIMER -= diff;

        //BREATH_RED
        if (BREATH_RED_TIMER < diff)
        {
            DoCastSpellIfCan(m_creature->GetVictim(), SPELL_BREATH_RED);
            if(shapeshifting_red)
                BREATH_RED_TIMER = urand(8000,10500);
            else
                BREATH_RED_TIMER = urand(12000,16000);
        }
        else BREATH_RED_TIMER -= diff;
        
        //AOE_RED
        if (shapeshifting_red)
        {
            if (AOE_RED_TIMER < diff)
            {
                DoCastSpellIfCan(m_creature->GetVictim(), SPELL_AOE_RED);
                AOE_RED_TIMER = urand(10000,13500);
            }
            else AOE_RED_TIMER -= diff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_dragon_knight_red(Creature* pCreature)
{

    return new boss_dragon_knight_redAI(pCreature);
}

void AddSC_boss_dragon_knight_red()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_dragon_knight_red";
    pNewScript->GetAI = &GetAI_boss_dragon_knight_red;
    pNewScript->RegisterSelf();
}
