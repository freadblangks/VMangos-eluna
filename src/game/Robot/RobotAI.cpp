#include "RobotAI.h"
#include "Strategy_Solo.h"
#include "Group.h"
#include "Strategy_Group_Azuregos.h"
#include "Strategy_Group_BlackrockSpire.h"
#include "Strategy_Group_DoctorWeavil.h"
#include "Strategy_Group_Emeriss.h"
#include "Strategy_Group_Lethon.h"
#include "Strategy_Group_MoltenCore.h"
#include "Strategy_Group_Taerar.h"
#include "Strategy_Group_Test.h"
#include "Strategy_Group_Ysondre.h"

RobotMovement::RobotMovement(Player* pmMe)
{
	me = pmMe;
	chaseTarget = NULL;
	activeMovementType = RobotMovementType::RobotMovementType_None;
	chaseDistanceMin = CONTACT_DISTANCE;
	chaseDistanceMax = VISIBILITY_DISTANCE_NORMAL;
	checkDelay = 0;
	limitDelay = 0;
}

void RobotMovement::ResetMovement()
{
	chaseTarget = NULL;
	activeMovementType = RobotMovementType::RobotMovementType_None;
	chaseDistanceMin = CONTACT_DISTANCE;
	chaseDistanceMax = VISIBILITY_DISTANCE_NORMAL;
	checkDelay = 0;
	limitDelay = 0;
	if (me)
	{
		me->GetMotionMaster()->Clear();
		me->StopMoving();
	}
}

bool RobotMovement::Chase(Unit* pmChaseTarget, float pmChaseDistanceMax, float pmChaseDistanceMin, uint32 pmLimitDelay)
{
	limitDelay = pmLimitDelay;
	if (!me)
	{
		return false;
	}
	if (!me->IsAlive())
	{
		return false;
	}
	if (me->HasAuraType(SPELL_AURA_MOD_PACIFY))
	{
		return false;
	}
	if (me->HasUnitState(UnitState::UNIT_STAT_NOT_MOVE))
	{
		return false;
	}
	if (me->IsNonMeleeSpellCasted(false, false, true))
	{
		return false;
	}
	if (!pmChaseTarget)
	{
		return false;
	}
	if (me->GetMapId() != pmChaseTarget->GetMapId())
	{
		return false;
	}
	float unitTargetDistance = me->GetDistance3dToCenter(pmChaseTarget);
	if (unitTargetDistance > VISIBILITY_DISTANCE_LARGE)
	{
		return false;
	}
	if (pmChaseTarget->GetTypeId() == TypeID::TYPEID_PLAYER)
	{
		if (Player* targetPlayer = pmChaseTarget->ToPlayer())
		{
			if (targetPlayer->IsBeingTeleported())
			{
				return false;
			}
		}
	}	
	if (activeMovementType == RobotMovementType::RobotMovementType_Chase)
	{
		if (chaseTarget)
		{
			if (chaseTarget->GetObjectGuid() == pmChaseTarget->GetObjectGuid())
			{
				return true;
			}
		}
	}
	if (me->IsMoving())
	{
		me->StopMoving();
		me->GetMotionMaster()->Clear();
	}
	chaseTarget = pmChaseTarget;
	chaseDistanceMax = pmChaseDistanceMax;
	chaseDistanceMin = pmChaseDistanceMin;
	activeMovementType = RobotMovementType::RobotMovementType_Chase;

	if (unitTargetDistance >= chaseDistanceMin && unitTargetDistance <= chaseDistanceMax + MELEE_MAX_DISTANCE)
	{
		if (me->IsWithinLOSInMap(chaseTarget))
		{
			if (!me->HasInArc(M_PI / 4, chaseTarget))
			{
				me->SetFacingToObject(chaseTarget);
			}
		}
	}
	float distanceInRange = frand(chaseDistanceMin, chaseDistanceMax);
	me->GetMotionMaster()->MoveDistance(chaseTarget, distanceInRange);
	return true;
}

void RobotMovement::MovePosition(Position pmTargetPosition, uint32 pmLimitDelay)
{
	MovePosition(pmTargetPosition.x, pmTargetPosition.y, pmTargetPosition.z, pmLimitDelay);
}

