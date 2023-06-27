#include "NierAction_Base.h"
#include "World.h"
#include "Player.h"
#include "Pet.h"
#include "CreatureAI.h"
#include "Spell.h"
#include "GridNotifiers.h"
#include "Map.h"
#include "SpellMgr.h"
#include "SpellAuras.h"
#include "Bag.h"
#include "TargetedMovementGenerator.h"

NierMovement::NierMovement(Player* pmMe)
{
	me = pmMe;
	ogTankTarget = ObjectGuid();
	ogChaseTarget = ObjectGuid();
	ogFollowTarget = ObjectGuid();
	positionTarget = me->GetPosition();
	maxDistance = 0.0f;
	minDistance = 0.0f;
	moveCheckDelay = 0;
	updateCheckDelay = 0;
	activeMovementType = NierMovementType::NierMovementType_None;
}

void NierMovement::ResetMovement()
{
	me->StopMoving();
	me->GetMotionMaster()->Clear();
	ogTankTarget = ObjectGuid();
	ogChaseTarget = ObjectGuid();
	ogFollowTarget = ObjectGuid();
	positionTarget = me->GetPosition();
	minDistance = 0.0f;
	maxDistance = 0.0f;
	moveCheckDelay = 0;
	updateCheckDelay = 0;
	me->m_movementInfo.moveFlags = 0;
	activeMovementType = NierMovementType::NierMovementType_None;
}

void NierMovement::Run()
{
	if (me)
	{
		if (me->GetStandState() != UnitStandStateType::UNIT_STAND_STATE_STAND)
		{
			me->SetStandState(UnitStandStateType::UNIT_STAND_STATE_STAND);
		}
		if (me->IsWalking())
		{
			me->SetWalk(false);
		}
	}
}

