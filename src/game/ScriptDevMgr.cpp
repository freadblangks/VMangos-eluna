#include "Common.h"
#include "Policies/SingletonImp.h"
#include "Config/Config.h"
#include "Database/DatabaseEnv.h"
#include "DBCStores.h"
#include "ObjectMgr.h"
#include "ProgressBar.h"
#include "SystemConfig.h"
#include "ScriptDevMgr.h"
#include "ace/Singleton.h"
#include "CreatureGroups.h"

#ifdef BUILD_SCRIPTDEV
#include "system/ScriptLoader.h"
#endif

INSTANTIATE_SINGLETON_1(ScriptDevMgr);

ScriptDevMgr::ScriptDevMgr()
{
}

ScriptDevMgr::~ScriptDevMgr()
{
#define SCR_CLEAR(T) \
        FOR_SCRIPTS(T, itr, end) \
            delete itr->second; \
        SCR_REG_LST(T).clear();

    // Clear scripts for every script type.
    /*SCR_CLEAR(SpellHandlerScript);
    SCR_CLEAR(AuraHandlerScript);
    SCR_CLEAR(ServerScript);
    SCR_CLEAR(WorldScript);
    SCR_CLEAR(GroupScript);
    SCR_CLEAR(FormulaScript);
    SCR_CLEAR(WorldMapScript);
    SCR_CLEAR(InstanceMapScript);
    SCR_CLEAR(BattlegroundMapScript);
    SCR_CLEAR(ItemScript);
    SCR_CLEAR(GameObjectScript);
    SCR_CLEAR(AreaTriggerScript);
    SCR_CLEAR(BattlegroundScript);
    SCR_CLEAR(OutdoorPvPScript);
    SCR_CLEAR(CommandScript);
    SCR_CLEAR(WeatherScript);
    SCR_CLEAR(AuctionHouseScript);
    SCR_CLEAR(ConditionScript);
    SCR_CLEAR(DynamicObjectScript);
    SCR_CLEAR(TransportScript);*/
    SCR_CLEAR(ModuleScript);
    SCR_CLEAR(CreatureScript);
    SCR_CLEAR(PlayerScript);
    SCR_CLEAR(AllCreatureScript);


#undef SCR_CLEAR
}


/* #############################################
   #                ModuleScript
   #
   ############################################# */


ModuleScript::ModuleScript(const char* name)
    : ScriptObject(name)
{
    ScriptDevMgr::ScriptRegistry<ModuleScript>::AddScript(this);
}

/* #############################################
   #                AllCreatuteScript
   #
   ############################################# */

void ScriptDevMgr::OnAllCreatureUpdate(Creature* creature, uint32 diff)
{
    ASSERT(creature);

    FOREACH_SCRIPT(AllCreatureScript)->OnAllCreatureUpdate(creature, diff);
}

void ScriptDevMgr::OnInitStatsForLevel(Creature* creature)
{
    ASSERT(creature);

    FOREACH_SCRIPT(AllCreatureScript)->OnInitStatsForLevel(creature);
}

AllCreatureScript::AllCreatureScript(const char* name)
    : ScriptObject(name)
{
    ScriptDevMgr::ScriptRegistry<AllCreatureScript>::AddScript(this);
}

/* #############################################
   #                UnitScript
   #
   ############################################# */

uint32 ScriptDevMgr::DealDamage(Unit* AttackerUnit, Unit* pVictim, uint32 damage, DamageEffectType damagetype)
{
    FOR_SCRIPTS_RET(UnitScript, itr, end, damage)
        damage = itr->second->DealDamage(AttackerUnit, pVictim, damage, damagetype);
    return damage;
}

void ScriptDevMgr::OnDamage(Unit* attacker, Unit* victim, uint32& damage)
{
    FOREACH_SCRIPT(UnitScript)->OnDamage(attacker, victim, damage);
}

void ScriptDevMgr::OnHeal(Unit* healer, Unit* reciever, uint32& gain)
{
    FOREACH_SCRIPT(UnitScript)->OnHeal(healer, reciever, gain);
}

UnitScript::UnitScript(const char* name)
    : ScriptObject(name)
{
    ScriptDevMgr::ScriptRegistry<UnitScript>::AddScript(this);
}

/* #############################################
   #                CreatureScript
   #
   ############################################# */

bool ScriptDevMgr::OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
{
    ASSERT(player);
    ASSERT(creature);
    ASSERT(quest);


    GET_SCRIPT_RET(CreatureScript, creature->GetScriptId(), tmpscript, false);
    player->PlayerTalkClass->ClearMenus();
    return tmpscript->OnQuestAccept(player, creature, quest);
}



