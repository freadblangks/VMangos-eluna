#include "NierStrategy_Base.h"
#include "NierConfig.h"
#include "NierManager.h"
#include "NierAction_Base.h"

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
	aoe = true;
	petting = false;
	rushing = false;

	forceBack = false;
	instantOnly = false;

	actionLimit = 0;
	ogActionTarget = ObjectGuid();
	ogTank = ObjectGuid();
	ogHealer = ObjectGuid();
	vipEntry = 0;
	actionType = ActionType::ActionType_None;

	dpsDistance = MELEE_RANGE;
	dpsDistanceMin = 0.0f;
	followDistance = INTERACTION_DISTANCE;

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
	aoe = true;
	petting = false;
	rushing = false;
	rti = -1;

	forceBack = false;
	instantOnly = false;

	actionLimit = 0;
	ogActionTarget = ObjectGuid();
	ogTank = ObjectGuid();
	ogHealer = ObjectGuid();
	actionType = ActionType::ActionType_None;
	vipEntry = 0;
	cautionSpellMap.clear();

	dpsDistance = me->GetMeleeReach();
	dpsDistanceMin = 0.0f;
	followDistance = INTERACTION_DISTANCE;

	switch (me->GetClass())
	{
	case Classes::CLASS_WARRIOR:
	{
		me->groupRole = GroupRole::GroupRole_Tank;
		break;
	}
	case Classes::CLASS_HUNTER:
	{
		aoe = true;
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
		break;
	}
	case Classes::CLASS_WARLOCK:
	{
		aoe = true;
		dpsDistance = NIER_FAR_DISTANCE;
		break;
	}
	case Classes::CLASS_PRIEST:
	{
		aoe = true;
		me->groupRole = GroupRole::GroupRole_Healer;
		dpsDistance = NIER_FAR_DISTANCE;
		break;
	}
	case Classes::CLASS_ROGUE:
	{
		break;
	}
	case Classes::CLASS_MAGE:
	{
		aoe = true;
		dpsDistance = NIER_FAR_DISTANCE;
		dpsDistanceMin = 0;
		break;
	}
	case Classes::CLASS_DRUID:
	{
		aoe = true;
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
					if (TryTank())
					{
						return;
					}
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
			}
			Follow();
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
				if (TryDPS(false, false, true))
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
									if (Engage(eachUnit))
									{
										me->SetPvP(true);
										me->UpdatePvP(true);
										me->pvpInfo.inPvPCombat = true;
										actionLimit = 30 * IN_MILLISECONDS;
										ogActionTarget = eachUnit->GetObjectGuid();
										actionType = ActionType::ActionType_Engage;
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
			if (me->nierAction->Tank(pmTarget, aoe, dpsDistance, dpsDistanceMin, basicStrategyType == BasicStrategyType::BasicStrategyType_Hold))
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

bool NierStrategy_Base::TryTank()
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
		Unit* ogTankTarget = ObjectAccessor::GetUnit(*me, myGroup->GetGuidByTargetIcon(7));
		if (ogTankTarget)
		{
			if (!ogTankTarget->GetTargetGuid().IsEmpty())
			{
				if (ogTankTarget->GetTargetGuid() != me->GetObjectGuid())
				{
					if (DoTank(ogTankTarget))
					{
						return true;
					}
				}
			}
		}
		Unit* myTarget = me->GetSelectedUnit();
		if (myTarget)
		{
			if (!myTarget->GetTargetGuid().IsEmpty())
			{
				if (myTarget->GetTargetGuid() != me->GetObjectGuid())
				{
					if (DoTank(myTarget))
					{
						myGroup->SetTargetIcon(7, myTarget->GetObjectGuid());
						return true;
					}
				}
			}
		}
		Unit* nearestOTUnit = nullptr;
		float nearestDistance = VISIBILITY_DISTANCE_NORMAL;
		for (GroupReference* groupRef = myGroup->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
		{
			if (Player* member = groupRef->getSource())
			{
				if (member->IsAlive())
				{
					if (member->GetObjectGuid() != me->GetObjectGuid())
					{
						float memberDistance = me->GetDistance(member);
						if (memberDistance < VISIBILITY_DISTANCE_NORMAL)
						{
							std::set<Unit*> memberAttackers = member->GetAttackers();
							for (std::set<Unit*>::iterator ait = memberAttackers.begin(); ait != memberAttackers.end(); ++ait)
							{
								if (Unit* eachAttacker = *ait)
								{
									if (me->IsValidAttackTarget(eachAttacker))
									{
										if (!eachAttacker->GetTargetGuid().IsEmpty())
										{
											if (eachAttacker->GetTargetGuid() != me->GetObjectGuid())
											{
												float eachDistance = me->GetDistance(eachAttacker);
												if (eachDistance < NIER_NORMAL_DISTANCE)
												{
													if (eachDistance < nearestDistance)
													{
														if (myGroup->GetTargetIconByGuid(eachAttacker->GetObjectGuid()) == -1)
														{
															nearestDistance = eachDistance;
															nearestOTUnit = eachAttacker;
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		if (nearestOTUnit)
		{
			if (DoTank(nearestOTUnit))
			{
				myGroup->SetTargetIcon(7, nearestOTUnit->GetObjectGuid());
				return true;
			}
		}
		if (DoTank(ogTankTarget))
		{
			return true;
		}
		if (myTarget)
		{
			if (!sNierManager->IsPolymorphed(myTarget))
			{
				if (DoTank(myTarget))
				{
					myGroup->SetTargetIcon(7, myTarget->GetObjectGuid());
					return true;
				}
			}
		}
		Unit* nearestAttacker = nullptr;
		nearestDistance = VISIBILITY_DISTANCE_NORMAL;
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
							if (myGroup->GetTargetIconByGuid(eachAttacker->GetObjectGuid()) == -1)
							{
								if (!eachAttacker->IsImmuneToDamage(SpellSchoolMask::SPELL_SCHOOL_MASK_NORMAL))
								{
									nearestDistance = eachDistance;
									nearestAttacker = eachAttacker;
								}
							}
						}
					}
				}
			}
		}
		if (nearestAttacker)
		{
			if (DoTank(nearestAttacker))
			{
				myGroup->SetTargetIcon(7, nearestAttacker->GetObjectGuid());
				return true;
			}
		}
	}

	return false;
}

bool NierStrategy_Base::DoTank(Unit* pmTarget)
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}

	if (me->nierAction->Tank(pmTarget, aoe, dpsDistance, dpsDistanceMin, basicStrategyType == BasicStrategyType::BasicStrategyType_Hold))
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

	if (Group* myGroup = me->GetGroup())
	{
		if (ObjectGuid ogTankTarget = myGroup->GetGuidByTargetIcon(7))
		{
			if (Unit* tankTarget = ObjectAccessor::GetUnit(*me, ogTankTarget))
			{
				if (DoDPS(tankTarget, pmForceInstantOnly, pmChasing))
				{
					return true;
				}
			}
		}

		if (Player* leader = ObjectAccessor::FindPlayer(myGroup->GetLeaderGuid()))
		{
			if (Unit* leaderTarget = leader->GetSelectedUnit())
			{
				if (leaderTarget->IsInCombat())
				{
					if (!sNierManager->IsPolymorphed(leaderTarget))
					{
						if (DoDPS(leaderTarget, pmForceInstantOnly, pmChasing))
						{
							return true;
						}
					}
				}
			}
		}
	}
	else
	{
		if (Unit* myTarget = me->GetSelectedUnit())
		{
			if (!sNierManager->IsPolymorphed(myTarget))
			{
				if (DoDPS(myTarget, pmForceInstantOnly, pmChasing))
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
			if (DoDPS(nearestAttacker, pmForceInstantOnly, pmChasing))
			{
				return true;
			}
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

	bool instant = pmForceInstantOnly;
	if (!instant)
	{
		instant = instantOnly;
	}

	if (aoe)
	{
		int attackerInRangeCount = 0;
		std::list<Creature*> creatureList;
		pmTarget->GetCreatureListWithEntryInGrid(creatureList, 0, INTERACTION_DISTANCE);
		if (!creatureList.empty())
		{
			for (std::list<Creature*>::iterator itr = creatureList.begin(); itr != creatureList.end(); ++itr)
			{
				if (Creature* hostileCreature = *itr)
				{
					if (!hostileCreature->IsPet())
					{
						if (me->IsValidAttackTarget(hostileCreature))
						{
							attackerInRangeCount++;
						}
					}
				}
			}
		}
		if (attackerInRangeCount > 3)
		{
			if (me->nierAction->AOE(pmTarget, rushing, dpsDistance, dpsDistanceMin, basicStrategyType == BasicStrategyType::BasicStrategyType_Hold, instant, pmChasing))
			{
				return true;
			}
		}
	}

	if (me->nierAction->DPS(pmTarget, rushing, dpsDistance, dpsDistanceMin, basicStrategyType == BasicStrategyType::BasicStrategyType_Hold, instant, pmChasing))
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
		if (me->GetHealthPercent() < 50.0f)
		{
			if (DoHeal(me, pmForceInstantOnly))
			{
				return true;
			}
		}
		Player* tank = nullptr;
		Player* lowMember = nullptr;
		uint32 lowMemberCount = 0;
		for (GroupReference* groupRef = myGroup->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
		{
			if (Player* member = groupRef->getSource())
			{
				if (me->IsWithinDist(member, NIER_MAX_DISTANCE))
				{
					if (member->GetHealthPercent() < 60.0f)
					{
						lowMember = member;
						lowMemberCount++;
					}
					if (member->groupRole == GroupRole::GroupRole_Tank)
					{
						tank = member;
					}
				}
			}
		}
		if (tank)
		{
			if (tank->GetHealthPercent() < 50.0f)
			{
				if (DoHeal(tank, pmForceInstantOnly))
				{
					return true;
				}
			}
		}
		if (lowMemberCount > 1)
		{
			if (me->nierAction->GroupHeal(lowMember, pmForceInstantOnly))
			{
				return true;
			}
		}
		if (tank)
		{
			if (DoHeal(tank, pmForceInstantOnly))
			{
				return true;
			}
		}
		for (GroupReference* groupRef = myGroup->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
		{
			if (Player* member = groupRef->getSource())
			{
				if (me->nierAction->SimpleHeal(member, pmForceInstantOnly))
				{
					return true;
				}
			}
		}
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

bool NierStrategy_Base::Revive()
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
				if (!member->IsAlive())
				{
					if (me->nierAction->Revive(member))
					{
						actionLimit = DEFAULT_ACTION_LIMIT_DELAY;
						ogActionTarget = member->GetObjectGuid();
						actionType = ActionType::ActionType_Revive;
						return true;
					}
				}
			}
		}
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
	if (Player* targetPlayer = (Player*)pmTarget)
	{
		if (me->nierAction->Revive(targetPlayer))
		{
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
		if (me->groupRole != GroupRole::GroupRole_Tank)
		{
			if (Player* tank = ObjectAccessor::FindPlayer(ogTank))
			{
				if (me->nierAction->Follow(tank, followDistance, 0.0f, basicStrategyType == BasicStrategyType::BasicStrategyType_Hold))
				{
					ogActionTarget = tank->GetObjectGuid();
					return true;
				}
			}
		}
		if (Player* leader = ObjectAccessor::FindPlayer(myGroup->GetLeaderGuid()))
		{
			if (me->nierAction->Follow(leader, followDistance, 0.0f, basicStrategyType == BasicStrategyType::BasicStrategyType_Hold))
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
					if (Engage(eachUnit))
					{
						me->SetPvP(true);
						me->UpdatePvP(true);
						me->pvpInfo.inPvPCombat = true;
						actionLimit = 30 * IN_MILLISECONDS;
						ogActionTarget = eachUnit->GetObjectGuid();
						actionType = ActionType::ActionType_Engage;
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

NierStrategy_The_Underbog::NierStrategy_The_Underbog() :NierStrategy_Base()
{
	hungarfen = false;
}

NierStrategy_The_Black_Morass::NierStrategy_The_Black_Morass() : NierStrategy_Base()
{

}

bool NierStrategy_The_Black_Morass::DoDPS(Unit* pmTarget, bool pmForceInstantOnly, bool pmChasing)
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	if (aoe)
	{
		int attackerInRangeCount = 0;
		std::list<Creature*> creatureList;
		pmTarget->GetCreatureListWithEntryInGrid(creatureList, 0, INTERACTION_DISTANCE);
		if (!creatureList.empty())
		{
			for (std::list<Creature*>::iterator itr = creatureList.begin(); itr != creatureList.end(); ++itr)
			{
				if (Creature* hostileCreature = *itr)
				{
					if (!hostileCreature->IsPet())
					{
						if (me->IsValidAttackTarget(hostileCreature))
						{
							attackerInRangeCount++;
						}
					}
				}
			}
		}
		if (attackerInRangeCount > 2)
		{
			if (me->nierAction->AOE(pmTarget, rushing, dpsDistance, dpsDistanceMin, basicStrategyType == BasicStrategyType::BasicStrategyType_Hold, instantOnly, pmChasing))
			{
				return true;
			}
		}
	}

	if (pmTarget->GetEntry() == 20745 || pmTarget->GetEntry() == 17880)
	{
		if (pmTarget->HasAura(38592))
		{
			return false;
		}
	}
	if (me->nierAction->DPS(pmTarget, rushing, dpsDistance, dpsDistanceMin, basicStrategyType == BasicStrategyType::BasicStrategyType_Hold, instantOnly, pmChasing))
	{
		return true;
	}

	return false;
}

NierStrategy_Magisters_Terrace::NierStrategy_Magisters_Terrace() : NierStrategy_Base()
{
	kael = false;
}

void NierStrategy_Magisters_Terrace::Update(uint32 pmDiff)
{
	if (!me)
	{
		kael = false;
	}
	else if (!me->IsAlive())
	{
		kael = false;
	}
	else if (!me->IsInCombat())
	{
		kael = false;
	}
	else if (kael)
	{
		me->nierAction->Update(pmDiff);
		if (actionLimit > 0)
		{
			actionLimit -= pmDiff;
			if (actionLimit < 0)
			{
				actionLimit = 0;
			}
			return;
		}
		if (Creature* phoenix = me->FindNearestCreature(24674, NIER_NORMAL_DISTANCE - MELEE_RANGE))
		{
			me->nierAction->nm->ResetMovement();
			me->InterruptNonMeleeSpells(true);
			Position pos;
			me->GetNearPoint(phoenix, pos.x, pos.y, pos.z, 0.0f, NIER_NORMAL_DISTANCE, phoenix->GetAngle(me));
			me->GetMotionMaster()->MovePoint(0, pos.x, pos.y, pos.z, MoveOptions::MOVE_PATHFINDING);
			actionType = ActionType::ActionType_Move;
			actionLimit = 2000;
			return;
		}
		if (Creature* flame = me->FindNearestCreature(24666, NIER_NORMAL_DISTANCE - MELEE_RANGE))
		{
			me->nierAction->nm->ResetMovement();
			me->InterruptNonMeleeSpells(true);
			Position pos;
			me->GetNearPoint(flame, pos.x, pos.y, pos.z, 0.0f, NIER_NORMAL_DISTANCE, flame->GetAngle(me));
			me->GetMotionMaster()->MovePoint(0, pos.x, pos.y, pos.z, MoveOptions::MOVE_PATHFINDING);
			actionType = ActionType::ActionType_Move;
			actionLimit = 2000;
			return;
		}
	}
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
		if (Group* myGroup = me->GetGroup())
		{
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
	}
	NierStrategy_Base::Update(pmDiff);
}

bool NierStrategy_Magisters_Terrace::TryTank()
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	if (kael)
	{
		if (Creature* egg = me->FindNearestCreature(24675, VISIBILITY_DISTANCE_NORMAL))
		{
			if (DoTank(egg))
			{
				return true;
			}
		}
		Creature* boss = me->FindNearestCreature(24664, VISIBILITY_DISTANCE_NORMAL);
		if (!boss)
		{
			boss = me->FindNearestCreature(24857, VISIBILITY_DISTANCE_NORMAL);
		}
		if (DoTank(boss))
		{
			return true;
		}
	}

	return NierStrategy_Base::TryTank();
}

bool NierStrategy_Magisters_Terrace::DoTank(Unit* pmTarget)
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	if (pmTarget->GetEntry() == 24664 || pmTarget->GetEntry() == 24857)
	{
		kael = true;
	}
	return NierStrategy_Base::DoTank(pmTarget);
}

bool NierStrategy_Magisters_Terrace::TryDPS(bool pmDelay, bool pmForceInstantOnly, bool pmChasing)
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
	if (kael)
	{
		if (Creature* phoenix = me->FindNearestCreature(24674, VISIBILITY_DISTANCE_NORMAL))
		{
			if (DoDPS(phoenix, false, pmChasing))
			{
				rushing = true;
				return true;
			}
		}
		if (Creature* egg = me->FindNearestCreature(24675, VISIBILITY_DISTANCE_NORMAL))
		{
			if (DoDPS(egg, false, pmChasing))
			{
				rushing = true;
				return true;
			}
		}
		Creature* boss = me->FindNearestCreature(24664, VISIBILITY_DISTANCE_NORMAL);
		if (!boss)
		{
			boss = me->FindNearestCreature(24857, VISIBILITY_DISTANCE_NORMAL);
		}
		if (DoDPS(boss, false, pmChasing))
		{
			rushing = true;
			return true;
		}
	}
	if (Group* myGroup = me->GetGroup())
	{
		if (ObjectGuid ogTankTarget = myGroup->GetGuidByTargetIcon(7))
		{
			if (Unit* tankTarget = ObjectAccessor::GetUnit(*me, ogTankTarget))
			{
				if (DoDPS(tankTarget, pmForceInstantOnly, pmChasing))
				{
					if (tankTarget->GetEntry() == 24664 || tankTarget->GetEntry() == 24857)
					{
						kael = true;
					}
					return true;
				}
			}
		}

		if (Player* leader = ObjectAccessor::FindPlayer(myGroup->GetLeaderGuid()))
		{
			if (Unit* leaderTarget = leader->GetSelectedUnit())
			{
				if (leaderTarget->IsInCombat())
				{
					if (!sNierManager->IsPolymorphed(leaderTarget))
					{
						if (DoDPS(leaderTarget, pmForceInstantOnly, pmChasing))
						{
							if (leaderTarget->GetEntry() == 24664 || leaderTarget->GetEntry() == 24857)
							{
								kael = true;
							}
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

bool NierStrategy_Magisters_Terrace::DoDPS(Unit* pmTarget, bool pmForceInstantOnly, bool pmChasing)
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
	if (pmTarget->GetEntry() == 24664 || pmTarget->GetEntry() == 24857)
	{
		kael = true;
	}

	return NierStrategy_Base::DoDPS(pmTarget, pmForceInstantOnly, pmChasing);
}

bool NierStrategy_Magisters_Terrace::DoHeal(Unit* pmTarget, bool pmForceInstantOnly)
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
	if (Player* targetPlayer = pmTarget->ToPlayer())
	{
		if (Unit* targetTarget = targetPlayer->GetSelectedUnit())
		{
			if (targetTarget->GetEntry() == 24664 || targetTarget->GetEntry() == 24857)
			{
				kael = true;
			}
		}
	}

	return NierStrategy_Base::DoHeal(pmTarget, pmForceInstantOnly);
}