void RobotMovement::MovePosition(float pmX, float pmY, float pmZ, uint32 pmLimitDelay)
{
	limitDelay = pmLimitDelay;
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
	if (me->HasUnitState(UnitState::UNIT_STAT_NOT_MOVE))
	{
		return;
	}
	if (me->HasUnitState(UnitState::UNIT_STAT_ROAMING_MOVE))
	{
		return;
	}
	if (me->IsNonMeleeSpellCasted(false, false, true))
	{
		return;
	}
	if (me->IsBeingTeleported())
	{
		ResetMovement();
		return;
	}
	if (activeMovementType == RobotMovementType::RobotMovementType_Point)
	{
		float dx = pointTarget.x - pmX;
		float dy = pointTarget.y - pmY;
		float dz = pointTarget.z - pmZ;
		float gap = sqrt((dx * dx) + (dy * dy) + (dz * dz));
		if (gap < CONTACT_DISTANCE)
		{
			return;
		}
	}
	float distance = me->GetDistance(pmX, pmY, pmZ);
	if (distance >= 0.0f && distance <= VISIBILITY_DISTANCE_LARGE)
	{
		pointTarget.x = pmX;
		pointTarget.y = pmY;
		pointTarget.z = pmZ;
		activeMovementType = RobotMovementType::RobotMovementType_Point;
		MovePoint(pointTarget.x, pointTarget.y, pointTarget.z);
	}
}

void RobotMovement::MovePoint(float pmX, float pmY, float pmZ)
{
	if (me)
	{
		if (me->GetStandState() != UnitStandStateType::UNIT_STAND_STATE_STAND)
		{
			me->SetStandState(UnitStandStateType::UNIT_STAND_STATE_STAND);
		}
		me->GetMotionMaster()->MovePoint(1, pmX, pmY, pmZ, MoveOptions::MOVE_PATHFINDING | MoveOptions::MOVE_RUN_MODE);
	}
}

void RobotMovement::Update(uint32 pmDiff)
{
	checkDelay += pmDiff;
	if (checkDelay < MOVEMENT_CHECK_DELAY)
	{
		return;
	}
	checkDelay = 0;
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
	if (me->HasUnitState(UnitState::UNIT_STAT_NOT_MOVE))
	{
		return;
	}
	if (me->HasUnitState(UnitState::UNIT_STAT_ROAMING_MOVE))
	{
		return;
	}
	if (me->IsNonMeleeSpellCasted(false, false, true))
	{
		return;
	}
	if (me->IsBeingTeleported())
	{
		ResetMovement();
		return;
	}
	checkDelay += pmDiff;
	if (limitDelay > 0)
	{
		limitDelay -= pmDiff;
		if (limitDelay <= 0)
		{
			ResetMovement();
		}
	}
	switch (activeMovementType)
	{
	case RobotMovementType::RobotMovementType_None:
	{
		break;
	}
	case RobotMovementType::RobotMovementType_Point:
	{
		float distance = me->GetDistance3dToCenter(pointTarget);
		if (distance > VISIBILITY_DISTANCE_LARGE || distance < CONTACT_DISTANCE)
		{
			ResetMovement();
		}
		else
		{
			if (!me->IsMoving())
			{
				MovePoint(pointTarget.x, pointTarget.y, pointTarget.z);
			}
		}
		break;
	}
	case RobotMovementType::RobotMovementType_Chase:
	{
		if (!chaseTarget)
		{
			ResetMovement();
			break;
		}
		if (me->GetMapId() != chaseTarget->GetMapId())
		{
			ResetMovement();
			break;
		}
		if (chaseTarget->GetTypeId() == TypeID::TYPEID_PLAYER)
		{
			if (Player* targetPlayer = chaseTarget->ToPlayer())
			{
				if (!targetPlayer->IsInWorld())
				{
					ResetMovement();
					break;
				}
				else if (targetPlayer->IsBeingTeleported())
				{
					ResetMovement();
					break;
				}
			}
		}
		float unitTargetDistance = me->GetDistance3dToCenter(chaseTarget);
		if (unitTargetDistance > VISIBILITY_DISTANCE_LARGE)
		{
			ResetMovement();
			break;
		}
		if (!me->IsMoving())
		{
			bool ok = true;
			if (unitTargetDistance >= chaseDistanceMin && unitTargetDistance <= chaseDistanceMax + MELEE_MAX_DISTANCE)
			{
				if (me->IsWithinLOSInMap(chaseTarget))
				{
					if (!me->HasInArc(M_PI / 4, chaseTarget))
					{
						me->SetFacingToObject(chaseTarget);
					}
				}
				else
				{
					ok = false;
				}
			}
			else
			{
				ok = false;
			}

			if (!ok)
			{
				float distanceInRange = frand(chaseDistanceMin, chaseDistanceMax);
				me->GetMotionMaster()->MoveDistance(chaseTarget, distanceInRange);
				//chaseTarget->GetNearPoint(chaseTarget, pointTarget.x, pointTarget.y, pointTarget.z, 0, distanceInRange, me->GetAngle(chaseTarget));
				//MovePoint(pointTarget.x, pointTarget.y, pointTarget.z);
			}
		}
		break;
	}
	default:
	{
		break;
	}
	}
}

