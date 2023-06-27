#include "NierStrategy_Base.h"
#include "NierConfig.h"
#include "NierManager.h"
#include "NierAction_Base.h"
#include "NierAction_Paladin.h"

#include "Player.h"
#include "Group.h"
#include "Pet.h"
#include "GridNotifiers.h"
#include "Spell.h"

NierStrategy_Base::NierStrategy_Base()
{
	me = nullptr;
	initialized = false;

	dpsDelay = sNierConfig.DPSDelay;
	randomTeleportDelay = 0;

	corpseRunDelay = 0;

	restLimit = 0;
	drinkDelay = 0;
	wanderDelay = 0;
	assembleDelay = 0;
	reviveDelay = 0;

	combatDuration = 0;
	pvpDelay = 0;
	rti = -1;

	basicStrategyType = BasicStrategyType::BasicStrategyType_Normal;
	cure = true;
	petting = false;
	rushing = false;

	instantOnly = false;

	actionLimit = 0;
	ogActionTarget = ObjectGuid();
	ogTank = ObjectGuid();
	ogHealer = ObjectGuid();
	vipEntry = 0;
	actionType = ActionType::ActionType_None;

	dpsDistance = DEFAULT_COMBAT_REACH;
	dpsDistanceMin = 0.0f;
	followDistance = NIER_MIN_DISTANCE;

	cautionSpellMap.clear();
}

void NierStrategy_Base::Report()
{
	if (Group* myGroup = me->GetGroup())
	{
		if (myGroup->GetLeaderGuid() != me->GetObjectGuid())
		{
			if (Player* leaderPlayer = ObjectAccessor::FindPlayer(myGroup->GetLeaderGuid()))
			{
				std::ostringstream reportStream;
				reportStream << "Strategy : " << me->activeStrategyIndex << " - Role : " << me->groupRole << " - Specialty : " << sNierManager->characterTalentTabNameMap[me->GetClass()][me->nierAction->specialty] << " RTI : " << rti;
				sNierManager->WhisperTo(leaderPlayer, reportStream.str(), Language::LANG_UNIVERSAL, me);

				// paladin report 
				std::ostringstream classStream;
				if (me->GetClass() == Classes::CLASS_PALADIN)
				{
					if (NierAction_Paladin* na = (NierAction_Paladin*)me->nierAction)
					{
						classStream << "Seal type : " << na->sealType << " - Blessing type : " << na->blessingType << " - Aura type : " << na->auraType;
						sNierManager->WhisperTo(leaderPlayer, classStream.str(), Language::LANG_UNIVERSAL, me);
					}
				}
			}
		}
	}
}

void NierStrategy_Base::Reset()
{
	corpseRunDelay = 0;

	restLimit = 0;
	drinkDelay = 0;

	wanderDelay = 0;
	combatDuration = 0;
	pvpDelay = 0;

	dpsDelay = sNierConfig.DPSDelay;

	basicStrategyType = BasicStrategyType::BasicStrategyType_Normal;

	cure = true;
	petting = false;
	rushing = false;
	rti = -1;

	instantOnly = false;

	actionLimit = 0;
	ogActionTarget = ObjectGuid();
	ogTank = ObjectGuid();
	ogHealer = ObjectGuid();
	actionType = ActionType::ActionType_None;
	vipEntry = 0;
	cautionSpellMap.clear();

	dpsDistance = DEFAULT_COMBAT_REACH;
	dpsDistanceMin = 0.0f;
	followDistance = NIER_MIN_DISTANCE;

	switch (me->GetClass())
	{
	case Classes::CLASS_WARRIOR:
	{
		me->groupRole = GroupRole::GroupRole_Tank;
		break;
	}
	case Classes::CLASS_HUNTER:
	{
		dpsDistance = NIER_MAX_DISTANCE;
		dpsDistanceMin = 0;
		break;
	}
	case Classes::CLASS_SHAMAN:
	{
		break;
	}
	case Classes::CLASS_PALADIN:
	{
		me->groupRole = GroupRole::GroupRole_Tank;
		break;
	}
	case Classes::CLASS_WARLOCK:
	{
		dpsDistance = NIER_FAR_DISTANCE;
		break;
	}
	case Classes::CLASS_PRIEST:
	{
		me->groupRole = GroupRole::GroupRole_Healer;
		dpsDistance = NIER_FAR_DISTANCE;
		followDistance = NIER_NORMAL_DISTANCE;
		break;
	}
	case Classes::CLASS_ROGUE:
	{
		break;
	}
	case Classes::CLASS_MAGE:
	{
		dpsDistance = NIER_FAR_DISTANCE;
		dpsDistanceMin = 0;
		break;
	}
	case Classes::CLASS_DRUID:
	{
		dpsDistance = NIER_FAR_DISTANCE;
		break;
	}
	default:
	{
		break;
	}
	}
}