bool ScriptDevMgr::OnQuestReward(Player* player, Creature* creature, Quest const* quest)
{
    ASSERT(player);
    ASSERT(creature);
    ASSERT(quest);

    GET_SCRIPT_RET(CreatureScript, creature->GetScriptId(), tmpscript, false);
    player->PlayerTalkClass->ClearMenus();
    return tmpscript->OnQuestReward(player, creature, quest);
}

uint32 ScriptDevMgr::GetDialogStatus(Player* player, Creature* creature)
{
    ASSERT(player);
    ASSERT(creature);

    // TODO: 100 is a funny magic number to have hanging around here...
    GET_SCRIPT_RET(CreatureScript, creature->GetScriptId(), tmpscript, 100);
    player->PlayerTalkClass->ClearMenus();
    return tmpscript->OnDialogStatus(player, creature);
}

CreatureAI* ScriptDevMgr::GetCreatureAI(Creature* creature)
{
    ASSERT(creature);

    GET_SCRIPT_RET(CreatureScript, creature->GetScriptId(), tmpscript, NULL);
    return tmpscript->GetAI(creature);
}

bool ScriptDevMgr::OnGossipHello(Player* player, Creature* creature)
{
    ASSERT(player);
    ASSERT(creature);

    GET_SCRIPT_RET(CreatureScript, creature->GetScriptId(), tmpscript, false);
    return tmpscript->OnGossipHello(player, creature);
}

bool ScriptDevMgr::OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code)
{
    ASSERT(player);
    ASSERT(creature);

    GET_SCRIPT_RET(CreatureScript, creature->GetScriptId(), tmpscript, false);
    
    if (code)
    {
        player->PlayerTalkClass->ClearMenus();
        return tmpscript->OnGossipSelectCode(player, creature, sender, action, code);
    }
    else
    {
        player->PlayerTalkClass->ClearMenus();
        return tmpscript->OnGossipSelect(player, creature, sender, action);
    }
    
}

CreatureScript::CreatureScript(const char* name)
    : ScriptObject(name)
{
    ScriptDevMgr::ScriptRegistry<CreatureScript>::AddScript(this);
}


/* #############################################
   #                PlayerScripts
   #
   ############################################# */

void ScriptDevMgr::OnPlayerUseItem(Player* player, Item* item, SpellCastTargets const& targets)
{
    FOREACH_SCRIPT(PlayerScript)->OnPlayerUseItem(player, item, targets);
}

uint32 ScriptDevMgr::MaxPrimaryTradeSkill(Player* player, uint32& maxSkillsAllowed)
{
    FOR_SCRIPTS_RET(PlayerScript, itr, end, maxSkillsAllowed)
        maxSkillsAllowed = itr->second->UpdateCraftingSkillAmount(player, maxSkillsAllowed);
    return maxSkillsAllowed;
}

uint32 ScriptDevMgr::UpdateCraftingSkillAmount(Player* player, uint32& UpdateAmount)
{
    FOR_SCRIPTS_RET(PlayerScript, itr, end, UpdateAmount)
        UpdateAmount = itr->second->UpdateCraftingSkillAmount(player,UpdateAmount);
    return UpdateAmount;
}

int32 ScriptDevMgr::RewardReputationAmount(Player* player, int32& rep)
{
    FOR_SCRIPTS_RET(PlayerScript, itr, end, rep)
        rep = itr->second->RewardReputationAmount(player, rep);
    return rep;
}

uint32 ScriptDevMgr::UpdateGatheringSkillAmount(Player* player, uint32& UpdateAmount)
{
    FOR_SCRIPTS_RET(PlayerScript, itr, end, UpdateAmount)
        UpdateAmount = itr->second->UpdateGatheringSkillAmount(player, UpdateAmount);
    return UpdateAmount;
}

bool ScriptDevMgr::OnPlayerHandleTaxi(Player* player, uint32 sourcepath)
{
    CHECK_RETURN_BOOL(PlayerScript, false); // Check if the script list is empty and return false if it is.

    bool result = false;
    FOR_SCRIPTS_RET(PlayerScript, itr, end, result)
    {
        if (itr->second->OnPlayerHandleTaxi(player, sourcepath))
        {
            result = true; // Set the result to true if any script returns true.
        }
    }

    return result; // Return the final result.
}

void ScriptDevMgr::OnBeforePlayerUpdate(Player* player, uint32 p_time)
{
    FOREACH_SCRIPT(PlayerScript)->OnBeforeUpdate(player, p_time);
}


