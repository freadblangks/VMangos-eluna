#ifndef SC_SCRIPTDEVMGR_H
#define SC_SCRIPTDEVMGR_H

#include "Common.h"
#include "BattleGround.h"
#include "Chat/Chat.h"
#include "Player.h"
#include "World.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "ScriptedInstance.h"

class Player;
class BattlegroundScript;
class Creature;
class CreatureAI;
class InstanceData;
class SpellScript;
class Quest;
class Item;
class Transport;
class GameObject;
class SpellCastTargets;
class Map;
class Unit;
class WorldObject;
struct ItemPrototype;
class Spell;
class ScriptMgr;
class WorldSocket;

// Utility macros to refer to the script registry.
#define SCR_REG_MAP(T) ScriptRegistry<T>::ScriptMap
#define SCR_REG_LST(T) ScriptRegistry<T>::ScriptPointerList

// Utility macros for looping over scripts.
#define FOR_SCRIPTS(T,C,E) \
    if (SCR_REG_LST(T).empty()) \
        return; \
    for (SCR_REG_MAP(T)::iterator C = SCR_REG_LST(T).begin(); \
        C != SCR_REG_LST(T).end(); ++C)
#define FOR_SCRIPTS_RET(T,C,E,R) \
    if (SCR_REG_LST(T).empty()) \
        return R; \
    for (SCR_REG_MAP(T)::iterator C = SCR_REG_LST(T).begin(); \
        C != SCR_REG_LST(T).end(); ++C)
#define FOREACH_SCRIPT(T) \
    FOR_SCRIPTS(T, itr, end) \
    itr->second
#define CHECK_RETURN_BOOL(T, R) \
    if (SCR_REG_LST(T).empty()) \
        return R; \
    for (SCR_REG_MAP(T)::iterator itr = SCR_REG_LST(T).begin(); \
        itr != SCR_REG_LST(T).end(); ++itr)

// Utility macros for finding specific scripts.
#define GET_SCRIPT(T,I,V) \
    T* V = ScriptRegistry<T>::GetScriptById(I); \
    if (!V) \
        return;
#define GET_SCRIPT_RET(T,I,V,R) \
    T* V = ScriptRegistry<T>::GetScriptById(I); \
    if (!V) \
        return R;


class ScriptObject
{
    friend class ScriptMgr;

public:

    // Called when the script is initialized. Use it to initialize any properties of the script.
    virtual void OnInitialize() { }

    // Called when the script is deleted. Use it to free memory, etc.
    virtual void OnTeardown() { }

    // Do not override this in scripts; it should be overridden by the various script type classes. It indicates
    // whether or not this script type must be assigned in the database.
    virtual bool IsDatabaseBound() const { return false; }

    const std::string& GetName() const { return _name; }

    const char* ToString() const { return _name.c_str(); }

protected:

    ScriptObject(const char* name)
        : _name(std::string(name))
    {
        // Allow the script to do startup routines.
        OnInitialize();
    }

    virtual ~ScriptObject()
    {
        // Allow the script to do cleanup routines.
        OnTeardown();
    }

private:

    const std::string _name;
};

template<class TObject> class UpdatableScript
{
protected:

    UpdatableScript()
    {
    }

public:

    virtual void OnUpdate(TObject* obj, uint32 diff) { }
};

/* #############################################
   #                AllCreatureScript
   #
   #############################################*/
class AllCreatureScript : public ScriptObject
{
protected:
    AllCreatureScript(const char* name);

public:
    // Called from End of Creature Update.
    virtual void OnAllCreatureUpdate(Creature* /*creature*/, uint32 /*diff*/) { }

    // called when InitStats For creature
    virtual void OnInitStatsForLevel(Creature* /*creature*/) { }
};

/* #############################################
   #                ModuleScript
   #
   #############################################*/

// this class can be used to be extended by Modules
// creating their own custom hooks inside module itself
class ModuleScript : public ScriptObject
{
protected:
    ModuleScript(const char* name);
};


/* #############################################
   #                UnitScript
   #
   #############################################*/

class UnitScript : public ScriptObject
{
protected:
    UnitScript(const char* name);
public:

    // Called when a unit deals damage to another unit
    virtual void OnDamage(Unit* /*attacker*/, Unit* /*victim*/, uint32& /*damage*/) { }

    // Called when a unit deals damage to another unit
    virtual uint32 DealDamage(Unit* /*AttackerUnit*/, Unit* /*pVictim*/, uint32 damage, DamageEffectType /*damagetype*/) { return damage; }