void NierMovement::Update_Direct(uint32 pmDiff)
{
	if (updateCheckDelay > 0)
	{
		updateCheckDelay -= pmDiff;
		return;
	}
	updateCheckDelay = DEFAULT_MOVEMENT_UPDATE_DELAY;
	if (!me)
	{
		return;
	}
	if (!me->IsAlive())
	{
		return;
	}
	if (me->HasAuraType(SPELL_AURA_MOD_PACIFY))
	{
		return;
	}
	if (!me->CanFreeMove())
	{
		return;
	}
	if (me->HasUnitState(UnitState::UNIT_STAT_CAN_NOT_MOVE) || me->HasUnitState(UnitState::UNIT_STAT_NOT_MOVE))
	{
		return;
	}
	if (me->IsBeingTeleported())
	{
		return;
	}
	if (me->IsNonMeleeSpellCasted(false))
	{
		return;
	}
	switch (activeMovementType)
	{
	case NierMovementType::NierMovementType_None:
	{
		break;
	}
	case NierMovementType::NierMovementType_Point:
	{
		float distance = me->GetDistance3dToCenter(positionTarget);
		if (distance > VISIBILITY_DISTANCE_LARGE)
		{
			ResetMovement();
		}
		else
		{
			if (distance < CONTACT_DISTANCE)
			{
				me->StopMoving();
				me->GetMotionMaster()->Clear();
				activeMovementType = NierMovementType::NierMovementType_None;
			}
			else
			{
				if (!me->IsMoving())
				{
					me->StopMoving();
					me->GetMotionMaster()->Clear();
					me->GetMotionMaster()->MovePoint(0, positionTarget.x, positionTarget.y, positionTarget.z, MoveOptions::MOVE_PATHFINDING | MoveOptions::MOVE_RUN_MODE);
				}
			}
		}
		break;
	}
	case NierMovementType::NierMovementType_Tank:
	{
		if (Unit* chaseTarget = ObjectAccessor::GetUnit(*me, ogTankTarget))
		{
			float targetDistance = me->GetDistance3dToCenter(chaseTarget->GetPosition());
			if (targetDistance > VISIBILITY_DISTANCE_LARGE)
			{
				ResetMovement();
				break;
			}
			float attackDistance = chaseTarget->GetCombatReach();
			float chaseDistance = attackDistance / 2.0f;
			float meleeMinDistance = std::min(NIER_MIN_DISTANCE, attackDistance);
			if (targetDistance < attackDistance && targetDistance > meleeMinDistance)
			{
				if (me->IsWithinLOSInMap(chaseTarget))
				{
					if (me->IsMoving())
					{
						me->StopMoving();
						me->GetMotionMaster()->Clear();
					}
					if (!me->IsFacingTarget(chaseTarget))
					{
						me->SetFacingToObject(chaseTarget);
					}
					//if (!me->HasInArc(chaseTarget, M_PI / 2))
					//{
					//	me->SetFacingToObject(chaseTarget);
					//}
					break;
				}
				else if (me->IsMoving())
				{
					if (moveCheckDelay > 0)
					{
						moveCheckDelay -= pmDiff;
						break;
					}
					moveCheckDelay = DEFAULT_MOVEMENT_CHECK_DELAY;
				}
				else
				{
					chaseDistance = std::max(targetDistance / 2.0f, NIER_MIN_DISTANCE);
				}
			}
			Position nextPosition;
			chaseTarget->GetNearPoint(nullptr, nextPosition.x, nextPosition.y, nextPosition.z, 0.0f, chaseDistance, chaseTarget->GetAngle(me));
			float dx = nextPosition.x - positionTarget.x;
			float dy = nextPosition.y - positionTarget.y;
			float dz = nextPosition.z - positionTarget.z;
			float dist = sqrt((dx * dx) + (dy * dy) + (dz * dz));
			if (dist > NIER_MIN_DISTANCE)
			{
				//Run();
				//me->StopMoving();
				//me->GetMotionMaster()->Clear();
				positionTarget.x = nextPosition.x;
				positionTarget.y = nextPosition.y;
				positionTarget.z = nextPosition.z;
				me->GetMotionMaster()->MovePoint(0, positionTarget.x, positionTarget.y, positionTarget.z, MoveOptions::MOVE_PATHFINDING | MoveOptions::MOVE_RUN_MODE);
			}
		}
		else
		{
			ResetMovement();
		}
		break;
	}
	case NierMovementType::NierMovementType_Chase:
	{
		if (Unit* chaseTarget = ObjectAccessor::GetUnit(*me, ogChaseTarget))
		{
			float targetDistance = me->GetDistance3dToCenter(chaseTarget->GetPosition());
			if (targetDistance > VISIBILITY_DISTANCE_LARGE)
			{
				ResetMovement();
				break;
			}
			float attackDistance = maxDistance;
			// melee attack 
			if (attackDistance < INTERACTION_DISTANCE)
			{
				attackDistance = chaseTarget->GetCombatReach();
			}
			float chaseDistance = attackDistance / 2.0f;
			float attackMinDistance = std::max(NIER_MIN_DISTANCE, minDistance);
			if (targetDistance < attackDistance && targetDistance > attackMinDistance)
			{
				if (me->IsWithinLOSInMap(chaseTarget))
				{
					if (me->IsMoving())
					{
						me->StopMoving();
						me->GetMotionMaster()->Clear();
					}
					if (!me->IsFacingTarget(chaseTarget))
					{
						me->SetFacingToObject(chaseTarget);
					}
					//if (!me->HasInArc(chaseTarget, M_PI / 2))
					//{
					//	me->SetFacingToObject(chaseTarget);
					//}
					break;
				}
				else if (me->IsMoving())
				{
					if (moveCheckDelay > 0)
					{
						moveCheckDelay -= pmDiff;
						break;
					}
					moveCheckDelay = DEFAULT_MOVEMENT_CHECK_DELAY;
				}
				else
				{
					chaseDistance = std::max(targetDistance / 2.0f, NIER_MIN_DISTANCE);
				}
			}
			Position nextPosition;
			chaseTarget->GetNearPoint(nullptr, nextPosition.x, nextPosition.y, nextPosition.z, 0.0f, chaseDistance, chaseTarget->GetAngle(me));
			float dx = nextPosition.x - positionTarget.x;
			float dy = nextPosition.y - positionTarget.y;
			float dz = nextPosition.z - positionTarget.z;
			float dist = sqrt((dx * dx) + (dy * dy) + (dz * dz));
			if (dist > NIER_MIN_DISTANCE)
			{
				//Run();
				//me->StopMoving();
				//me->GetMotionMaster()->Clear();
				positionTarget.x = nextPosition.x;
				positionTarget.y = nextPosition.y;
				positionTarget.z = nextPosition.z;
				me->GetMotionMaster()->MovePoint(0, positionTarget.x, positionTarget.y, positionTarget.z, MoveOptions::MOVE_PATHFINDING | MoveOptions::MOVE_RUN_MODE);
			}
		}
		else
		{
			ResetMovement();
		}
		break;
	}
	case NierMovementType::NierMovementType_Follow:
	{
		if (Unit* chaseTarget = ObjectAccessor::GetUnit(*me, ogFollowTarget))
		{
			float targetDistance = me->GetDistance3dToCenter(chaseTarget->GetPosition());
			if (targetDistance > VISIBILITY_DISTANCE_LARGE)
			{
				ResetMovement();
				break;
			}
			float chaseDistance = maxDistance;
			if (targetDistance < chaseDistance)
			{
				if (me->IsWithinLOSInMap(chaseTarget))
				{
					if (me->IsMoving())
					{
						me->StopMoving();
						me->GetMotionMaster()->Clear();
					}
					if (!me->IsFacingTarget(chaseTarget))
					{
						me->SetFacingToObject(chaseTarget);
					}
					//if (!me->HasInArc(chaseTarget, M_PI / 2))
					//{
					//	me->SetFacingToObject(chaseTarget);
					//}
					break;
				}
				else if (me->IsMoving())
				{
					if (moveCheckDelay > 0)
					{
						moveCheckDelay -= pmDiff;
						break;
					}
					moveCheckDelay = DEFAULT_MOVEMENT_CHECK_DELAY;
				}
				else
				{
					chaseDistance = targetDistance / 2.0f;
				}
			}
			Position nextPosition;
			chaseTarget->GetNearPoint(nullptr, nextPosition.x, nextPosition.y, nextPosition.z, 0.0f, chaseDistance, chaseTarget->GetAngle(me));
			float dx = nextPosition.x - positionTarget.x;
			float dy = nextPosition.y - positionTarget.y;
			float dz = nextPosition.z - positionTarget.z;
			float dist = sqrt((dx * dx) + (dy * dy) + (dz * dz));
			if (dist > NIER_MIN_DISTANCE)
			{
				//Run();
				//me->StopMoving();
				//me->GetMotionMaster()->Clear();
				positionTarget.x = nextPosition.x;
				positionTarget.y = nextPosition.y;
				positionTarget.z = nextPosition.z;
				me->GetMotionMaster()->MovePoint(0, positionTarget.x, positionTarget.y, positionTarget.z, MoveOptions::MOVE_PATHFINDING | MoveOptions::MOVE_RUN_MODE);
			}
		}
		else
		{
			ResetMovement();
		}
		break;
	}
	default:
	{
		break;
	}
	}
}

bool NierMovement::Tank(Unit* pmTankTarget)
{
	if (me && pmTankTarget)
	{
		float distance = me->GetDistance(pmTankTarget);
		if (distance < VISIBILITY_DISTANCE_NORMAL)
		{
			activeMovementType = NierMovementType::NierMovementType_Tank;
			if (ogTankTarget != pmTankTarget->GetObjectGuid())
			{
				me->StopMoving();
				me->GetMotionMaster()->Clear();
				ogTankTarget = pmTankTarget->GetObjectGuid();
				moveCheckDelay = 0;
			}
			return true;
		}
	}

	return false;
}

