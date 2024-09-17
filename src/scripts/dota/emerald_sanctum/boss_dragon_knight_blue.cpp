#include "scriptPCH.h"
#include "emerald_sanctum.h"

enum
{
    SAY_AGGRO_BLUE          = -2000006,
    SAY_SHAPESHIFTING_BLUE  = -2000007,
    SAY_DEATH_BLUE          = -2000008,
    SAY_BKB                 = -2000009,
    SPELL_BKB               = 34210,
    SPELL_SLAM              = 34211,
    SPELL_BREATH_BLUE       = 34216,
    SPELL_AOE_BLUE          = 34217,
    MODEL_DRAGON_BLUE       = 6373,
    MODEL_HUMAN_BLUE        = 3167,
};

struct boss_dragon_knight_blueAI : public ScriptedAI
{
    boss_dragon_knight_blueAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 SLAM_TIMER;
    uint32 BREATH_BLUE_TIMER;
    uint32 AOE_BLUE_TIMER;
    bool shapeshifting_blue;
    bool bkb_blue;
    bool bkb_again_blue;

    void Reset() override
    {
        SLAM_TIMER = 9000;
        BREATH_BLUE_TIMER = 12000;
        AOE_BLUE_TIMER = 10000;
        m_creature->LoadEquipment(m_creature->GetCreatureInfo()->equipment_id, true);
        m_creature->SetDisplayId(MODEL_HUMAN_BLUE);
        shapeshifting_blue = false;
        bkb_blue = false;
        bkb_again_blue = false;
    }

    void JustDied(Unit* Killer) override
    {
        m_creature->LoadEquipment(m_creature->GetCreatureInfo()->equipment_id, true);
        m_creature->SetDisplayId(MODEL_HUMAN_BLUE);
        DoScriptText(SAY_DEATH_BLUE, Killer);
    }

    void Aggro(Unit* pWho) override
    {
        DoScriptText(SAY_AGGRO_BLUE, m_creature);
    }

    void UpdateAI(uint32 const diff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_creature->GetHealthPercent() < 75.0f && !shapeshifting_blue)
        {
            m_creature->SetVirtualItem(BASE_ATTACK, 0);
            m_creature->SetVirtualItem(OFF_ATTACK, 0);
            m_creature->SetVirtualItem(RANGED_ATTACK, 0);
            m_creature->SetDisplayId(MODEL_DRAGON_BLUE);
            DoScriptText(SAY_SHAPESHIFTING_BLUE, m_creature);
            shapeshifting_blue = true;
        }

        if (m_creature->GetHealthPercent() < 50.0f && !bkb_blue)
        {
            DoCastSpellIfCan(m_creature, SPELL_BKB);
            DoScriptText(SAY_BKB, m_creature);
            bkb_blue = true;
        }

        if (m_creature->GetHealthPercent() < 25.0f && !bkb_again_blue)
        {
            DoCastSpellIfCan(m_creature, SPELL_BKB);
            DoScriptText(SAY_BKB, m_creature);
            bkb_again_blue = true;
        }

        //SLAM
        if (SLAM_TIMER < diff)
        {
            DoCastSpellIfCan(m_creature->GetVictim(), SPELL_SLAM);
            if(shapeshifting_blue)
                SLAM_TIMER = urand(6000,8000);
            else
                SLAM_TIMER = urand(9000,12000);
        }
        else SLAM_TIMER -= diff;

        //BREATH_BLUE
        if (BREATH_BLUE_TIMER < diff)
        {
            DoCastSpellIfCan(m_creature->GetVictim(), SPELL_BREATH_BLUE);
            if(shapeshifting_blue)
                BREATH_BLUE_TIMER = urand(8000,10500);
            else
                BREATH_BLUE_TIMER = urand(12000,16000);
        }
        else BREATH_BLUE_TIMER -= diff;
        
        //AOE_BLUE
        if (shapeshifting_blue)
        {
            if (AOE_BLUE_TIMER < diff)
            {
                DoCastSpellIfCan(m_creature->GetVictim(), SPELL_AOE_BLUE);
                AOE_BLUE_TIMER = urand(10000,13500);
            }
            else AOE_BLUE_TIMER -= diff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_dragon_knight_blue(Creature* pCreature)
{

    return new boss_dragon_knight_blueAI(pCreature);
}

void AddSC_boss_dragon_knight_blue()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_dragon_knight_blue";
    pNewScript->GetAI = &GetAI_boss_dragon_knight_blue;
    pNewScript->RegisterSelf();
}
