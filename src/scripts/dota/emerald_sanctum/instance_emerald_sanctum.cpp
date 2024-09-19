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
    uint64 m_uiWarlockGUID;
    uint32 m_uiSpawnWarlockOnDragonKnightsDeath;
    uint32 m_uiSpawnChestOnWarlockDeath;
    uint32 m_uiDragonKnightDeathCount;
    bool   m_isDragonKnightGreenDead;
    bool   m_isDragonKnightRedDead;
    bool   m_isDragonKnightBlueDead;
    bool   m_isWarlockDead;    

    void Initialize() override
    {
        m_uiDragonKnightGreenGUID = 0;
        m_uiDragonKnightRedGUID = 0;
        m_uiDragonKnightBlueGUID = 0;
        m_uiWarlockGUID = 0;
        m_uiSpawnWarlockOnDragonKnightsDeath = 5000;
        m_uiSpawnChestOnWarlockDeath = 10000;
        m_uiDragonKnightDeathCount = 0;
        m_isDragonKnightGreenDead = false;
        m_isDragonKnightRedDead = false;
        m_isDragonKnightBlueDead = false;
        m_isWarlockDead = false;
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
            case NPC_WARLOCK:
                m_uiWarlockGUID = pCreature->GetGUID();
                break;
        }
    }

    void OnCreatureDeath(Creature *who) override
    {
        switch (who->GetEntry())
        {
            case NPC_DRAGON_KNIGHT_GREEN :
                m_isDragonKnightGreenDead = true;
                m_uiSpawnWarlockOnDragonKnightsDeath = 5000;
                m_uiDragonKnightDeathCount += 1;
                break;
            case NPC_DRAGON_KNIGHT_RED :
                m_isDragonKnightRedDead = true;
                m_uiSpawnWarlockOnDragonKnightsDeath = 5000;
                m_uiDragonKnightDeathCount += 1;
                break;
            case NPC_DRAGON_KNIGHT_BLUE :
                m_isDragonKnightBlueDead = true;
                m_uiSpawnWarlockOnDragonKnightsDeath = 5000;
                m_uiDragonKnightDeathCount += 1;
                break;
            case NPC_WARLOCK :
                m_isWarlockDead = true;
                m_uiSpawnChestOnWarlockDeath = 10000;
                break;
        }
    }

    uint32 GetData(uint32 uiType) override
    {
        if (uiType == TYPE_DRAGON_KNIGHT_DEATH_COUNT)
            return m_uiDragonKnightDeathCount;
        return 0;
    }

    void Update(uint32 uiDiff) override
    {
        //spawn warlock
        if (m_isDragonKnightGreenDead && m_isDragonKnightRedDead && m_isDragonKnightBlueDead && m_uiSpawnWarlockOnDragonKnightsDeath)
        {
            if (m_uiSpawnWarlockOnDragonKnightsDeath <= uiDiff)
            {
                if (Creature* pDragonKnightGreen = instance->GetCreature(m_uiDragonKnightGreenGUID))
                {
                    pDragonKnightGreen->SummonCreature(NPC_WARLOCK, 3319.990f, 3057.540f, 23.333f, 3.172990f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 120 * MINUTE * IN_MILLISECONDS);
                    m_uiSpawnWarlockOnDragonKnightsDeath = 0;
                }
            }
            else
                m_uiSpawnWarlockOnDragonKnightsDeath -= uiDiff;
        }
        //spawn chest
        if (m_isWarlockDead && m_uiSpawnChestOnWarlockDeath)
        {
            if (m_uiSpawnChestOnWarlockDeath <= uiDiff)
            {
                if (Creature* pWarlock = instance->GetCreature(m_uiWarlockGUID))
                {
                    pWarlock->SummonGameObject(GO_CHEST, 2762.25f, 2972.77f, 26.903f, 0.0f, 0, 0, 0, 0, 43200);
                    m_uiSpawnChestOnWarlockDeath = 0;
                }
            }
            else
                m_uiSpawnChestOnWarlockDeath -= uiDiff;
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