bool NierMovement::Chase(Unit* pmChaseTarget, float pmDistanceMax, float pmDistanceMin)
{
	if (me && pmChaseTarget)
	{
		if (me->IsWithinDist(pmChaseTarget, VISIBILITY_DISTANCE_NORMAL))
		{
			maxDistance = pmDistanceMax;
			minDistance = pmDistanceMin;
			activeMovementType = NierMovementType::NierMovementType_Chase;
			if (ogChaseTarget != pmChaseTarget->GetObjectGuid())
			{
				me->StopMoving();
				me->GetMotionMaster()->Clear();
				ogChaseTarget = pmChaseTarget->GetObjectGuid();
				moveCheckDelay = 0;
			}
			return true;
		}
	}

	return false;
}

bool NierMovement::Follow(Unit* pmFollowTarget, float pmDistance)
{
	if (me && pmFollowTarget)
	{
		if (me->IsWithinDistInMap(pmFollowTarget, VISIBILITY_DISTANCE_NORMAL))
		{
			maxDistance = pmDistance;
			activeMovementType = NierMovementType::NierMovementType_Follow;
			if (ogFollowTarget != pmFollowTarget->GetObjectGuid())
			{
				me->StopMoving();
				me->GetMotionMaster()->Clear();
				ogFollowTarget = pmFollowTarget->GetObjectGuid();
				moveCheckDelay = 0;
			}
			return true;
		}
	}

	return false;
}

void NierMovement::Point(Position pmPosTarget, uint32 pmLimit)
{
	if (!me)
	{
		return;
	}
	float distance = me->GetDistance(pmPosTarget.x, pmPosTarget.y, pmPosTarget.z);
	if (distance < VISIBILITY_DISTANCE_NORMAL)
	{
		activeMovementType = NierMovementType::NierMovementType_Point;
		if (me->IsNonMeleeSpellCasted(false))
		{
			me->InterruptSpell(CurrentSpellTypes::CURRENT_GENERIC_SPELL);
			me->InterruptSpell(CurrentSpellTypes::CURRENT_CHANNELED_SPELL);
		}
		positionTarget = pmPosTarget;

		Run();
		me->StopMoving();
		me->GetMotionMaster()->Clear();
		me->GetMotionMaster()->MovePoint(0, positionTarget.x, positionTarget.y, positionTarget.z, MoveOptions::MOVE_PATHFINDING | MoveOptions::MOVE_RUN_MODE);
		moveCheckDelay = DEFAULT_MOVEMENT_UPDATE_DELAY;
	}
}

Position NierMovement::GetDestPosition(Unit* pmTarget, float pmMoveDistance, bool pmCloser)
{
	Position result;

	bool found = false;
	if (pmMoveDistance < VISIBILITY_DISTANCE_NORMAL)
	{
		float moveAngle = me->GetAngle(pmTarget);
		if (!pmCloser)
		{
			moveAngle += M_PI;
		}
		float checkDistance = pmMoveDistance;
		while (checkDistance < NIER_MAX_DISTANCE)
		{
			me->GetNearPoint(me, result.x, result.y, result.z, 0.0f, checkDistance + CONTACT_DISTANCE, moveAngle);
			me->UpdateGroundPositionZ(result.x, result.y, result.z);
			if (pmTarget->IsWithinLOS(result.x, result.y, result.z))
			{
				found = true;
				break;
			}
			checkDistance += MELEE_RANGE;
		}
	}
	if (!found)
	{
		result = pmTarget->GetPosition();
	}

	return result;
}

bool NierMovement::Direction(Unit* pmCommander, uint32 pmDirection, uint32 pmLimit, float pmDistance)
{
	bool result = false;

	if (pmCommander)
	{
		float absAngle = pmCommander->GetOrientation();
		switch (pmDirection)
		{
		case NierMovementDirection_Forward:
		{
			break;
		}
		case NierMovementDirection_Back:
		{
			absAngle = absAngle + M_PI;
			break;
		}
		case NierMovementDirection_Left:
		{
			absAngle = absAngle + M_PI / 2;
			break;
		}
		case NierMovementDirection_Right:
		{
			absAngle = absAngle + M_PI * 3 / 2;
			break;
		}
		default:
		{
			break;
		}
		}
		me->GetNearPoint(me, positionTarget.x, positionTarget.y, positionTarget.z, 0.0f, pmDistance, absAngle);
		activeMovementType = NierMovementType::NierMovementType_Point;
		if (me->IsNonMeleeSpellCasted(false))
		{
			me->InterruptSpell(CurrentSpellTypes::CURRENT_GENERIC_SPELL);
			me->InterruptSpell(CurrentSpellTypes::CURRENT_CHANNELED_SPELL);
		}
		Run();
		me->StopMoving();
		me->GetMotionMaster()->Clear();
		me->GetMotionMaster()->MovePoint(0, positionTarget.x, positionTarget.y, positionTarget.z, MoveOptions::MOVE_PATHFINDING | MoveOptions::MOVE_RUN_MODE);
		result = true;
	}

	return result;
}

bool NierMovement::Direction(float pmAngle, uint32 pmLimit, float pmDistance)
{
	bool result = false;

	me->GetNearPoint(me, positionTarget.x, positionTarget.y, positionTarget.z, 0.0f, pmDistance, pmAngle);
	activeMovementType = NierMovementType::NierMovementType_Point;
	Run();
	if (me->IsNonMeleeSpellCasted(false))
	{
		me->InterruptSpell(CurrentSpellTypes::CURRENT_GENERIC_SPELL);
		me->InterruptSpell(CurrentSpellTypes::CURRENT_CHANNELED_SPELL);
	}
	me->StopMoving();
	me->GetMotionMaster()->Clear();
	me->GetMotionMaster()->MovePoint(0, positionTarget.x, positionTarget.y, positionTarget.z, MoveOptions::MOVE_PATHFINDING | MoveOptions::MOVE_RUN_MODE);
	result = true;

	return result;
}

