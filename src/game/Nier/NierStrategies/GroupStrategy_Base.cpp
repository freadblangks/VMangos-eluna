#include "GroupStrategy_Base.h"
#include "NierConfig.h"
#include "NierManager.h"
#include "NierAction_Base.h"
#include "NierAction_Paladin.h"

#include "Player.h"
#include "Group.h"
#include "Pet.h"
#include "GridNotifiers.h"
#include "Spell.h"

GroupStrategy_Base::GroupStrategy_Base()
{
	me = nullptr;
	interruptDelay = 0;
}

void GroupStrategy_Base::Reset()
{
	interruptDelay = 0;
}

void GroupStrategy_Base::Update(uint32 pmDiff)
{
	if (!me)
	{
		return;
	}

	if (interruptDelay > 0)
	{
		interruptDelay -= pmDiff;
	}
	bool groupInCombat = false;
	Unit* tank = nullptr;
	Unit* lowMember = nullptr;
	for (GroupReference* groupRef = me->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
	{
		if (Player* member = groupRef->getSource())
		{
			if (member->IsAlive())
			{
				if (member->groupRole == GroupRole::GroupRole_Tank)
				{
					tank = member;
				}
				else
				{
					if (member->GetHealthPercent() < 50.0f)
					{
						lowMember = member;
					}
				}
			}
			if (member->IsInCombat())
			{
				groupInCombat = true;
			}
		}
	}
	if (groupInCombat)
	{
		bool tankAOE = false;
		uint32 tankEnemyCount = 0;
		if (tank)
		{
			std::list<Unit*> targets;
			MaNGOS::AnyUnitInObjectRangeCheck u_check(tank, NIER_NEAR_DISTANCE);
			MaNGOS::UnitListSearcher<MaNGOS::AnyUnitInObjectRangeCheck> searcher(targets, u_check);
			// Don't need to use visibility modifier, units won't be able to cast outside of draw distance
			Cell::VisitAllObjects(tank, searcher, NIER_NEAR_DISTANCE);
			for (std::list<Unit*>::iterator uIT = targets.begin(); uIT != targets.end(); uIT++)
			{
				if (Unit* eachUnit = *uIT)
				{
					if (eachUnit->IsHostileTo(tank))
					{
						if (eachUnit->IsInCombat())
						{
							tankEnemyCount += 1;
						}
					}
				}
			}
		}
		if (tankEnemyCount > 1)
		{
			tankAOE = true;
		}
		std::unordered_set<ObjectGuid> interruptedSet;
		for (GroupReference* groupRef = me->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
		{
			if (Player* member = groupRef->getSource())
			{
				if (member->isNier)
				{
					if (member->IsAlive())
					{
						if (member->HasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_CONFUSED | UNIT_STAT_FLEEING))
						{
							continue;
						}
						if (NierStrategy_Base* ns = member->nierStrategyMap[member->activeStrategyIndex])
						{
							if (ns->basicStrategyType == BasicStrategyType::BasicStrategyType_Freeze)
							{
								continue;
							}
							if (member->groupRole == GroupRole::GroupRole_Tank)
							{
								Unit* ogTankTarget = ObjectAccessor::GetUnit(*member, me->GetGuidByTargetIcon(7));
								if (ogTankTarget)
								{
									if (!ogTankTarget->GetTargetGuid().IsEmpty())
									{
										if (ogTankTarget->GetTargetGuid() != member->GetObjectGuid())
										{
											if (ns->DoTank(ogTankTarget, tankAOE))
											{
												continue;
											}
										}
									}
								}
								Unit* myTarget = member->GetSelectedUnit();
								if (myTarget)
								{
									if (!myTarget->GetTargetGuid().IsEmpty())
									{
										if (myTarget->GetTargetGuid() != member->GetObjectGuid())
										{
											if (ns->DoTank(myTarget, tankAOE))
											{
												me->SetTargetIcon(7, myTarget->GetObjectGuid());
												continue;
											}
										}
									}
								}
								if (Unit* nearestOTUnit = GetNeareastOTUnit(member))
								{
									if (ns->DoTank(nearestOTUnit, tankAOE))
									{
										me->SetTargetIcon(7, nearestOTUnit->GetObjectGuid());
										continue;
									}
								}
								if (ns->DoTank(ogTankTarget, tankAOE))
								{
									continue;
								}
								if (myTarget)
								{
									if (!sNierManager->IsPolymorphed(myTarget))
									{
										if (ns->DoTank(myTarget, tankAOE))
										{
											me->SetTargetIcon(7, myTarget->GetObjectGuid());
											continue;
										}
									}
								}
								if (Unit* nearestAttacker = GetNeareastAttacker(member))
								{
									if (ns->DoTank(nearestAttacker, tankAOE))
									{
										me->SetTargetIcon(7, nearestAttacker->GetObjectGuid());
										continue;
									}
								}
							}
							else if (member->groupRole == GroupRole::GroupRole_DPS)
							{
								if (ns->ogVip.IsEmpty())
								{
									if (ObjectGuid ogTankTarget = me->GetGuidByTargetIcon(7))
									{
										if (Unit* tankTarget = ObjectAccessor::GetUnit(*member, ogTankTarget))
										{
											if (interruptDelay <= 0)
											{
												if (tankTarget->IsNonMeleeSpellCasted(false, false, true))
												{
													if (interruptedSet.find(tankTarget->GetObjectGuid()) == interruptedSet.end())
													{
														if (member->nierAction->Interrupt(tankTarget))
														{
															interruptedSet.insert(tankTarget->GetObjectGuid());
															interruptDelay = 500;
															continue;
														}
													}
												}
											}
											if (ns->DoDPS(tankTarget, false, true))
											{
												continue;
											}
										}
									}

									if (Player* leader = ObjectAccessor::FindPlayer(me->GetLeaderGuid()))
									{
										if (Unit* leaderTarget = leader->GetSelectedUnit())
										{
											if (leaderTarget->IsInCombat())
											{
												if (!sNierManager->IsPolymorphed(leaderTarget))
												{
													if (interruptDelay <= 0)
													{
														if (leaderTarget->IsNonMeleeSpellCasted(false, false, true))
														{
															if (interruptedSet.find(leaderTarget->GetObjectGuid()) == interruptedSet.end())
															{
																if (member->nierAction->Interrupt(leaderTarget))
																{
																	interruptedSet.insert(leaderTarget->GetObjectGuid());
																	interruptDelay = 500;
																	continue;
																}
															}
														}
													}
													if (ns->DoDPS(leaderTarget, false, true))
													{
														continue;
													}
												}
											}
										}
									}
								}
							}
							else if (member->groupRole == GroupRole::GroupRole_Healer)
							{
								if (member->GetHealthPercent() < 50.0f)
								{
									if (ns->DoHeal(member, false))
									{
										continue;
									}
								}
								if (tank)
								{
									if (tank->GetHealthPercent() < 80.0f)
									{
										if (ns->DoHeal(tank, false))
										{
											continue;
										}
									}
								}
								if (lowMember)
								{
									if (ns->DoHeal(lowMember, false))
									{
										continue;
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

Unit* GroupStrategy_Base::GetNeareastOTUnit(Unit* pmTank)
{
	if (!me)
	{
		return nullptr;
	}

	Unit* nearestOTUnit = nullptr;
	float nearestDistance = VISIBILITY_DISTANCE_NORMAL;
	for (GroupReference* groupRef = me->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
	{
		if (Player* member = groupRef->getSource())
		{
			if (member->IsAlive())
			{
				if (member->GetObjectGuid() != pmTank->GetObjectGuid())
				{
					float memberDistance = pmTank->GetDistance(member);
					if (memberDistance < NIER_FAR_DISTANCE)
					{
						std::set<Unit*> memberAttackers = member->GetAttackers();
						for (std::set<Unit*>::iterator ait = memberAttackers.begin(); ait != memberAttackers.end(); ++ait)
						{
							if (Unit* eachAttacker = *ait)
							{
								if (pmTank->IsValidAttackTarget(eachAttacker))
								{
									if (!eachAttacker->GetTargetGuid().IsEmpty())
									{
										if (eachAttacker->GetTargetGuid() != pmTank->GetObjectGuid())
										{
											float eachDistance = pmTank->GetDistance(eachAttacker);
											if (eachDistance < NIER_FAR_DISTANCE)
											{
												if (eachDistance < nearestDistance)
												{
													if (me->GetTargetIconByGuid(eachAttacker->GetObjectGuid()) == -1)
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

	return nearestOTUnit;
}

Unit* GroupStrategy_Base::GetNeareastAttacker(Unit* pmTank)
{
	if (!me)
	{
		return nullptr;
	}

	Unit* nearestAttacker = nullptr;
	float nearestDistance = VISIBILITY_DISTANCE_NORMAL;
	std::set<Unit*> myAttackers = pmTank->GetAttackers();
	for (std::set<Unit*>::iterator ait = myAttackers.begin(); ait != myAttackers.end(); ++ait)
	{
		if (Unit* eachAttacker = *ait)
		{
			if (pmTank->IsValidAttackTarget(eachAttacker))
			{
				if (!sNierManager->IsPolymorphed(eachAttacker))
				{
					float eachDistance = pmTank->GetDistance(eachAttacker);
					if (eachDistance < nearestDistance)
					{
						if (me->GetTargetIconByGuid(eachAttacker->GetObjectGuid()) == -1)
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

	return nearestAttacker;
}