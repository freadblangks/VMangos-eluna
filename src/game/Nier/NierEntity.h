#ifndef NIER_ENTITY_H
#define NIER_ENTITY_H

enum NierEntityState :uint32
{
    NierEntityState_None = 0,
    NierEntityState_OffLine,
    NierEntityState_Enter,
    NierEntityState_CheckAccount,
    NierEntityState_CreateAccount,
    NierEntityState_CheckCharacter,
    NierEntityState_CreateCharacter,    
    NierEntityState_CheckLogin,
    NierEntityState_DoLogin,
    NierEntityState_Initialize,
    NierEntityState_Online,
    NierEntityState_Exit,
    NierEntityState_CheckLogoff,
    NierEntityState_DoLogoff,
};

class NierEntity
{
public:
    NierEntity(uint32 pmNierID);
    void Update(uint32 pmDiff);

public:
    uint32 nier_id;
    uint32 account_id;
    std::string account_name;
    uint32 character_id;
    uint32 target_level;
    int checkDelay;
    uint32 entityState;
};
#endif