NierAction_Base::NierAction_Base(Player* pmMe)
{
	me = pmMe;
	nm = new NierMovement(me);
	specialty = 0;
}

void NierAction_Base::Prepare()
{
	if (me)
	{
		me->SetPvP(true);
		me->UpdatePvP(true);
		me->pvpInfo.inPvPCombat = true;
		me->DurabilityRepairAll(false, 0);
		if (!me->GetGroup())
		{
			if (me->GetMap()->Instanceable())
			{
				me->TeleportToHomebind();
			}
		}
	}
}

void NierAction_Base::Reset()
{
	ClearTarget();
}

void NierAction_Base::Update(uint32 pmDiff)
{
	nm->Update_Direct(pmDiff);
	//nm->Update_Chase(pmDiff);
}

bool NierAction_Base::Attack(Unit* pmTarget)
{
	return false;
}

bool NierAction_Base::Interrupt(Unit* pmTarget)
{
	return false;
}

bool NierAction_Base::DPS(Unit* pmTarget, bool pmRushing, bool pmChasing, float pmDistanceMax, float pmDistanceMin)
{
	return false;
}

bool NierAction_Base::Tank(Unit* pmTarget, bool aoe)
{
	return false;
}

bool NierAction_Base::Follow(Unit* pmFollowTarget, float pmDistance)
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
	if (!pmFollowTarget)
	{
		return false;
	}
	if (!nm->Follow(pmFollowTarget, pmDistance))
	{
		if (me->GetTargetGuid() == pmFollowTarget->GetObjectGuid())
		{
			ClearTarget();
		}
		return false;
	}
	ChooseTarget(pmFollowTarget);
}

bool NierAction_Base::Heal(Unit* pmTarget, bool pmInstantOnly)
{
	return false;
}

bool NierAction_Base::ReadyTank(Unit* pmTarget)
{
	return false;
}

bool NierAction_Base::GroupHeal(Unit* pmTarget, bool pmInstantOnly)
{
	return false;
}

bool NierAction_Base::SimpleHeal(Unit* pmTarget, bool pmInstantOnly)
{
	return false;
}

bool NierAction_Base::Cure(Unit* pmTarget)
{
	return false;
}

bool NierAction_Base::Buff(Unit* pmTarget)
{
	return false;
}

uint32 NierAction_Base::Caution()
{
	if (me)
	{
		if (me->IsAlive())
		{
			float startAngle = me->GetOrientation() + M_PI / 4;
			float angleGap = 0.0f;
			Position pos;
			while (angleGap < M_PI * 2)
			{
				me->GetNearPoint(me, pos.x, pos.y, pos.z, 0.0f, NIER_NORMAL_DISTANCE + MELEE_RANGE, startAngle + angleGap);
				if (me->GetDistance(pos.x, pos.y, pos.z) > NIER_NORMAL_DISTANCE - CONTACT_DISTANCE)
				{
					me->InterruptNonMeleeSpells(false);
					me->StopMoving();
					me->GetMotionMaster()->Clear();
					me->GetMotionMaster()->MovePoint(0, pos.x, pos.y, pos.z, MoveOptions::MOVE_PATHFINDING | MoveOptions::MOVE_RUN_MODE);
					return 2000;
				}
			}
		}
	}

	return 0;
}

bool NierAction_Base::Mark(Unit* pmTarget, int pmRTI)
{
	return false;
}

bool NierAction_Base::Assist(int pmRTI)
{
	return false;
}

bool NierAction_Base::Revive(Player* pmTarget)
{
	return false;
}

bool NierAction_Base::Petting(bool pmSummon, bool pmReset)
{
	return false;
}

void NierAction_Base::InitializeCharacter(uint32 pmTargetLevel, uint32 pmSpecialtyTabIndex)
{

}

void NierAction_Base::ResetTalent()
{

}

bool NierAction_Base::InitializeEquipments(bool pmReset)
{
	return true;
}

