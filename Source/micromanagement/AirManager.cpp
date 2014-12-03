#include "Common.h"
#include "AirManager.h"

AirManager::AirManager() { }

void AirManager::executeMicro(const UnitVector & targets)
{
	BWTA::BaseLocation * enemyLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
	BWTA::Region * enemyRegion = enemyLocation->getRegion();

	const UnitVector & airUnits = getUnits();
	int squadSize = airUnits.size();

	// figure out targets
	UnitVector airUnitTargets;
	for (size_t i(0); i<targets.size(); i++)
	{
		// conditions for targeting
		if (targets[i]->isVisible() && isValidTarget(targets[i]))
		{
			airUnitTargets.push_back(targets[i]);
		}
	}

	if (squadSize < 3)
		return;

	// for each air unit
	BOOST_FOREACH(BWAPI::Unit * airUnit, airUnits)
	{
		// train sub units such as scarabs or interceptors
		trainSubUnits(airUnit);

		// if the order is to attack or defend
		if (order.type == order.Attack || order.type == order.Defend)
		{
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
					//smartMove(airUnit, enemyRegion->getCenter());
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
	double range = airUnit->getType().airWeapon().maxRange();

	// determine whether the target can be kited
	if (range <= target->getType().airWeapon().maxRange())
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
	if ((timeToEnter >= airUnit->getAirWeaponCooldown()) && (dist >= minDist))
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
	int range(airUnit->getType().airWeapon().maxRange());

	int highestInRangePriority(0);
	int highestNotInRangePriority(0);
	int lowestInRangeHitPoints(10000);
	int lowestNotInRangeDistance(10000);

	BWAPI::Unit * inRangeTarget = NULL;
	BWAPI::Unit * notInRangeTarget = NULL;

	BOOST_FOREACH(BWAPI::Unit * unit, targets)
	{
		int priority = getAttackPriority(airUnit, unit);
		int distance = airUnit->getDistance(unit);

		// if the unit is in range, update the target with the lowest hp
		if (airUnit->getDistance(unit) <= range)
		{
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
			if (priority > highestNotInRangePriority ||
				(priority == highestNotInRangePriority && distance < lowestNotInRangeDistance))
			{
				lowestNotInRangeDistance = distance;
				highestNotInRangePriority = priority;
				notInRangeTarget = unit;
			}
		}
	}

	// if there is a highest priority unit in range, attack it first
	return (highestInRangePriority >= highestNotInRangePriority) ? inRangeTarget : notInRangeTarget;
}

// get the attack priority of a type in relation to a zergling
int AirManager::getAttackPriority(BWAPI::Unit * airUnit, BWAPI::Unit * target)
{
	BWAPI::UnitType airUnitType = airUnit->getType();
	BWAPI::UnitType targetType = target->getType();

	bool canAttackUs = targetType.airWeapon() != BWAPI::WeaponTypes::None;

	// highest priority is something that can attack us or aid in combat
	if (targetType == BWAPI::UnitTypes::Terran_Medic || canAttackUs ||
		targetType == BWAPI::UnitTypes::Terran_Bunker)
	{
		return 3;
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

bool AirManager::isValidTarget(BWAPI::Unit * target) const
{
	// Only allow air units to attack air units, or harrass probes
	BWAPI::UnitType targetType = target->getType();

	return targetType.isFlyer();
}
