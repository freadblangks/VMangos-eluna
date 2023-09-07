#include "ScriptDevMgr.h"
#include "World.h"

class modsample : public PlayerScript
{
public:
    modsample() : PlayerScript("modsample") {}

    void OnLogin(Player* player)
    {
        if (!sWorld.GetModuleBoolConfig("module_sample", false))
            return;

        ChatHandler(player).PSendSysMessage("Hook works fine!");
    }

    void OnGiveXP(Player* player, uint32& xp, Unit* victim)
    {
        if (sWorld.GetModuleBoolConfig("module_sample", false))
            return;

        ChatHandler(player).PSendSysMessage("You just recieved %u xp", xp);
    }
};

void Addmod_sampleScripts()
{
    new modsample();
}