void NierAction_Base::RemoveEquipments()
{
	if (!me)
	{
		return;
	}
	for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
	{
		if (Item* inventoryItem = me->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
		{
			me->DestroyItem(INVENTORY_SLOT_BAG_0, slot, true);
		}
	}
	for (uint32 checkEquipSlot = EquipmentSlots::EQUIPMENT_SLOT_HEAD; checkEquipSlot < EquipmentSlots::EQUIPMENT_SLOT_TABARD; checkEquipSlot++)
	{
		if (Item* currentEquip = me->GetItemByPos(INVENTORY_SLOT_BAG_0, checkEquipSlot))
		{
			me->DestroyItem(INVENTORY_SLOT_BAG_0, checkEquipSlot, true);
		}
	}
}

void NierAction_Base::LearnTalent(uint32 pmTalentId, uint32 pmMaxRank)
{
	if (!me)
	{
		return;
	}
	uint32 checkRank = 0;
	while (checkRank < pmMaxRank)
	{
		me->LearnTalent(pmTalentId, checkRank);
		checkRank++;
	}
}

void NierAction_Base::TrainSpells(uint32 pmTrainerEntry)
{
	if (CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(pmTrainerEntry))
	{
		if (cInfo->trainer_type == TrainerType::TRAINER_TYPE_CLASS)
		{
			if (cInfo->trainer_class == me->GetClass())
			{
				bool hadNew = false;
				if (const TrainerSpellData* cSpells = sObjectMgr.GetNpcTrainerSpells(cInfo->trainer_id))
				{
					do
					{
						hadNew = false;
						for (const auto& itr : cSpells->spellList)
						{
							TrainerSpell const* eachSpell = &itr.second;
							if (me->GetTrainerSpellState(eachSpell) == TRAINER_SPELL_GREEN)
							{
								hadNew = true;
								SpellEntry const* proto = sSpellMgr.GetSpellEntry(eachSpell->spell);
								me->InterruptSpellsWithChannelFlags(AURA_INTERRUPT_INTERACTING_CANCELS);
								me->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_INTERACTING_CANCELS);
								me->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
								Spell* activeSpell;
								activeSpell = new Spell(me, proto, false);
								SpellCastTargets targets;
								targets.setUnitTarget(me);
								SpellCastResult scr = activeSpell->prepare(std::move(targets));
								activeSpell->update(1); // Update the spell right now. Prevents desynch => take twice the money if you click really fast.
								if (scr == SPELL_CAST_OK)
								{
									me->GetSession()->SendTrainingSuccess(me->GetObjectGuid(), eachSpell->spell);
								}
								else
								{
									me->GetSession()->SendTrainingFailure(me->GetObjectGuid(), eachSpell->spell, TRAIN_FAIL_UNAVAILABLE);
								}
							}
						}
					} while (hadNew);
				}
				if (const TrainerSpellData* tSpells = sObjectMgr.GetNpcTrainerTemplateSpells(cInfo->trainer_id))
				{
					do
					{
						hadNew = false;
						for (const auto& itr : tSpells->spellList)
						{
							TrainerSpell const* eachSpell = &itr.second;
							if (me->GetTrainerSpellState(eachSpell) == TRAINER_SPELL_GREEN)
							{
								hadNew = true;
								SpellEntry const* proto = sSpellMgr.GetSpellEntry(eachSpell->spell);
								for (size_t i = 0; i < MAX_SPELL_REAGENTS; i++)
								{
									if (proto->Reagent[i] > 0)
									{
										if (!me->HasItemCount(proto->Reagent[i], proto->ReagentCount[i]))
										{
											me->StoreNewItemInBestSlots(proto->Reagent[i], proto->ReagentCount[i] * 10);
										}
									}
								}
								me->InterruptSpellsWithChannelFlags(AURA_INTERRUPT_INTERACTING_CANCELS);
								me->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_INTERACTING_CANCELS);
								me->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
								Spell* activeSpell;
								activeSpell = new Spell(me, proto, false);
								SpellCastTargets targets;
								targets.setUnitTarget(me);
								SpellCastResult scr = activeSpell->prepare(std::move(targets));
								activeSpell->update(1); // Update the spell right now. Prevents desynch => take twice the money if you click really fast.
								if (scr == SPELL_CAST_OK)
								{
									me->GetSession()->SendTrainingSuccess(me->GetObjectGuid(), eachSpell->spell);
								}
								else
								{
									me->GetSession()->SendTrainingFailure(me->GetObjectGuid(), eachSpell->spell, TRAIN_FAIL_UNAVAILABLE);
								}
							}
						}
					} while (hadNew);
				}
			}
		}
	}
}

void NierAction_Base::EquipRandomItem(uint32 pmEquipSlot, uint32 pmClass, uint32 pmSubclass, uint32 pmMinQuality, int pmModType, std::unordered_set<uint32> pmInventoryTypeSet)
{
	bool checkStat = true;
	if (pmModType < 0)
	{
		checkStat = false;
	}
	uint32 inventoryType = 0;
	if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_HEAD)
	{
		inventoryType = 1;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_SHOULDERS)
	{
		inventoryType = 3;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_WRISTS)
	{
		inventoryType = 9;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_WAIST)
	{
		inventoryType = 6;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_FEET)
	{
		inventoryType = 8;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_HANDS)
	{
		inventoryType = 10;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_CHEST)
	{
		inventoryType = 5;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_LEGS)
	{
		inventoryType = 7;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_BACK)
	{
		inventoryType = 16;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_NECK)
	{
		inventoryType = 2;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_FINGER1)
	{
		inventoryType = 11;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_FINGER2)
	{
		inventoryType = 11;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_MAINHAND)
	{
		inventoryType = InventoryType::INVTYPE_WEAPON;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_OFFHAND)
	{
		inventoryType = InventoryType::INVTYPE_WEAPON;
	}
	else if (pmEquipSlot == EquipmentSlots::EQUIPMENT_SLOT_RANGED)
	{
		inventoryType = 15;
	}
	else
	{
		return;
	}

	if (pmInventoryTypeSet.size() > 0)
	{
		inventoryType = *pmInventoryTypeSet.begin();
	}
	int maxReqLevel = me->GetLevel();
	int minReqLevel = maxReqLevel - 5;
	while (minReqLevel > 0 && maxReqLevel > 1)
	{
		if (sNierManager->equipsMap.find(inventoryType) != sNierManager->equipsMap.end())
		{
			int activeLevel = urand(minReqLevel, maxReqLevel);
			if (sNierManager->equipsMap[inventoryType][pmSubclass].find(activeLevel) != sNierManager->equipsMap[inventoryType][pmSubclass].end())
			{
				int itemsSize = sNierManager->equipsMap[inventoryType][pmSubclass][activeLevel].size();
				if (itemsSize > 0)
				{
					int itemIndex = urand(0, itemsSize - 1);
					uint32 itemEntry = sNierManager->equipsMap[inventoryType][pmSubclass][activeLevel][itemIndex];
					if (const ItemPrototype* pProto = sObjectMgr.GetItemPrototype(itemEntry))
					{
						bool hasStat = false;
						if (checkStat)
						{
							if (pProto->RandomProperty > 0)
							{
								hasStat = true;
							}
							else
							{
								for (uint32 statIndex = 0; statIndex < MAX_ITEM_PROTO_STATS; statIndex++)
								{
									if (pProto->ItemStat[statIndex].ItemStatType == pmModType)
									{
										hasStat = true;
										break;
									}
								}
							}
						}
						else
						{
							hasStat = true;
						}
						if (hasStat)
						{
							uint16 eDest;
							uint8 msg = me->CanEquipNewItem(NULL_SLOT, eDest, itemEntry, false);
							if (msg == EQUIP_ERR_OK)
							{
								if (Item* pItem = Item::CreateItem(itemEntry, 1, me->GetObjectGuid()))
								{
									if (uint32 randomPropertyId = Item::GenerateItemRandomPropertyId(itemEntry))
									{
										pItem->SetItemRandomProperties(randomPropertyId);
										pItem->SetState(ItemUpdateState::ITEM_NEW, me);
									}
									me->SendNewItem(pItem, 1, true, true);
									ItemPosCountVec sDest;
									uint8 storeResult = me->CanStoreItem(NULL_BAG, NULL_SLOT, sDest, pItem, false);
									if (storeResult == EQUIP_ERR_OK)
									{
										me->StoreItem(sDest, pItem, true);
									}
								}
								for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
								{
									if (Item* targetItem = me->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
									{
										if (targetItem->GetEntry() == itemEntry)
										{
											uint16 dest = 0;
											InventoryResult ir = me->CanEquipItem(NULL_SLOT, dest, targetItem, false);
											if (ir == EQUIP_ERR_OK)
											{
												me->RemoveItem(INVENTORY_SLOT_BAG_0, i, true);
												me->EquipItem(dest, targetItem, true);
												break;
											}
										}
									}
								}
							}

							//if (me->StoreNewItemInBestSlots(itemEntry, 1))
							//{
							//	std::ostringstream msgStream;
							//	msgStream << me->GetName() << " Equiped " << itemEntry;
							//	sWorld.SendServerMessage(ServerMessageType::SERVER_MSG_CUSTOM, msgStream.str().c_str());
							//	sLog.Out(LogType::LOG_BASIC, LogLevel::LOG_LVL_BASIC, msgStream.str().c_str());
							//	return;
							//}
						}
					}
				}
			}
		}
		maxReqLevel = maxReqLevel - 1;
		minReqLevel = maxReqLevel - 5;
	}
}