void NierStrategy_Base::Update(uint32 pmDiff)
{
	if (!me)
	{
		return;
	}
	if (!initialized)
	{
		return;
	}
	me->nierAction->Update(pmDiff);
	if (reviveDelay > 0)
	{
		reviveDelay -= pmDiff;
		if (reviveDelay <= 0)
		{
			reviveDelay = 0;
			me->ResurrectPlayer(0.1f);
			me->ClearInCombat();
			me->SetSelectionGuid(ObjectGuid());
			me->GetMotionMaster()->Clear();
		}
	}
	if (assembleDelay > 0)
	{
		assembleDelay -= pmDiff;
		if (assembleDelay <= 0)
		{
			assembleDelay = 0;
			if (me->IsBeingTeleported())
			{
				assembleDelay = 1000;
			}
			else
			{
				if (Group* myGroup = me->GetGroup())
				{
					if (me->GetObjectGuid() != myGroup->GetLeaderGuid())
					{
						if (Player* leader = ObjectAccessor::FindPlayer(myGroup->GetLeaderGuid()))
						{
							me->ClearInCombat();
							me->SetSelectionGuid(ObjectGuid());
							me->GetMotionMaster()->Clear();
							if (me->TeleportTo(leader->GetMapId(), leader->GetPositionX(), leader->GetPositionY(), leader->GetPositionZ(), leader->GetOrientation()))
							{
								sNierManager->WhisperTo(leader, "In position.", Language::LANG_UNIVERSAL, me);
							}
							if (!me->IsAlive())
							{
								sNierManager->WhisperTo(leader, "Revive in 5 seconds", Language::LANG_UNIVERSAL, me);
								reviveDelay = 5000;
							}
						}
					}
				}
			}
		}
	}
	if (me->IsAlive())
	{
		if (me->HasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_CONFUSED | UNIT_STAT_FLEEING))
		{
			return;
		}
		if (me->IsNonMeleeSpellCasted(true))
		{
			bool interrupt = false;
			if (Spell* cs = me->GetCurrentSpell(CurrentSpellTypes::CURRENT_AUTOREPEAT_SPELL))
			{
				if (Unit* victim = cs->m_targets.getUnitTarget())
				{
					if (!victim->IsAlive())
					{
						interrupt = true;
					}
				}
			}
			else if (Spell* cs = me->GetCurrentSpell(CurrentSpellTypes::CURRENT_CHANNELED_SPELL))
			{
				if (Unit* victim = cs->m_targets.getUnitTarget())
				{
					if (!victim->IsAlive())
					{
						interrupt = true;
					}
				}
			}
			else if (Spell* cs = me->GetCurrentSpell(CurrentSpellTypes::CURRENT_GENERIC_SPELL))
			{
				if (Unit* victim = cs->m_targets.getUnitTarget())
				{
					if (!victim->IsAlive())
					{
						if (victim->IsHostileTo(me))
						{
							interrupt = true;
						}
					}
				}
			}
			else if (Spell* cs = me->GetCurrentSpell(CurrentSpellTypes::CURRENT_MELEE_SPELL))
			{
				if (Unit* victim = cs->m_targets.getUnitTarget())
				{
					if (!victim->IsAlive())
					{
						interrupt = true;
					}
				}
			}
			if (interrupt)
			{
				me->InterruptNonMeleeSpells(true);
			}
			else
			{
				return;
			}
		}
		if (actionLimit > 0)
		{
			actionLimit -= pmDiff;
			if (actionLimit < 0)
			{
				actionLimit = 0;
			}
			switch (actionType)
			{
			case ActionType_None:
			{
				break;
			}
			case ActionType_Engage:
			{
				if (Unit* actionTarget = ObjectAccessor::GetUnit(*me, ogActionTarget))
				{
					if (Engage(actionTarget))
					{
						return;
					}
				}
				break;
			}
			case ActionType_Revive:
			{
				if (Unit* actionTarget = ObjectAccessor::GetUnit(*me, ogActionTarget))
				{
					if (Revive(actionTarget))
					{
						return;
					}
				}
				break;
			}
			case ActionType::ActionType_Move:
			{
				return;
			}
			case ActionType::ActionType_ReadyTank:
			{
				if (Unit* actionTarget = ObjectAccessor::GetUnit(*me, ogActionTarget))
				{
					if (me->nierAction->ReadyTank(actionTarget))
					{
						return;
					}
				}
				return;
			}
			case ActionType::ActionType_Attack:
			{
				if (Unit* actionTarget = ObjectAccessor::GetUnit(*me, ogActionTarget))
				{
					if (me->nierAction->Attack(actionTarget))
					{
						return;
					}
				}
				break;
			}
			default:
				break;
			}
			actionLimit = 0;
			ogActionTarget.Clear();
		}
		if (Group* myGroup = me->GetGroup())
		{
			if (basicStrategyType == BasicStrategyType::BasicStrategyType_Glue)
			{
				switch (me->groupRole)
				{
				case GroupRole::GroupRole_DPS:
				{
					TryDPS(false, true, false);
					break;
				}
				case GroupRole::GroupRole_Healer:
				{
					TryHeal(true);
					Cure();
					break;
				}
				case GroupRole::GroupRole_Tank:
				{
					break;
				}
				default:
				{
					break;
				}
				}
				if (Player* leader = ObjectAccessor::FindPlayer(myGroup->GetLeaderGuid()))
				{
					if (leader->IsAlive())
					{
						if (me->GetDistance(leader) > CONTACT_DISTANCE)
						{
							me->nierAction->nm->ResetMovement();
							me->InterruptNonMeleeSpells(true);
							me->GetMotionMaster()->MovePoint(0, leader->GetPositionX(), leader->GetPositionY(), leader->GetPositionZ(), MoveOptions::MOVE_PATHFINDING);
						}
						return;
					}
					else
					{
						basicStrategyType = BasicStrategyType::BasicStrategyType_Normal;
					}
				}
				else
				{
					basicStrategyType = BasicStrategyType::BasicStrategyType_Normal;
				}
			}
			if (GroupInCombat())
			{
				if (me->GetStandState() != UnitStandStateType::UNIT_STAND_STATE_STAND)
				{
					me->SetStandState(UNIT_STAND_STATE_STAND);
				}
				if (vipEntry > 0)
				{
					if (vipCheckDelay > 0)
					{
						vipCheckDelay -= pmDiff;
						if (vipCheckDelay <= 0)
						{
							vipCheckDelay = 2000;
							if (ogVip.IsEmpty())
							{
								if (Creature* vipC = me->FindNearestCreature(vipEntry, VISIBILITY_DISTANCE_NORMAL))
								{
									ogVip = vipC->GetObjectGuid();
								}
							}
						}
					}
				}
				combatDuration += pmDiff;
				restLimit = 0;
				if (basicStrategyType == BasicStrategyType::BasicStrategyType_Freeze)
				{
					return;
				}
				uint32 cautionDelay = Caution();
				if (cautionDelay > 0)
				{
					actionType = ActionType::ActionType_Move;
					actionLimit = cautionDelay;
					basicStrategyType = BasicStrategyType::BasicStrategyType_Hold;
					return;
				}
				if (Assist())
				{
					return;
				}
				if (me->IsNonMeleeSpellCasted(false))
				{
					return;
				}
				switch (me->groupRole)
				{
				case GroupRole::GroupRole_DPS:
				{
					if (Cure())
					{
						return;
					}
					if (TryDPS(true, false, true))
					{
						return;
					}
					break;
				}
				case GroupRole::GroupRole_Healer:
				{
					if (TryHeal(false))
					{
						return;
					}
					if (Cure())
					{
						return;
					}
					break;
				}
				case GroupRole::GroupRole_Tank:
				{
					break;
				}
				default:
				{
					break;
				}
				}
			}
			else
			{
				rushing = false;
				combatDuration = 0;
				if (restLimit > 0)
				{
					restLimit -= pmDiff;
					if (drinkDelay >= 0)
					{
						drinkDelay -= pmDiff;
						if (drinkDelay < 0)
						{
							me->nierAction->Drink();
						}
					}
					return;
				}
				if (Rest())
				{
					return;
				}
				if (me->GetStandState() != UnitStandStateType::UNIT_STAND_STATE_STAND)
				{
					me->SetStandState(UNIT_STAND_STATE_STAND);
				}
				if (basicStrategyType == BasicStrategyType::BasicStrategyType_Freeze)
				{
					return;
				}
				if (Buff())
				{
					return;
				}
				if (Cure())
				{
					return;
				}
				if (Petting())
				{
					return;
				}
				Follow();
			}
		}
		else
		{
			if (me->IsInCombat())
			{
				restLimit = 0;
				combatDuration += pmDiff;
				if (me->GetStandState() != UnitStandStateType::UNIT_STAND_STATE_STAND)
				{
					me->SetStandState(UNIT_STAND_STATE_STAND);
				}
				if (Cure())
				{
					return;
				}
				if (TryHeal(false))
				{
					return;
				}
				if (TryAttack())
				{
					return;
				}
			}
			else
			{
				if (Buff())
				{
					return;
				}
				combatDuration = 0;
				if (randomTeleportDelay > 0)
				{
					randomTeleportDelay -= pmDiff;
					if (randomTeleportDelay <= 0)
					{
						randomTeleportDelay = urand(sNierConfig.RandomTeleportDelay_Min, sNierConfig.RandomTeleportDelay_Max);
						int myClass = me->GetClass();
						if (myClass != Classes::CLASS_PRIEST)
						{
							if (me->nierAction->RandomTeleport())
							{
								return;
							}
						}
					}
				}
				if (pvpDelay >= 0)
				{
					pvpDelay -= pmDiff;
					if (pvpDelay <= 0)
					{
						pvpDelay = 5000;
						std::list<Unit*> targets;
						MaNGOS::AnyUnitInObjectRangeCheck u_check(me, 100.0f);
						MaNGOS::UnitListSearcher<MaNGOS::AnyUnitInObjectRangeCheck> searcher(targets, u_check);
						// Don't need to use visibility modifier, units won't be able to cast outside of draw distance
						Cell::VisitAllObjects(me, searcher, 100.0f);
						for (std::list<Unit*>::iterator uIT = targets.begin(); uIT != targets.end(); uIT++)
						{
							if (Unit* eachUnit = *uIT)
							{
								if (eachUnit->GetTypeId() == TypeID::TYPEID_PLAYER)
								{
									if (me->nierAction->Attack(eachUnit))
									{
										me->SetPvP(true);
										me->UpdatePvP(true);
										me->pvpInfo.inPvPCombat = true;
										actionLimit = 30 * IN_MILLISECONDS;
										ogActionTarget = eachUnit->GetObjectGuid();
										actionType = ActionType::ActionType_Attack;
										return;
									}
								}
							}
						}
					}
				}
				if (wanderDelay >= 0)
				{
					wanderDelay -= pmDiff;
					if (wanderDelay <= 0)
					{
						wanderDelay = urand(30 * IN_MILLISECONDS, 1 * MINUTE * IN_MILLISECONDS);
						if (Wander())
						{
							return;
						}
					}
				}
			}
		}
	}
	else
	{
		if (me->GetDeathTimer() > sNierConfig.ReviveDelay)
		{
			me->ClearInCombat();
			me->nierAction->ClearTarget();
			me->nierAction->nm->ResetMovement();
			float nearX = 0.0f;
			float nearY = 0.0f;
			float nearZ = 0.0f;
			float nearDistance = frand(40.0f, 50.0f);
			float nearAngle = frand(0.0f, M_PI * 2);
			me->GetNearPoint(me, nearX, nearY, nearZ, 0.0f, nearDistance, nearAngle);
			me->ResurrectPlayer(0.2f);
			me->TeleportTo(me->GetMapId(), nearX, nearY, nearZ, 0);
			me->SpawnCorpseBones();
		}
		return;
	}
}