RobotAI::RobotAI(Player* pmMe)
{
	me = pmMe;
	rm = new RobotMovement(me);
	checkDelay = 0;
	robotType = 0;

	strategyMap.clear();

	Strategy_Solo* rss = new Strategy_Solo(me);
	strategyMap[Strategy_Index::Strategy_Index_Solo] = rss;

	Strategy_Group* rsg = new Strategy_Group(me);
	strategyMap[Strategy_Index::Strategy_Index_Group] = rsg;

	Strategy_Group_BlackrockSpire* rsBlackrockSpire = new Strategy_Group_BlackrockSpire(me);
	strategyMap[Strategy_Index::Strategy_Index_Group_BlackrockSpire] = rsBlackrockSpire;

	Strategy_Group_DoctorWeavil* rsDoctorWeavil = new Strategy_Group_DoctorWeavil(me);
	strategyMap[Strategy_Index::Strategy_Index_Group_DoctorWeavil] = rsDoctorWeavil;

	Strategy_Group_Emeriss* rsEmeriss = new Strategy_Group_Emeriss(me);
	strategyMap[Strategy_Index::Strategy_Index_Group_Emeriss] = rsEmeriss;

	Strategy_Group_Lethon* rsLethon = new Strategy_Group_Lethon(me);
	strategyMap[Strategy_Index::Strategy_Index_Group_Lethon] = rsLethon;

	Strategy_Group_Taerar* rsTaerar = new Strategy_Group_Taerar(me);
	strategyMap[Strategy_Index::Strategy_Index_Group_Taerar] = rsTaerar;

	Strategy_Group_Ysondre* rsYsondre = new Strategy_Group_Ysondre(me);
	strategyMap[Strategy_Index::Strategy_Index_Group_Ysondre] = rsYsondre;

	Strategy_Group_Azuregos* rsAzuregos = new Strategy_Group_Azuregos(me);
	strategyMap[Strategy_Index::Strategy_Index_Group_Azuregos] = rsAzuregos;

	Strategy_Group_MoltenCore* rsMoltenCore = new Strategy_Group_MoltenCore(me);
	strategyMap[Strategy_Index::Strategy_Index_Group_MoltenCore] = rsMoltenCore;

	Strategy_Group_Test* rsTest = new Strategy_Group_Test(me);
	strategyMap[Strategy_Index::Strategy_Index_Group_Test] = rsTest;
}

void RobotAI::ResetStrategies()
{
	if (rm)
	{
		rm->ResetMovement();
	}
	for (std::unordered_map<uint32, Strategy_Base*>::iterator stIT = strategyMap.begin(); stIT != strategyMap.end(); stIT++)
	{
		if (Strategy_Base* eachST = stIT->second)
		{
			if (eachST->sb)
			{
				eachST->sb->IdentifyCharacterSpells();
				eachST->sb->Reset();
			}
		}
	}
}

void RobotAI::Update(uint32 pmDiff)
{
	checkDelay += pmDiff;
	if (rm)
	{
		rm->Update(pmDiff);
	}
	if (checkDelay > AI_CHECK_DELAY)
	{
		if (Group* myGroup = me->GetGroup())
		{
			strategyMap[myGroup->groupStrategyIndex]->Update(checkDelay);
		}
		else
		{
			strategyMap[Strategy_Index::Strategy_Index_Solo]->Update(checkDelay);
		}
		checkDelay = 0;
	}
}