void NierAction_Base::PetAttack(Unit* pmTarget)
{
	if (me)
	{
		if (Pet* myPet = me->GetPet())
		{
			if (myPet->IsAlive())
			{
				if (CreatureAI* cai = myPet->AI())
				{
					cai->AttackStart(pmTarget);
				}
			}
		}
	}
}

void NierAction_Base::PetStop()
{
	if (me)
	{
		if (Pet* myPet = me->GetPet())
		{
			myPet->AttackStop();
			if (CharmInfo* pci = myPet->GetCharmInfo())
			{
				if (pci->IsCommandAttack())
				{
					pci->SetIsCommandAttack(false);
				}
				if (!pci->IsCommandFollow())
				{
					pci->SetIsCommandFollow(true);
				}
			}
		}
	}
}

bool NierAction_Base::UseItem(Item* pmItem, Unit* pmTarget)
{
	if (!me)
	{
		return false;
	}
	if (me->CanUseItem(pmItem) != EQUIP_ERR_OK)
	{
		return false;
	}

	if (me->IsNonMeleeSpellCasted(false, false, true))
	{
		return false;
	}

	if (const ItemPrototype* proto = pmItem->GetProto())
	{
		ChooseTarget(pmTarget);
		SpellCastTargets targets;
		targets.Update(pmTarget);
		me->CastItemUseSpell(pmItem, targets);
		return true;
	}

	return false;
}

bool NierAction_Base::UseItem(Item* pmItem, Item* pmTarget)
{
	if (!me)
	{
		return false;
	}
	if (me->CanUseItem(pmItem) != EQUIP_ERR_OK)
	{
		return false;
	}
	if (me->IsNonMeleeSpellCasted(false, false, true))
	{
		return false;
	}
	if (!pmTarget)
	{
		return false;
	}

	if (const ItemPrototype* proto = pmItem->GetProto())
	{
		SpellCastTargets targets;
		targets.setItemTarget(pmTarget);
		me->CastItemUseSpell(pmItem, targets);
		return true;
	}

	return false;
}

bool NierAction_Base::CastSpell(Unit* pmTarget, uint32 pmSpellId, bool pmCheckAura, bool pmOnlyMyAura, bool pmClearShapeShift, uint32 pmMaxAuraStack)
{
	if (!SpellValid(pmSpellId))
	{
		return false;
	}
	if (!me)
	{
		return false;
	}
	if (me->IsNonMeleeSpellCasted(false, false, true))
	{
		return true;
	}
	if (pmClearShapeShift)
	{
		me->RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT);
	}
	if (const SpellEntry* pS = sSpellMgr.GetSpellEntry(pmSpellId))
	{
		if (pmTarget)
		{
			if (!me->IsWithinLOSInMap(pmTarget))
			{
				return false;
			}
			if (pmTarget->IsImmuneToSpell(pS, false))
			{
				return false;
			}
			if (pmCheckAura)
			{
				if (pmOnlyMyAura)
				{
					if (sNierManager->HasAura(pmTarget, pmSpellId, me))
					{
						return false;
					}
				}
				else
				{
					if (sNierManager->HasAura(pmTarget, pmSpellId))
					{
						return false;
					}
				}
			}
			if (Spell* checkSpell = new Spell(me, pS, false))
			{
				SpellCastResult scr = checkSpell->CheckCast(true);
				if (scr != SpellCastResult::SPELL_CAST_OK)
				{
					return false;
				}
			}
			if (!me->HasInArc(pmTarget, M_PI / 2))
			{
				me->SetFacingToObject(pmTarget);
			}
			if (me->GetTargetGuid() != pmTarget->GetObjectGuid())
			{
				ChooseTarget(pmTarget);
			}
		}
		for (size_t i = 0; i < MAX_SPELL_REAGENTS; i++)
		{
			if (pS->Reagent[i] > 0)
			{
				if (!me->HasItemCount(pS->Reagent[i], pS->ReagentCount[i]))
				{
					me->StoreNewItemInBestSlots(pS->Reagent[i], pS->ReagentCount[i] * 10);
				}
			}
		}
		if (me->GetStandState() != UnitStandStateType::UNIT_STAND_STATE_STAND)
		{
			me->SetStandState(UNIT_STAND_STATE_STAND);
		}
		//me->CastSpell(pmTarget, pS, false);
		//return true;

		SpellCastResult scr = me->CastSpell(pmTarget, pS->Id, false);
		if (scr == SpellCastResult::SPELL_CAST_OK)
		{
			return true;
		}
	}

	return false;
}

