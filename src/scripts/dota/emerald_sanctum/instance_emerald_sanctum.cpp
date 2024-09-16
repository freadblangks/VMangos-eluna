#include "scriptPCH.h"
#include "emerald_sanctum.h"

struct instance_emerald_sanctum : public ScriptedInstance
{
    instance_emerald_sanctum(Map* pMap) : ScriptedInstance(pMap)
    {
        Initialize();
    };

    uint64 m_uiDragonKnightGreenGUID;
    uint64 m_uiDragonKnightRedGUID;
    uint64 m_uiDragonKnightBlueGUID;
    uint32 m_uiSpawnChestOnDragonKnightsDeath;
    bool   m_isDragonKnightGreenDead;
    bool   m_isDragonKnightRedDead;
    bool   m_isDragonKnightBlueDead;

    void Initialize() override
    {
        m_uiDragonKnightGreenGUID = 0;
        m_uiDragonKnightRedGUID = 0;
        m_uiDragonKnightBlueGUID = 0;
        m_uiSpawnChestOnDragonKnightsDeath = 10000;
        m_isDragonKnightGreenDead = false;
        m_isDragonKnightRedDead = false;
        m_isDragonKnightBlueDead = false;
    }

    void OnCreatureCreate(Creature* pCreature) override
    {
        switch (pCreature->GetEntry())
        {
            case NPC_DRAGON_KNIGHT_GREEN:
                m_uiDragonKnightGreenGUID = pCreature->GetGUID();
                break;
            case NPC_DRAGON_KNIGHT_RED:
                m_uiDragonKnightRedGUID = pCreature->GetGUID();
                break;
            case NPC_DRAGON_KNIGHT_BLUE:
                m_uiDragonKnightBlueGUID = pCreature->GetGUID();
                break;
        }
    }

    void OnCreatureDeath(Creature *who) override
    {
        switch (who->GetEntry())
        {
            case NPC_DRAGON_KNIGHT_GREEN :
                m_isDragonKnightGreenDead = true;
                m_uiSpawnChestOnDragonKnightsDeath = 10000;
                break;
            case NPC_DRAGON_KNIGHT_RED :
                m_isDragonKnightRedDead = true;
                m_uiSpawnChestOnDragonKnightsDeath = 10000;
                break;
            case NPC_DRAGON_KNIGHT_BLUE :
                m_isDragonKnightBlueDead = true;
                m_uiSpawnChestOnDragonKnightsDeath = 10000;
                break;
        }
    }
    void Update(uint32 uiDiff) override
    {
        if (m_isDragonKnightGreenDead && m_isDragonKnightRedDead && m_isDragonKnightBlueDead && m_uiSpawnChestOnDragonKnightsDeath)
        {
            if (m_uiSpawnChestOnDragonKnightsDeath <= uiDiff)
            {
                if (Creature* pDragonKnightGreen = instance->GetCreature(m_uiDragonKnightGreenGUID))
                {
                    pDragonKnightGreen->SummonGameObject(GO_CHEST, 2762.25f, 2972.77f, 26.903f, 0.0f, 0, 0, 0, 0, 43200);
                    m_uiSpawnChestOnDragonKnightsDeath = 0;
                }
            }
            else
                m_uiSpawnChestOnDragonKnightsDeath -= uiDiff;
        }
    }
};

InstanceData* GetInstanceData_instance_emerald_sanctum(Map* pMap)
{
    return new instance_emerald_sanctum(pMap);
}

void AddSC_instance_emerald_sanctum()
{
    Script* newscript;
    newscript = new Script;
    newscript->Name = "instance_emerald_sanctum";
    newscript->GetInstanceData = &GetInstanceData_instance_emerald_sanctum;
    newscript->RegisterSelf();
}