bool NierStrategy_Base::GroupInCombat()
{
	if (!me)
	{
		return false;
	}
	if (Group* myGroup = me->GetGroup())
	{
		for (GroupReference* groupRef = myGroup->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
		{
			if (Player* member = groupRef->getSource())
			{
				if (member->IsInCombat())
				{
					if (me->GetDistance(member) < VISIBILITY_DISTANCE_NORMAL)
					{
						return true;
					}
				}
				else if (Pet* memberPet = member->GetPet())
				{
					if (memberPet->IsInCombat())
					{
						if (me->GetDistance(memberPet) < VISIBILITY_DISTANCE_NORMAL)
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

bool NierStrategy_Base::Engage(Unit* pmTarget)
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}

	if (NierAction_Base* nab = me->nierAction)
	{
		switch (me->groupRole)
		{
		case GroupRole::GroupRole_Tank:
		{
			if (me->nierAction->Tank(pmTarget, false))
			{
				if (Group* myGroup = me->GetGroup())
				{
					if (myGroup->GetGuidByTargetIcon(7) != pmTarget->GetObjectGuid())
					{
						myGroup->SetTargetIcon(7, pmTarget->GetObjectGuid());
					}
				}
				return true;
			}
		}
		case GroupRole::GroupRole_DPS:
		{
			return DoDPS(pmTarget, false, true);
		}
		default:
		{
			break;
		}
		}
	}

	return false;
}

bool NierStrategy_Base::TryAttack()
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}

	if (Unit* myTarget = me->GetSelectedUnit())
	{
		if (!sNierManager->IsPolymorphed(myTarget))
		{
			if (me->nierAction->Attack(myTarget))
			{
				return true;
			}
		}
	}
	Unit* nearestAttacker = nullptr;
	float nearestDistance = VISIBILITY_DISTANCE_NORMAL;
	std::set<Unit*> myAttackers = me->GetAttackers();
	for (std::set<Unit*>::iterator ait = myAttackers.begin(); ait != myAttackers.end(); ++ait)
	{
		if (Unit* eachAttacker = *ait)
		{
			if (me->IsValidAttackTarget(eachAttacker))
			{
				if (!sNierManager->IsPolymorphed(eachAttacker))
				{
					float eachDistance = me->GetDistance(eachAttacker);
					if (eachDistance < nearestDistance)
					{
						nearestDistance = eachDistance;
						nearestAttacker = eachAttacker;
					}
				}
			}
		}
	}
	if (nearestAttacker)
	{
		if (me->nierAction->Attack(nearestAttacker))
		{
			return true;
		}
	}

	return false;
}

bool NierStrategy_Base::DoTank(Unit* pmTarget, bool aoe)
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	if (!pmTarget)
	{
		return false;
	}
	else if (!pmTarget->IsAlive())
	{
		return false;
	}


	if (me->nierAction->Tank(pmTarget, aoe))
	{
		return true;
	}

	return false;
}

bool NierStrategy_Base::TryDPS(bool pmDelay, bool pmForceInstantOnly, bool pmChasing)
{
	if (pmDelay)
	{
		if (combatDuration < dpsDelay)
		{
			return false;
		}
	}

	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}

	if (!ogVip.IsEmpty())
	{
		if (Unit* vip = ObjectAccessor::GetUnit(*me, ogVip))
		{
			if (vip->IsAlive())
			{
				if (DoDPS(vip, pmForceInstantOnly, pmChasing))
				{
					return true;
				}
			}
			else
			{
				ogVip.Clear();
			}
		}
		else
		{
			ogVip.Clear();
		}
	}

	return false;
}

bool NierStrategy_Base::DoDPS(Unit* pmTarget, bool pmForceInstantOnly, bool pmChasing)
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	if (!pmTarget)
	{
		return false;
	}
	else if (!pmTarget->IsAlive())
	{
		return false;
	}

	bool instant = pmForceInstantOnly;
	if (!instant)
	{
		instant = instantOnly;
	}

	//if (aoe)
	//{
	//	int attackerInRangeCount = 0;
	//	std::list<Creature*> creatureList;
	//	pmTarget->GetCreatureListWithEntryInGrid(creatureList, 0, INTERACTION_DISTANCE);
	//	if (!creatureList.empty())
	//	{
	//		for (std::list<Creature*>::iterator itr = creatureList.begin(); itr != creatureList.end(); ++itr)
	//		{
	//			if (Creature* hostileCreature = *itr)
	//			{
	//				if (!hostileCreature->IsPet())
	//				{
	//					if (me->IsValidAttackTarget(hostileCreature))
	//					{
	//						attackerInRangeCount++;
	//					}
	//				}
	//			}
	//		}
	//	}
	//	if (attackerInRangeCount > 3)
	//	{
	//		if (me->nierAction->AOE(pmTarget, rushing, dpsDistance, dpsDistanceMin, basicStrategyType == BasicStrategyType::BasicStrategyType_Hold, instant, pmChasing))
	//		{
	//			return true;
	//		}
	//	}
	//}

	if (me->nierAction->DPS(pmTarget, rushing, pmChasing, dpsDistance, dpsDistanceMin))
	{
		return true;
	}

	return false;
}

