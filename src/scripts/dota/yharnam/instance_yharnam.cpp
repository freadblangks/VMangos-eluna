#include "scriptPCH.h"
#include "yharnam.h"

struct instance_yharnam : public ScriptedInstance
{
    instance_yharnam(Map* pMap) : ScriptedInstance(pMap)
    {
        Initialize();
    };
};

InstanceData* GetInstanceData_instance_yharnam(Map* pMap)
{
    return new instance_yharnam(pMap);
}

void AddSC_instance_yharnam()
{
    Script* newscript;
    newscript = new Script;
    newscript->Name = "instance_yharnam";
    newscript->GetInstanceData = &GetInstanceData_instance_yharnam;
    newscript->RegisterSelf();
}