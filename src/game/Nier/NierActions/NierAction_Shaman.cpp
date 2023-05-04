#include "NierAction_Shaman.h"
#include "Group.h"
#include "Spell.h"
#include "Player.h"

NierAction_Shaman::NierAction_Shaman(Player* pmMe) :NierAction_Base(pmMe)
{
	earthTotemType = ShamanEarthTotemType::ShamanEarthTotemType_StoneskinTotem;
}

void NierAction_Shaman::Prepare()
{
	if (!me->HasItemCount(5175, 1))
	{
		me->StoreNewItemInBestSlots(5175, 1);
	}
	if (!me->HasItemCount(5176, 1))
	{
		me->StoreNewItemInBestSlots(5176, 1);
	}

	me->Say("Prepared", Language::LANG_UNIVERSAL);
}