bool NierStrategy_Base::Rest(bool pmForce)
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	bool doRest = false;
	if (pmForce)
	{
		doRest = true;
	}
	else
	{
		if (me->GetHealthPercent() < 70.0f)
		{
			doRest = true;
		}
		else
		{
			if (me->GetPowerType() == Powers::POWER_MANA)
			{
				if (me->GetPowerPercent(Powers::POWER_MANA) < 50.0f)
				{
					doRest = true;
				}
			}
		}
	}

	if (doRest)
	{
		if (me->nierAction->Eat())
		{
			restLimit = 25 * IN_MILLISECONDS;
			drinkDelay = 1 * IN_MILLISECONDS;
			return true;
		}
	}

	return false;
}

bool NierStrategy_Base::TryHeal(bool pmForceInstantOnly)
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}

	if (Group* myGroup = me->GetGroup())
	{
		// group actions
	}
	else
	{
		if (DoHeal(me, pmForceInstantOnly))
		{
			return true;
		}
	}

	return false;
}

bool NierStrategy_Base::DoHeal(Unit* pmTarget, bool pmForceInstantOnly)
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}

	bool instant = pmForceInstantOnly;
	if (!instant)
	{
		instant = instantOnly;
	}
	if (me->nierAction->Heal(pmTarget, instant))
	{
		return true;
	}

	return false;
}

