#include "Common.h"
#include "AirManager.h"

AirManager::AirManager() { }

void AirManager::executeMicro(const UnitVector & targets)
{
	const UnitVector & airUnits = getUnits();

	// figure out targets
	UnitVector airUnitTargets;
	for (size_t i(0); i<targets.size(); i++)
	{
		// conditions for targeting
		if (targets[i]->isVisible())
		{
			airUnitTargets.push_back(targets[i]);
		}
	}

	// for each air unit
	BOOST_FOREACH(BWAPI::Unit * airUnit, airUnits)
	{
		// train sub units such as scarabs or interceptors
		trainSubUnits(airUnit);

		// if the order is to attack or defend
		if (order.type == order.Attack || order.type == order.Defend) {

			// if there are targets
			if (!airUnitTargets.empty())
			{
				// find the best target for this air unit
				BWAPI::Unit * target = getTarget(airUnit, airUnitTargets);

				// attack it
				kiteTarget(airUnit, target);
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (airUnit->getDistance(order.position) > 100)
				{
					// move to it
					smartAttackMove(airUnit, order.position);
				}
			}
		}

		if (Options::Debug::DRAW_UALBERTABOT_DEBUG)
		{
			BWAPI::Broodwar->drawLineMap(airUnit->getPosition().x(), airUnit->getPosition().y(),
				airUnit->getTargetPosition().x(), airUnit->getTargetPosition().y(), Options::Debug::COLOR_LINE_TARGET);
		}
	}
}

void AirManager::kiteTarget(BWAPI::Unit * airUnit, BWAPI::Unit * target)
{

	// if we can't kite it, there's no point
	smartAttackUnit(airUnit, target);
	return;

	double range(airUnit->getType().groundWeapon().maxRange());
	if (airUnit->getType() == BWAPI::UnitTypes::Protoss_Dragoon && BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Singularity_Charge))
	{
		range = 6 * 32;
	}

	// determine whether the target can be kited
	if (range <= target->getType().groundWeapon().maxRange())
	{
		// if we can't kite it, there's no point
		smartAttackUnit(airUnit, target);
		return;
	}

	double		minDist(64);
	bool		kite(true);
	double		dist(airUnit->getDistance(target));
	double		speed(airUnit->getType().topSpeed());

	double	timeToEnter = std::max(0.0, (dist - range) / speed);
	if ((timeToEnter >= airUnit->getGroundWeaponCooldown()) && (dist >= minDist))
	{
		kite = false;
	}

	if (target->getType().isBuilding() && target->getType() != BWAPI::UnitTypes::Terran_Bunker)
	{
		kite = false;
	}

	if (airUnit->isSelected())
	{
		BWAPI::Broodwar->drawCircleMap(airUnit->getPosition().x(), airUnit->getPosition().y(),
			(int)range, BWAPI::Colors::Cyan);
	}

	// if we can't shoot, run away
	if (kite)
	{
		BWAPI::Position fleePosition(airUnit->getPosition() - target->getPosition() + airUnit->getPosition());

		BWAPI::Broodwar->drawLineMap(airUnit->getPosition().x(), airUnit->getPosition().y(),
			fleePosition.x(), fleePosition.y(), BWAPI::Colors::Cyan);

		smartMove(airUnit, fleePosition);
	}
	// otherwise shoot
	else
	{
		smartAttackUnit(airUnit, target);
	}
}

// get a target for the air unit to attack
BWAPI::Unit * AirManager::getTarget(BWAPI::Unit * airUnit, UnitVector & targets)
{
	int range(airUnit->getType().groundWeapon().maxRange());

	int highestInRangePriority(0);
	int highestNotInRangePriority(0);
	int lowestInRangeHitPoints(10000);
	int lowestNotInRangeDistance(10000);

	//int lowestInRangeBuildingHitPoints(100000);
	//int lowestNotInRangeBuildingHitPoints(100000);

	BWAPI::Unit * inRangeTarget = NULL;
	BWAPI::Unit * notInRangeTarget = NULL;
	//BWAPI::Unit * inRangeBuilding = NULL;
	//BWAPI::Unit * notInRangeBuilding = NULL;

	BOOST_FOREACH(BWAPI::Unit * unit, targets)
	{
		int priority = getAttackPriority(airUnit, unit);
		int distance = airUnit->getDistance(unit);

		// if the unit is in range, update the target with the lowest hp
		if (airUnit->getDistance(unit) <= range)
		{
			/*if (unit->getType().isBuilding() && unit->getHitPoints() < lowestInRangeBuildingHitPoints)
			{
				inRangeBuilding = unit;
			}*/

			if (priority > highestInRangePriority ||
				(priority == highestInRangePriority && unit->getHitPoints() < lowestInRangeHitPoints))
			{
				lowestInRangeHitPoints = unit->getHitPoints();
				highestInRangePriority = priority;
				inRangeTarget = unit;
			}
		}
		// otherwise it isn't in range so see if it's closest
		else
		{
			/*if (unit->getType().isBuilding() && unit->getHitPoints() < lowestNotInRangeBuildingHitPoints)
			{
				notInRangeBuilding = unit;
			}*/

			if (priority > highestNotInRangePriority ||
				(priority == highestNotInRangePriority && distance < lowestNotInRangeDistance))
			{
				lowestNotInRangeDistance = distance;
				highestNotInRangePriority = priority;
				notInRangeTarget = unit;
			}
		}
	}

	//if (inRangeBuilding != NULL || notInRangeBuilding != NULL)
		//return (inRangeBuilding != NULL) ? inRangeBuilding : notInRangeBuilding;

	// if there is a highest priority unit in range, attack it first
	return (highestInRangePriority >= highestNotInRangePriority) ? inRangeTarget : notInRangeTarget;
}

// get the attack priority of a type in relation to a zergling
int AirManager::getAttackPriority(BWAPI::Unit * airUnit, BWAPI::Unit * target)
{
	BWAPI::UnitType airUnitType = airUnit->getType();
	BWAPI::UnitType targetType = target->getType();

	bool canAttackUs = airUnitType.isFlyer() ? targetType.airWeapon() != BWAPI::WeaponTypes::None : targetType.groundWeapon() != BWAPI::WeaponTypes::None;



	// highest priority is something that can attack us or aid in combat
	if (targetType == BWAPI::UnitTypes::Terran_Medic || canAttackUs ||
		targetType == BWAPI::UnitTypes::Terran_Bunker)
	{
		return 3;
	}
	// next priority is worker
	else if (targetType.isWorker())
	{
		return 2;
	}
	// then everything else
	else
	{
		return 1;
	}
}

BWAPI::Unit * AirManager::closestrangedUnit(BWAPI::Unit * target, std::set<BWAPI::Unit *> & airUnitsToAssign)
{
	double minDistance = 0;
	BWAPI::Unit * closest = NULL;

	BOOST_FOREACH(BWAPI::Unit * airUnit, airUnitsToAssign)
	{
		double distance = airUnit->getDistance(target);
		if (!closest || distance < minDistance)
		{
			minDistance = distance;
			closest = airUnit;
		}
	}

	return closest;
}