void ScriptDevMgr::OnLootMoney(Player* player, uint32 amount)
{
    FOREACH_SCRIPT(PlayerScript)->OnLootMoney(player, amount);
}

void ScriptDevMgr::OnPlayerLogoutRequest(Player* player)
{
    FOREACH_SCRIPT(PlayerScript)->OnPlayerLogoutRequest(player);
}

void ScriptDevMgr::OnPVPKill(Player* killer, Player* killed)
{
    FOREACH_SCRIPT(PlayerScript)->OnPVPKill(killer, killed);
}

void ScriptDevMgr::OnQuestComplete(Player* player, Quest const* quest)
{
    player->PlayerTalkClass->ClearMenus();
    FOREACH_SCRIPT(PlayerScript)->OnQuestComplete(player, quest);
}

void ScriptDevMgr::OnCreatureKill(Player* killer, Creature* killed)
{
    FOREACH_SCRIPT(PlayerScript)->OnCreatureKill(killer, killed);
}

void ScriptDevMgr::OnPlayerKilledByCreature(Creature* killer, Player* killed, bool& durabilityLoss)
{
    FOREACH_SCRIPT(PlayerScript)->OnPlayerKilledByCreature(killer, killed, durabilityLoss);
}

void ScriptDevMgr::OnPlayerLogin(Player* player)
{
    FOREACH_SCRIPT(PlayerScript)->OnLogin(player);
}

void ScriptDevMgr::OnPlayerLogout(Player* player)
{
    FOREACH_SCRIPT(PlayerScript)->OnLogout(player);
}

void ScriptDevMgr::OnPlayerLevelChanged(Player* player, uint8 oldLevel, uint8 newLevel)
{
    FOREACH_SCRIPT(PlayerScript)->OnLevelChanged(player, oldLevel, newLevel);
}

void ScriptDevMgr::OnGivePlayerXP(Player* player, uint32& amount, Unit* victim)
{
    FOREACH_SCRIPT(PlayerScript)->OnGiveXP(player, amount, victim);
}

void ScriptDevMgr::OnLootItem(Player* player, Item* item, uint32 count, uint64 lootGUID)
{
    FOREACH_SCRIPT(PlayerScript)->OnLootItem(player, item, count, lootGUID);
}

void ScriptDevMgr::OnCreateItem(Player* player, Item* item, uint32 count)
{
    FOREACH_SCRIPT(PlayerScript)->OnCreateItem(player, item, count);
}

void ScriptDevMgr::OnQuestRewardItem(Player* player, Item* item, uint32 count)
{
    FOREACH_SCRIPT(PlayerScript)->OnQuestRewardItem(player, item, count);
}

bool ScriptDevMgr::OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg)
{
    CHECK_RETURN_BOOL(PlayerScript, false); // Check if the script list is empty and return false if it is.

    bool result = false;
    FOR_SCRIPTS_RET(PlayerScript, itr, end, result)
    {
        if (itr->second->OnChat(player, type, lang, msg))
        {
            result = true; // Set the result to true if any script returns true.
        }
    }

    return result; // Return the final result.
}

void ScriptDevMgr::OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg, Player* receiver)
{
    FOREACH_SCRIPT(PlayerScript)->OnChat(player, type, lang, msg, receiver);
}

void ScriptDevMgr::OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg, Group* group)
{
    FOREACH_SCRIPT(PlayerScript)->OnChat(player, type, lang, msg, group);
}

void ScriptDevMgr::OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg, Guild* guild)
{
    FOREACH_SCRIPT(PlayerScript)->OnChat(player, type, lang, msg, guild);
}

void ScriptDevMgr::OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg, Channel* channel)
{
    FOREACH_SCRIPT(PlayerScript)->OnChat(player, type, lang, msg, channel);
}

void ScriptDevMgr::OnPlayerTalentsReset(Player* player, bool noCost)
{
    FOREACH_SCRIPT(PlayerScript)->OnTalentsReset(player, noCost);
}


void ScriptDevMgr::OnMapChanged(Player* player)
{
    FOREACH_SCRIPT(PlayerScript)->OnMapChanged(player);
}


PlayerScript::PlayerScript(const char* name)
    : ScriptObject(name)
{
    ScriptDevMgr::ScriptRegistry<PlayerScript>::AddScript(this);
}