void NierAction_Base::CancelAura(uint32 pmSpellID)
{
	if (pmSpellID == 0)
	{
		return;
	}
	if (!me)
	{
		return;
	}
	me->RemoveAurasDueToSpell(pmSpellID);
}

bool NierAction_Base::Eat()
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	else if (me->IsInCombat())
	{
		return false;
	}
	uint32 foodEntry = 0;
	uint32 myLevel = me->GetLevel();
	if (myLevel >= 75)
	{
		foodEntry = 35950;
	}
	else if (myLevel >= 65)
	{
		foodEntry = 33449;
	}
	else if (myLevel >= 55)
	{
		foodEntry = 21023;
	}
	else if (myLevel >= 45)
	{
		foodEntry = 8950;
	}
	else if (myLevel >= 35)
	{
		foodEntry = 4601;
	}
	else if (myLevel >= 25)
	{
		foodEntry = 4544;
	}
	else if (myLevel >= 15)
	{
		foodEntry = 4542;
	}
	else if (myLevel >= 5)
	{
		foodEntry = 4541;
	}
	else
	{
		foodEntry = 4540;
	}
	if (!me->HasItemCount(foodEntry, 1))
	{
		me->StoreNewItemInBestSlots(foodEntry, 20);
	}
	me->CombatStop(true);
	me->StopMoving();
	me->GetMotionMaster()->Clear();
	ClearTarget();

	Item* pFood = GetItemInInventory(foodEntry);
	if (pFood && !pFood->IsInTrade())
	{
		if (UseItem(pFood, me))
		{
			nm->ResetMovement();
			return true;
		}
	}
	return false;
}

bool NierAction_Base::Drink()
{
	if (!me)
	{
		return false;
	}
	if (!me->IsAlive())
	{
		return false;
	}
	if (me->IsInCombat())
	{
		return false;
	}
	uint32 drinkEntry = 0;
	uint32 myLevel = me->GetLevel();
	if (myLevel >= 75)
	{
		drinkEntry = 33445;
	}
	else if (myLevel >= 70)
	{
		drinkEntry = 33444;
	}
	else if (myLevel >= 65)
	{
		drinkEntry = 27860;
	}
	else if (myLevel >= 60)
	{
		drinkEntry = 28399;
	}
	else if (myLevel >= 55)
	{
		drinkEntry = 18300;
	}
	else if (myLevel >= 45)
	{
		drinkEntry = 8766;
	}
	else if (myLevel >= 35)
	{
		drinkEntry = 1645;
	}
	else if (myLevel >= 25)
	{
		drinkEntry = 1708;
	}
	else if (myLevel >= 15)
	{
		drinkEntry = 1205;
	}
	else if (myLevel >= 5)
	{
		drinkEntry = 1179;
	}
	else
	{
		drinkEntry = 159;
	}

	if (!me->HasItemCount(drinkEntry, 1))
	{
		me->StoreNewItemInBestSlots(drinkEntry, 20);
	}
	me->CombatStop(true);
	me->StopMoving();
	me->GetMotionMaster()->Clear();
	ClearTarget();
	Item* pDrink = GetItemInInventory(drinkEntry);
	if (pDrink && !pDrink->IsInTrade())
	{
		if (UseItem(pDrink, me))
		{
			nm->ResetMovement();
			return true;
		}
	}

	return false;
}

bool NierAction_Base::HealthPotion()
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	else if (!me->IsInCombat())
	{
		return false;
	}
	uint32 potionEntry = 0;
	uint32 myLevel = me->GetLevel();
	if (myLevel >= 70)
	{
		potionEntry = 33447;
	}
	else if (myLevel >= 55)
	{
		potionEntry = 22829;
	}
	else if (myLevel >= 45)
	{
		potionEntry = 13446;
	}
	else if (myLevel >= 35)
	{
		potionEntry = 3928;
	}
	else if (myLevel >= 21)
	{
		potionEntry = 1710;
	}
	else if (myLevel >= 12)
	{
		potionEntry = 929;
	}
	else if (myLevel >= 3)
	{
		potionEntry = 858;
	}
	else
	{
		potionEntry = 118;
	}
	if (!me->HasItemCount(potionEntry, 1))
	{
		me->StoreNewItemInBestSlots(potionEntry, 20);
	}
	Item* pPotion = GetItemInInventory(potionEntry);
	if (pPotion && !pPotion->IsInTrade())
	{
		if (UseItem(pPotion, me))
		{
			return true;
		}
	}
	return false;
}