bool NierStrategy_Base::Revive(Unit* pmTarget)
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	if (me->IsNonMeleeSpellCasted(false, false, true))
	{
		return true;
	}
	if (Player* targetPlayer = (Player*)pmTarget)
	{
		if (me->nierAction->Revive(targetPlayer))
		{
			actionLimit = DEFAULT_ACTION_LIMIT_DELAY;
			ogActionTarget = pmTarget->GetObjectGuid();
			actionType = ActionType::ActionType_Revive;
			return true;
		}
	}

	return false;
}

bool NierStrategy_Base::Buff()
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	if (Group* myGroup = me->GetGroup())
	{
		for (GroupReference* groupRef = myGroup->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
		{
			if (Player* member = groupRef->getSource())
			{
				if (me->nierAction->Buff(member))
				{
					return true;
				}
			}
		}
	}
	else
	{
		return me->nierAction->Buff(me);
	}

	return false;
}

bool NierStrategy_Base::Assist()
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	if (me->nierAction->Assist(rti))
	{
		return true;
	}

	return false;
}

uint32 NierStrategy_Base::Caution()
{
	uint32 result = 0;

	if (me)
	{
		if (me->IsAlive())
		{
			if (NierStrategy_Base* ns = me->nierStrategyMap[me->activeStrategyIndex])
			{
				if (ns->cautionSpellMap.size() > 0)
				{
					for (std::unordered_map<std::string, std::unordered_set<uint32>>::iterator nameIT = ns->cautionSpellMap.begin(); nameIT != ns->cautionSpellMap.end(); nameIT++)
					{
						for (std::unordered_set<uint32>::iterator idIT = nameIT->second.begin(); idIT != nameIT->second.end(); idIT++)
						{
							if (me->HasAura(*idIT))
							{
								result = me->nierAction->Caution();
								if (result > 0)
								{
									return result;
								}
							}
						}
					}
				}
			}
		}
	}

	return 0;
}