template<class TScript>
void ScriptDevMgr::ScriptRegistry<TScript>::AddScript(TScript* const script)
{
    ASSERT(script);

    // See if the script is using the same memory as another script. If this happens, it means that
    // someone forgot to allocate new memory for a script.
    typedef typename ScriptMap::iterator ScriptMapIterator;
    for (ScriptMapIterator it = ScriptPointerList.begin(); it != ScriptPointerList.end(); ++it)
    {
        if (it->second == script)
        {
            sLog.Out(LOG_PERFORMANCE, LOG_LVL_MINIMAL, "Script '%s' forgot to allocate memory, so this script and/or the script before that can't work.",
                script->ToString());

            return;
        }
    }
    // Get an ID for the script. An ID only exists if it's a script that is assigned in the database
    // through a script name (or similar).
    uint32 id = sScriptMgr.GetScriptId(script->ToString());
    if (id)
    {
        // Try to find an existing script.
        bool existing = false;
        typedef typename ScriptMap::iterator ScriptMapIterator;
        for (ScriptMapIterator it = ScriptPointerList.begin(); it != ScriptPointerList.end(); ++it)
        {
            // If the script names match...
            if (it->second->GetName() == script->GetName())
            {
                // ... It exists.
                existing = true;
                break;
            }
        }

        // If the script isn't assigned -> assign it!
        if (!existing)
        {
            ScriptPointerList[id] = script;
        }
        else
        {
            // If the script is already assigned -> delete it!
            sLog.Out(LOG_DBERROR, LOG_LVL_MINIMAL, "Script '%s' already assigned with the same script name, so the script can't work.",
                script->ToString());

            delete script;
        }
    }
    else if (script->IsDatabaseBound())
    {
        // The script uses a script name from database, but isn't assigned to anything.
        if (script->GetName().find("example") == std::string::npos)
            sLog.Out(LOG_DBERROR, LOG_LVL_MINIMAL, "Script named '%s' does not have a script name assigned in database.",
                script->ToString());

        delete script;
    }
    else
    {
        // We're dealing with a code-only script; just add it.
        ScriptPointerList[_scriptIdCounter++] = script;
    }
}

// Instantiate static members of ScriptMgr::ScriptRegistry.
template<class TScript> std::map<uint32, TScript*> ScriptDevMgr::ScriptRegistry<TScript>::ScriptPointerList;
template<class TScript> uint32 ScriptDevMgr::ScriptRegistry<TScript>::_scriptIdCounter;

// Specialize for each script type class like so:
/*template class ScriptDevMgr::ScriptRegistry<SpellHandlerScript>;
template class ScriptDevMgr::ScriptRegistry<AuraHandlerScript>;
template class ScriptDevMgr::ScriptRegistry<ServerScript>;
template class ScriptDevMgr::ScriptRegistry<WorldScript>;*/
template class ScriptDevMgr::ScriptRegistry<PlayerScript>;
template class ScriptDevMgr::ScriptRegistry<CreatureScript>;
template class ScriptDevMgr::ScriptRegistry<UnitScript>;
template class ScriptDevMgr::ScriptRegistry<AllCreatureScript>;
template class ScriptDevMgr::ScriptRegistry<ModuleScript>;
/*template class ScriptDevMgr::ScriptRegistry<GroupScript>;
template class ScriptDevMgr::ScriptRegistry<FormulaScript>;
template class ScriptDevMgr::ScriptRegistry<WorldMapScript>;
template class ScriptDevMgr::ScriptRegistry<InstanceMapScript>;
template class ScriptDevMgr::ScriptRegistry<BattlegroundMapScript>;
template class ScriptDevMgr::ScriptRegistry<ItemScript>;
template class ScriptDevMgr::ScriptRegistry<GameObjectScript>;
template class ScriptDevMgr::ScriptRegistry<AreaTriggerScript>;
template class ScriptDevMgr::ScriptRegistry<BattlegroundScript>;
template class ScriptDevMgr::ScriptRegistry<OutdoorPvPScript>;
template class ScriptDevMgr::ScriptRegistry<WeatherScript>;
template class ScriptDevMgr::ScriptRegistry<AuctionHouseScript>;
template class ScriptDevMgr::ScriptRegistry<ConditionScript>;
template class ScriptDevMgr::ScriptRegistry<DynamicObjectScript>;
template class ScriptDevMgr::ScriptRegistry<TransportScript>;
template class ScriptDevMgr::ScriptRegistry<MovementHandlerScript>;
template class ScriptDevMgr::ScriptRegistry<BGScript>; */


// Undefine utility macros.
#undef GET_SCRIPT_RET
#undef GET_SCRIPT
#undef FOREACH_SCRIPT
#undef FOR_SCRIPTS_RET
#undef FOR_SCRIPTS
#undef SCR_REG_LST
#undef SCR_REG_MAP