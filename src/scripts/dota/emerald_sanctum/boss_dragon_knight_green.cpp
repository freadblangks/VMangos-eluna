#include "scriptPCH.h"
#include "emerald_sanctum.h"

enum
{
    SAY_DEATH_GREEN         = -2000002,
    SAY_DEATH_RED           = -2000005,
    SAY_DEATH_BLUE          = -2000008,
    SAY_AGGRO_GREEN         = -2000000,
    SAY_SHAPESHIFTING_GREEN = -2000001,
    SAY_BKB                 = -2000009,
    SPELL_BKB               = 34210,
    SPELL_SLAM              = 34211,
    SPELL_BREATH_GREEN      = 34212,
    SPELL_AOE_GREEN         = 34213,
    MODEL_DRAGON_GREEN      = 7553,
    MODEL_HUMAN_GREEN       = 1860,
};

struct boss_dragon_knight_greenAI : public ScriptedAI
{
    boss_dragon_knight_greenAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 SLAM_TIMER;
    uint32 BREATH_GREEN_TIMER;
    uint32 AOE_GREEN_TIMER;
    bool shapeshifting_green;
    bool bkb_green;
    bool bkb_again_green;
    bool kill;

    void Reset() override
    {
        SLAM_TIMER = 9000;
        BREATH_GREEN_TIMER = 12000;
        AOE_GREEN_TIMER = 10000;
        m_creature->LoadEquipment(m_creature->GetCreatureInfo()->equipment_id, true);
        m_creature->SetDisplayId(MODEL_HUMAN_GREEN);
        shapeshifting_green = false;
        bkb_green = false;
        bkb_again_green = false;
        kill = false;
    }

    void JustDied(Unit* Killer) override
    {
        m_creature->LoadEquipment(m_creature->GetCreatureInfo()->equipment_id, true);
        m_creature->SetDisplayId(MODEL_HUMAN_GREEN);
    }

    void Aggro(Unit* pWho) override
    {
        DoScriptText(SAY_AGGRO_GREEN, m_creature);
    }

    void UpdateAI(uint32 const diff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_creature->GetHealthPercent() < 75.0f && !shapeshifting_green)
        {
            m_creature->SetVirtualItem(BASE_ATTACK, 0);
            m_creature->SetVirtualItem(OFF_ATTACK, 0);
            m_creature->SetVirtualItem(RANGED_ATTACK, 0);
            m_creature->SetDisplayId(MODEL_DRAGON_GREEN);
            DoScriptText(SAY_SHAPESHIFTING_GREEN, m_creature);
            shapeshifting_green = true;
        }

        if (m_creature->GetHealthPercent() < 50.0f && !bkb_green)
        {
            DoCastSpellIfCan(m_creature, SPELL_BKB);
            DoScriptText(SAY_BKB, m_creature);
            bkb_green = true;
        }

        if (m_creature->GetHealthPercent() < 25.0f && !bkb_again_green)
        {
            DoCastSpellIfCan(m_creature, SPELL_BKB);
            DoScriptText(SAY_BKB, m_creature);
            bkb_again_green = true;
        }

        if (m_creature->GetHealthPercent() < 10.0f && !kill)
        {
            if (m_pInstance->GetData(TYPE_DRAGON_KNIGHT_DEATH_COUNT) == 0)
                DoScriptText(SAY_DEATH_GREEN, m_creature);
            else if (m_pInstance->GetData(TYPE_DRAGON_KNIGHT_DEATH_COUNT) == 1)
                DoScriptText(SAY_DEATH_RED, m_creature);
            else if (m_pInstance->GetData(TYPE_DRAGON_KNIGHT_DEATH_COUNT) == 2)
                DoScriptText(SAY_DEATH_BLUE, m_creature);
            kill = true;
        }

        //SLAM
        if (SLAM_TIMER < diff)
        {
            DoCastSpellIfCan(m_creature->GetVictim(), SPELL_SLAM);
            if(shapeshifting_green)
                SLAM_TIMER = urand(6000,8000);
            else
                SLAM_TIMER = urand(9000,12000);
        }
        else SLAM_TIMER -= diff;

        //BREATH_GREEN
        if (BREATH_GREEN_TIMER < diff)
        {
            DoCastSpellIfCan(m_creature->GetVictim(), SPELL_BREATH_GREEN);
            if(shapeshifting_green)
                BREATH_GREEN_TIMER = urand(8000,10500);
            else
                BREATH_GREEN_TIMER = urand(12000,16000);
        }
        else BREATH_GREEN_TIMER -= diff;
        
        //AOE_GREEN
        if (shapeshifting_green)
        {
            if (AOE_GREEN_TIMER < diff)
            {
                DoCastSpellIfCan(m_creature->GetVictim(), SPELL_AOE_GREEN);
                AOE_GREEN_TIMER = urand(10000,13500);
            }
            else AOE_GREEN_TIMER -= diff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_dragon_knight_green(Creature* pCreature)
{

    return new boss_dragon_knight_greenAI(pCreature);
}

void AddSC_boss_dragon_knight_green()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_dragon_knight_green";
    pNewScript->GetAI = &GetAI_boss_dragon_knight_green;
    pNewScript->RegisterSelf();
}
