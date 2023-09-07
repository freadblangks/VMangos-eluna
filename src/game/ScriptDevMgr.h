#ifndef SC_SCRIPTDEVMGR_H
#define SC_SCRIPTDEVMGR_H

#include "Common.h"
#include "BattleGround.h"
#include "Chat/Chat.h"
#include "Player.h"
#include "World.h"
#include "Guild.h"
#include "GuildMgr.h"

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
   #                CreatureScript
   #
   #############################################*/

class CreatureScript : public ScriptObject, public UpdatableScript<Creature>
{
protected:
    CreatureScript(const char* name);

public:

    bool IsDatabaseBound() const { return true; }

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
    // Called when a player gains XP (before anything is given)
    virtual void OnGiveXP(Player* /*player*/, uint32& /*amount*/, Unit* /*victim*/) { }

    // Called when a player logs in.
    virtual void OnLogin(Player* /*player*/) { }

    // Called when Player Changed Level
    virtual void OnLevelChanged(Player* /*player*/, uint8 /*oldLevel*/, uint8 /*newLevel*/) { }

    // Called when a player kills another player
    virtual void OnPVPKill(Player* /*killer*/, Player* /*killed*/) { }

    // Called when a player kills a creature
    virtual void OnCreatureKill(Player* /*killer*/, Creature* /*killed*/) { }

    // Called when a player is killed by a creature
    virtual void OnPlayerKilledByCreature(Creature* /*killer*/, Player* /*killed*/) { }

    // Called When a player Loots an item
    virtual void OnLootItem(Player* /*player*/, Item* /*item*/, uint32 /*count*/, uint64 /*itemGUID*/) { }

    // Called when player creates an item
    virtual void OnCreateItem(Player* /*player*/, Item* /*item*/, uint32 /*count*/) { }

    // Called when player recieves item from quest reward
    virtual void OnQuestRewardItem(Player* /*player*/, Item* /*item*/, uint32 /*count*/) { }

    // The following methods are called when a player sends a chat message.
    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, char*& /*msg*/) { }
    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, char*& /*msg*/, Player* /*receiver*/) { }
    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, char*& /*msg*/, Group* /*group*/) { }
    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, char*& /*msg*/, Guild* /*guild*/) { }
    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, char*& /*msg*/, Channel* /*channel*/) { }
};



class ScriptDevMgr
{

    friend class ScriptDevAImgr;
    friend class ScriptObject;

public:
    ScriptDevMgr();
    virtual ~ScriptDevMgr();

public: /* PlayerScript */
    void OnGivePlayerXP(Player* player, uint32& amount, Unit* victim);
    void OnPlayerLogin(Player* player);
    void OnPlayerLevelChanged(Player* player, uint8 oldLevel, uint8 newLevel);
    void OnPVPKill(Player* killer, Player* killed);
    void OnCreatureKill(Player* killer, Creature* killed);
    void OnPlayerKilledByCreature(Creature* killer, Player* killed);
    void OnLootItem(Player* player, Item* item, uint32 count, uint64 lootGUID);
    void OnCreateItem(Player* player, Item* item, uint32 count);
    void OnQuestRewardItem(Player* player, Item* item, uint32 count);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg, Player* receiver);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg, Group* group);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg, Guild* guild);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, char*& msg, Channel* channel);

public : /* CreatureScript */
    bool OnGossipHello(Player* player, Creature*);
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action);
    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code);
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code);
    uint32 GetDialogStatus(Player* player, Creature* creature);
    CreatureAI* GetCreatureAI(Creature* creature);


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