bool NierStrategy_Base::Petting()
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	if (me->nierAction->Petting(petting))
	{
		return true;
	}

	return false;
}

bool NierStrategy_Base::Cure()
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	if (Group* myGroup = me->GetGroup())
	{
		for (GroupReference* groupRef = myGroup->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
		{
			if (Player* member = groupRef->getSource())
			{
				if (me->nierAction->Cure(member))
				{
					return true;
				}
			}
		}
	}
	else
	{
		return me->nierAction->Cure(me);
	}

	return false;
}

bool NierStrategy_Base::Follow()
{
	if (basicStrategyType == BasicStrategyType::BasicStrategyType_Freeze)
	{
		return false;
	}
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	if (Group* myGroup = me->GetGroup())
	{
		ObjectGuid ogLeader = myGroup->GetLeaderGuid();
		if (Player* leader = ObjectAccessor::FindPlayer(ogLeader))
		{
			if (me->nierAction->Follow(leader, followDistance))
			{
				ogActionTarget = leader->GetObjectGuid();
				return true;
			}
		}
	}

	return false;
}

bool NierStrategy_Base::Wander()
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	else if (me->IsMoving())
	{
		return true;
	}

	uint32 wanderRate = urand(0, 100);
	if (wanderRate < 25)
	{
		std::list<Unit*> targets;
		MaNGOS::AnyUnitInObjectRangeCheck u_check(me, 100.0f);
		MaNGOS::UnitListSearcher<MaNGOS::AnyUnitInObjectRangeCheck> searcher(targets, u_check);
		// Don't need to use visibility modifier, units won't be able to cast outside of draw distance
		Cell::VisitAllObjects(me, searcher, 100.0f);
		for (std::list<Unit*>::iterator uIT = targets.begin(); uIT != targets.end(); uIT++)
		{
			if (Unit* eachUnit = *uIT)
			{
				if (eachUnit->GetTypeId() == TypeID::TYPEID_PLAYER)
				{
					if (me->nierAction->Attack(eachUnit))
					{
						me->SetPvP(true);
						me->UpdatePvP(true);
						me->pvpInfo.inPvPCombat = true;
						actionLimit = 30 * IN_MILLISECONDS;
						ogActionTarget = eachUnit->GetObjectGuid();
						actionType = ActionType::ActionType_Attack;
						return true;
					}
				}
			}
		}
	}
	else if (wanderRate < 50)
	{
		float angle = frand(0, 2 * M_PI);
		float distance = frand(10.0f, 30.0f);
		Position dest;
		me->GetNearPoint(me, dest.x, dest.y, dest.z, 0.0f, distance, angle);
		me->nierAction->nm->Point(dest);
	}

	return true;
}

std::string NierStrategy_Base::GetGroupRole()
{
	if (!me)
	{
		return "";
	}
	switch (me->groupRole)
	{
	case GroupRole::GroupRole_DPS:
	{
		return "dps";
	}
	case GroupRole::GroupRole_Tank:
	{
		return "tank";
	}
	case GroupRole::GroupRole_Healer:
	{
		return "healer";
	}
	default:
	{
		break;
	}
	}
	return "dps";
}

void NierStrategy_Base::SetGroupRole(std::string pmRoleName)
{
	if (!me)
	{
		return;
	}
	if (pmRoleName == "dps")
	{
		me->groupRole = GroupRole::GroupRole_DPS;
	}
	else if (pmRoleName == "tank")
	{
		me->groupRole = GroupRole::GroupRole_Tank;
	}
	else if (pmRoleName == "healer")
	{
		me->groupRole = GroupRole::GroupRole_Healer;
	}
}