    // Called when a unit deals healing to another unit
    virtual void OnHeal(Unit* /*healer*/, Unit* /*reciever*/, uint32& /*gain*/) { }
};

/* #############################################
   #                CreatureScript
   #
   #############################################*/

class CreatureScript : public ScriptObject, public UpdatableScript<Creature>
{
protected:
    CreatureScript(const char* name);

public:

    bool IsDatabaseBound() const { return true; }

    // Called when a player accepts a quest from the creature.
    virtual bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) { return false; }

    // Called when a player selects a quest reward.
    virtual bool OnQuestReward(Player* player, Creature* creature, Quest const* quest) { return false; }

    // Called When a player Opens a gossip dialog with the creature
    virtual bool OnGossipHello(Player* player, Creature* creature) { return false; }

    // Called when a player selects a gossip item in the creature's gossip menu.
    virtual bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) { return false; }

    // Called when a player selects a gossip with a code in the creature's gossip menu.
    virtual bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code) { return false; }

    // Called when the dialog status between a player and the creature is requested.
    virtual uint32 OnDialogStatus(Player* player, Creature* creature) { return 0; }

    // Called when a CreatureAI object is needed for the creature.
    virtual CreatureAI* GetAI(Creature* creature) const { return NULL; }
};


/* #############################################
   #                PlayerScripts
   #
   #############################################*/

class PlayerScript : public ScriptObject
{
protected:
    PlayerScript(char const* name);

public:

    // Called when player want to use Taxi
    virtual bool OnPlayerHandleTaxi(Player* /*player*/, uint32 /*sourcepath*/) { return false; }

    // Called when a player gains XP (before anything is given)
    virtual void OnGiveXP(Player* /*player*/, uint32& /*amount*/, Unit* /*victim*/) { }

    // Called when a player logs in.
    virtual void OnLogin(Player* /*player*/) { }

    // Called when a player logs out.
    virtual void OnLogout(Player* /*player*/) { }

    // Called when Player Changed Level
    virtual void OnLevelChanged(Player* /*player*/, uint8 /*oldLevel*/, uint8 /*newLevel*/) { }

    // Called when a player kills another player
    virtual void OnPVPKill(Player* /*killer*/, Player* /*killed*/) { }

    // Called when a player kills a creature
    virtual void OnCreatureKill(Player* /*killer*/, Creature* /*killed*/) { }

    // Called when a player is killed by a creature
    virtual void OnPlayerKilledByCreature(Creature* /*killer*/, Player* /*killed*/, bool& /*durabilityLoss*/) { }

    // Called When a player Loots an item
    virtual void OnLootItem(Player* /*player*/, Item* /*item*/, uint32 /*count*/, uint64 /*itemGUID*/) { }

    // Called when player creates an item
    virtual void OnCreateItem(Player* /*player*/, Item* /*item*/, uint32 /*count*/) { }

    // Called when player recieves item from quest reward
    virtual void OnQuestRewardItem(Player* /*player*/, Item* /*item*/, uint32 /*count*/) { }

    // The following methods are called when a player sends a chat message.
    virtual bool OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, char*& /*msg*/) { return false; }
    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, char*& /*msg*/, Player* /*receiver*/) { }
    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, char*& /*msg*/, Group* /*group*/) { }
    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, char*& /*msg*/, Guild* /*guild*/) { }
    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, char*& /*msg*/, Channel* /*channel*/) { }

    // Called when a player's talent points are reset (right before the reset is done)
    virtual void OnTalentsReset(Player* /*player*/, bool /*noCost*/) { }

    // Called when a player completes a quest.
    virtual void OnQuestComplete(Player* player, Quest const* quest) {}

    // Called when player loots money
    virtual void OnLootMoney(Player* /*player*/, uint32 /*amount*/) { }

    // Called when player Requests to logout
    virtual void OnPlayerLogoutRequest(Player* player) {}

    // Called for player::update
    virtual void OnBeforeUpdate(Player* /*player*/, uint32 /*p_time*/) { }

    // Called when player uses an item
    virtual void OnPlayerUseItem(Player* /*player*/, Item* /*item*/, SpellCastTargets const& /*targets*/) { }

    // Called when a player changes to a new map (after moving to new map)
    virtual void OnMapChanged(Player* /*player*/) { }

    // Called when updating skill  for crafting
    virtual uint32 UpdateCraftingSkillAmount(Player* /*player*/, uint32& UpdateAmount) { return UpdateAmount; }

    // Called When updating gathering Skill
    virtual uint32 UpdateGatheringSkillAmount(Player* /*player*/, uint32& UpdateAmount) { return UpdateAmount; }

    // Called when giving player reputation
    virtual int32 RewardReputationAmount(Player* /*plater*/, int32& rep) { return rep; }

    // called for the amount of primary professions a player can have 
    virtual uint32 MaxPrimaryTradeSkill(Player* /*player*/, uint32& maxSkillsAllowed) { return maxSkillsAllowed; }

};