bool NierAction_Base::ManaPotion()
{
	if (!me)
	{
		return false;
	}
	else if (!me->IsAlive())
	{
		return false;
	}
	else if (!me->IsInCombat())
	{
		return false;
	}
	uint32 potionEntry = 0;
	uint32 myLevel = me->GetLevel();
	if (myLevel >= 70)
	{
		potionEntry = 33448;
	}
	else if (myLevel >= 55)
	{
		potionEntry = 22832;
	}
	else if (myLevel >= 49)
	{
		potionEntry = 13444;
	}
	else if (myLevel >= 41)
	{
		potionEntry = 13443;
	}
	else if (myLevel >= 31)
	{
		potionEntry = 6149;
	}
	else if (myLevel >= 22)
	{
		potionEntry = 3827;
	}
	else if (myLevel >= 14)
	{
		potionEntry = 3385;
	}
	else if (myLevel >= 5)
	{
		potionEntry = 2455;
	}
	if (potionEntry > 0)
	{
		if (!me->HasItemCount(potionEntry, 1))
		{
			me->StoreNewItemInBestSlots(potionEntry, 20);
		}
		Item* pPotion = GetItemInInventory(potionEntry);
		if (pPotion && !pPotion->IsInTrade())
		{
			if (UseItem(pPotion, me))
			{
				return true;
			}
		}
	}

	return false;
}

bool NierAction_Base::RandomTeleport()
{
	int myLevel = me->GetLevel();
	std::unordered_map<uint32, ObjectGuid> samePlayersMap;
	std::unordered_map<uint32, WorldSession*> allSessions = sWorld.GetAllSessions();
	for (std::unordered_map<uint32, WorldSession*>::iterator wsIT = allSessions.begin(); wsIT != allSessions.end(); wsIT++)
	{
		if (WorldSession* eachWS = wsIT->second)
		{
			if (!eachWS->isNier)
			{
				if (Player* eachPlayer = eachWS->GetPlayer())
				{
					if (Map* map = eachPlayer->GetMap())
					{
						if (!map->Instanceable())
						{
							int eachLevel = eachPlayer->GetLevel();
							if (myLevel > eachLevel - 5 && myLevel < eachLevel + 5)
							{
								//if (me->IsHostileTo(eachPlayer))
								//{
								//    samePlayersMap[samePlayersMap.size()] = eachPlayer->GetObjectGuid();
								//}
								samePlayersMap[samePlayersMap.size()] = eachPlayer->GetObjectGuid();
							}
						}
					}
				}
			}
		}
	}
	if (samePlayersMap.size() > 0)
	{
		uint32 targetPlayerIndex = urand(0, samePlayersMap.size() - 1);
		if (Player* targetPlayer = ObjectAccessor::FindPlayer(samePlayersMap[targetPlayerIndex]))
		{
			float nearX = 0.0f;
			float nearY = 0.0f;
			float nearZ = 0.0f;
			float nearDistance = frand(100.0f, 300.0f);
			float nearAngle = frand(0.0f, M_PI * 2);
			targetPlayer->GetNearPoint(targetPlayer, nearX, nearY, nearZ, 0.0f, nearDistance, nearAngle);
			if (!me->IsAlive())
			{
				me->ResurrectPlayer(1.0f);
			}
			me->SetPvP(true);
			me->UpdatePvP(true);
			me->pvpInfo.inPvPCombat = true;
			me->DurabilityRepairAll(false, 0);
			me->ClearInCombat();
			ClearTarget();
			nm->ResetMovement();
			me->TeleportTo(targetPlayer->GetMapId(), nearX, nearY, nearZ, 0);
			return true;
		}
	}

	return false;
}

void NierAction_Base::ChooseTarget(Unit* pmTarget)
{
	if (pmTarget)
	{
		if (me)
		{
			me->SetSelectionGuid(pmTarget->GetObjectGuid());
			me->SetTargetGuid(pmTarget->GetObjectGuid());
		}
	}
}

void NierAction_Base::ClearTarget()
{
	if (me)
	{
		me->SetSelectionGuid(ObjectGuid());
		me->SetTargetGuid(ObjectGuid());
		me->AttackStop();
		me->InterruptNonMeleeSpells(true);
	}
}

bool NierAction_Base::SpellValid(uint32 pmSpellID)
{
	if (pmSpellID == 0)
	{
		return false;
	}
	if (!me)
	{
		return false;
	}
	if (me->HasSpellCooldown(pmSpellID))
	{
		return false;
	}

	return true;
}

Item* NierAction_Base::GetItemInInventory(uint32 pmEntry)
{
	if (!me)
	{
		return NULL;
	}
	for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
	{
		Item* pItem = me->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
		if (pItem)
		{
			if (pItem->GetEntry() == pmEntry)
			{
				return pItem;
			}
		}
	}

	for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
	{
		if (Bag* pBag = (Bag*)me->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
		{
			for (uint32 j = 0; j < pBag->GetBagSize(); j++)
			{
				Item* pItem = me->GetItemByPos(i, j);
				if (pItem)
				{
					if (pItem->GetEntry() == pmEntry)
					{
						return pItem;
					}
				}
			}
		}
	}

	return NULL;
}

Unit* NierAction_Base::GetNearbyHostileUnit(float pmRange)
{
	std::list<Creature*> creatureList;
	me->GetCreatureListWithEntryInGrid(creatureList, 0, pmRange);
	if (!creatureList.empty())
	{
		for (std::list<Creature*>::iterator itr = creatureList.begin(); itr != creatureList.end(); ++itr)
		{
			if (Creature* hostileCreature = *itr)
			{
				if (hostileCreature->IsAlive())
				{
					if (!hostileCreature->IsCivilian())
					{
						if (me->IsValidAttackTarget(hostileCreature))
						{
							return hostileCreature;
						}
					}
				}
			}
		}
	}

	return nullptr;
}