class ScriptDevMgr
{

    friend class ScriptDevAImgr;
    friend class ScriptObject;

public:
    ScriptDevMgr();
    virtual ~ScriptDevMgr();

public: /* PlayerScript */
    bool OnPlayerHandleTaxi(Player* player, uint32 sourcepath);
    void OnGivePlayerXP(Player* player, uint32& amount, Unit* victim);
    void OnPlayerLogin(Player* player);
    void OnPlayerLogout(Player* player);
    void OnPlayerLevelChanged(Player* player, uint8 oldLevel, uint8 newLevel);
    void OnPVPKill(Player* killer, Player* killed);
    void OnCreatureKill(Player* killer, Creature* killed);
    void OnPlayerKilledByCreature(Creature* killer, Player* killed, bool& durabilityLoss);
    void OnLootItem(Player* player, Item* item, uint32 count, uint64 lootGUID);
    void OnCreateItem(Player* player, Item* item, uint32 count);
    void OnQuestRewardItem(Player* player, Item* item, uint32 count);
    bool OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg, Player* receiver);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg, Group* group);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg, Guild* guild);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg, Channel* channel);
    void OnPlayerTalentsReset(Player* player, bool noCost);
    void OnQuestComplete(Player* player, Quest const* quest);
    void OnLootMoney(Player* player, uint32 amount);
    void OnPlayerLogoutRequest(Player* player);
    void OnBeforePlayerUpdate(Player* player, uint32 p_time);
    void OnPlayerUseItem(Player* player, Item* item, SpellCastTargets const& targets);
    void OnMapChanged(Player* player);
    void SendSpellCooldown(Player* player, uint32 spellId, uint32 cooldown, ObjectGuid target);
    uint32 UpdateCraftingSkillAmount(Player* player, uint32& UpdateAmount);
    uint32 UpdateGatheringSkillAmount(Player* player, uint32& UpdateAmount);
    int32 RewardReputationAmount(Player* player, int32& rep);
    uint32 MaxPrimaryTradeSkill(Player* player, uint32& maxSkillsAllowed);


public : /* CreatureScript */
    bool OnGossipHello(Player* player, Creature*);
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action);
    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code);
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code);
    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest);
    bool OnQuestReward(Player* player, Creature* creature, Quest const* quest);
    uint32 GetDialogStatus(Player* player, Creature* creature);
    CreatureAI* GetCreatureAI(Creature* creature);

public : /* UnitScript */
    uint32 DealDamage(Unit* AttackerUnit, Unit* pVictim, uint32 damage, DamageEffectType damagetype);
    void OnHeal(Unit* healer, Unit* reciever, uint32& gain);
    void OnDamage(Unit* attacker, Unit* victim, uint32& damage);

public: /* AllCreatureScript */
    void OnAllCreatureUpdate(Creature* creature, uint32 diff);
    void OnInitStatsForLevel(Creature* creature);


public: /* ScriptRegistry */

    // This is the global static registry of scripts.
    template<class TScript> class ScriptRegistry
    {
        // Counter used for code-only scripts.
        static uint32 _scriptIdCounter;

    public:

        typedef std::map<uint32, TScript*> ScriptMap;
        typedef typename ScriptMap::iterator ScriptMapIterator;
        // The actual list of scripts. This will be accessed concurrently, so it must not be modified
        // after server startup.
        static ScriptMap ScriptPointerList;

        // Gets a script by its ID (assigned by ObjectMgr).
        static TScript* GetScriptById(uint32 id)
        {
            for (ScriptMapIterator it = ScriptPointerList.begin(); it != ScriptPointerList.end(); ++it)
                if (it->first == id)
                    return it->second;

            return NULL;
        }

        // Attempts to add a new script to the list.
        static void AddScript(TScript* const script);
    };
};

#define sScriptDevMgr MaNGOS::Singleton<ScriptDevMgr>::Instance()

#endif