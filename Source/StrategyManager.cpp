#include "base/ProductionManager.h"
#include "Common.h"
#include "StrategyManager.h"

// constructor
StrategyManager::StrategyManager() 
	: firstAttackSent(false)
	, allowRetreat(true)
	, currentArmySizeAdvantage(-100)
	, currentStrategy(0)
	, selfRace(BWAPI::Broodwar->self()->getRace())
	, enemyRace(BWAPI::Broodwar->enemy()->getRace())
{
	addStrategies();
	setStrategy();
}

// get an instance of this
StrategyManager & StrategyManager::Instance() 
{
	static StrategyManager instance;
	return instance;
}

void StrategyManager::addStrategies() 
{
	protossOpeningBook = std::vector<std::string>(NumProtossStrategies);
	terranOpeningBook  = std::vector<std::string>(NumTerranStrategies);
	zergOpeningBook    = std::vector<std::string>(NumZergStrategies);

	//protossOpeningBook[ProtossZealotRush]	= "0 0 0 0 1 0 0 3 0 0 3 0 1 3 0 4 4 4 4 4 1 0 4 4 4";
    protossOpeningBook[ProtossZealotRush]	= "0 0 0 0 1 0 3 3 0 0 4 1 4 4 0 4 4 0 1 4 3 0 1 0 4 0 4 4 4 4 1 0 4 4 4";
	protossOpeningBook[ProtossExtendedZealotRush] = protossOpeningBook[ProtossZealotRush];
	protossOpeningBook[ProtossCannonTurtle] = "0 0 0 0 1 0 3 3 0 0 4 1 4 4 0 4 4 0 1 4 3 0 1 0 4 0 4 4 4 1 4 9 0 10 10 10";
	protossOpeningBook[ProtossAggressiveTurtle] = "0 0 0 0 1 0 3 3 0 0 4 1 4 4 0 4 4 0 1 4 3 0 1 0 4 0 4 4 4 1 4 9 0 10 10 10";
	//protossOpeningBook[ProtossDarkTemplar]	= "0 0 0 0 1 3 0 7 5 0 0 12 3 13 0 22 22 22 22 0 1 0";
    protossOpeningBook[ProtossDarkTemplar]	=     "0 0 0 0 1 0 3 0 7 0 5 0 12 0 13 3 22 22 1 22 22 0 1 0";
	protossOpeningBook[ProtossDragoons]		= "0 0 0 0 1 0 0 3 0 7 0 0 5 0 0 3 8 6 1 6 6 0 3 1 0 6 6 6";
	terranOpeningBook[TerranMarineRush]		= "0 0 0 0 0 1 0 0 3 0 0 3 0 1 0 4 0 0 0 6";
	zergOpeningBook[ZergZerglingRush]		= "0 0 0 0 0 1 0 0 0 2 3 5 0 0 0 0 0 0 1 6";

	if (selfRace == BWAPI::Races::Protoss)
	{
		results = std::vector<IntPair>(NumProtossStrategies);

		if (enemyRace == BWAPI::Races::Protoss)
		{
			usableStrategies.push_back(ProtossZealotRush);
			usableStrategies.push_back(ProtossDarkTemplar);
			usableStrategies.push_back(ProtossDragoons);
			usableStrategies.push_back(ProtossCannonTurtle);
			usableStrategies.push_back(ProtossAggressiveTurtle);
			usableStrategies.push_back(ProtossExtendedZealotRush);
		}
		else if (enemyRace == BWAPI::Races::Terran)
		{
			usableStrategies.push_back(ProtossZealotRush);
			usableStrategies.push_back(ProtossDarkTemplar);
			usableStrategies.push_back(ProtossDragoons);
			usableStrategies.push_back(ProtossCannonTurtle);
			usableStrategies.push_back(ProtossAggressiveTurtle);
			usableStrategies.push_back(ProtossExtendedZealotRush);
		}
		else if (enemyRace == BWAPI::Races::Zerg)
		{
			usableStrategies.push_back(ProtossZealotRush);
			usableStrategies.push_back(ProtossDragoons);
			usableStrategies.push_back(ProtossCannonTurtle);
			usableStrategies.push_back(ProtossAggressiveTurtle);
			usableStrategies.push_back(ProtossExtendedZealotRush);
		}
		else
		{
			BWAPI::Broodwar->printf("Enemy Race Unknown");
			usableStrategies.push_back(ProtossZealotRush);
			usableStrategies.push_back(ProtossDragoons);
			usableStrategies.push_back(ProtossCannonTurtle);
			usableStrategies.push_back(ProtossAggressiveTurtle);
			usableStrategies.push_back(ProtossExtendedZealotRush);
		}
	}
	else if (selfRace == BWAPI::Races::Terran)
	{
		results = std::vector<IntPair>(NumTerranStrategies);
		usableStrategies.push_back(TerranMarineRush);
	}
	else if (selfRace == BWAPI::Races::Zerg)
	{
		results = std::vector<IntPair>(NumZergStrategies);
		usableStrategies.push_back(ZergZerglingRush);
	}

	if (Options::Modules::USING_STRATEGY_IO)
	{
		readResults();
	}
}

void StrategyManager::readResults()
{
	// read in the name of the read and write directories from settings file
	struct stat buf;

	// if the file doesn't exist something is wrong so just set them to default settings
	if (stat(Options::FileIO::FILE_SETTINGS, &buf) == -1)
	{
		readDir = "bwapi-data/testio/read/";
		writeDir = "bwapi-data/testio/write/";
	}
	else
	{
		std::ifstream f_in(Options::FileIO::FILE_SETTINGS);
		getline(f_in, readDir);
		getline(f_in, writeDir);
		f_in.close();
	}

	// the file corresponding to the enemy's previous results
	std::string readFile = readDir + BWAPI::Broodwar->enemy()->getName() + ".txt";

	// if the file doesn't exist, set the results to zeros
	if (stat(readFile.c_str(), &buf) == -1)
	{
		std::fill(results.begin(), results.end(), IntPair(0,0));
	}
	// otherwise read in the results
	else
	{
		std::ifstream f_in(readFile.c_str());
		std::string line;
		getline(f_in, line);
		results[ProtossZealotRush].first = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossZealotRush].second = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossDarkTemplar].first = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossDarkTemplar].second = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossDragoons].first = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossDragoons].second = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossCannonTurtle].first = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossCannonTurtle].second = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossAggressiveTurtle].first = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossAggressiveTurtle].second = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossExtendedZealotRush].first = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossExtendedZealotRush].second = atoi(line.c_str());
		f_in.close();
	}

	BWAPI::Broodwar->printf("Results (%s): (%d %d) (%d %d) (%d %d)", BWAPI::Broodwar->enemy()->getName().c_str(), 
		results[0].first, results[0].second, results[1].first, results[1].second, results[2].first, results[2].second);
}

void StrategyManager::writeResults()
{
	std::string writeFile = writeDir + BWAPI::Broodwar->enemy()->getName() + ".txt";
	std::ofstream f_out(writeFile.c_str());

	f_out << results[ProtossZealotRush].first   << "\n";
	f_out << results[ProtossZealotRush].second  << "\n";
	f_out << results[ProtossDarkTemplar].first  << "\n";
	f_out << results[ProtossDarkTemplar].second << "\n";
	f_out << results[ProtossDragoons].first     << "\n";
	f_out << results[ProtossDragoons].second    << "\n";
	f_out << results[ProtossCannonTurtle].first << "\n";
	f_out << results[ProtossCannonTurtle].second << "\n";
	f_out << results[ProtossAggressiveTurtle].first << "\n";
	f_out << results[ProtossAggressiveTurtle].second << "\n";
	f_out << results[ProtossExtendedZealotRush].first << "\n";
	f_out << results[ProtossExtendedZealotRush].second << "\n";
	

	f_out.close();
}

void StrategyManager::setStrategy()
{
	// if we are using file io to determine strategy, do so
	if (Options::Modules::USING_STRATEGY_IO)
	{
		double bestUCB = -1;
		int bestStrategyIndex = 0;

		// UCB requires us to try everything once before using the formula
		for (size_t strategyIndex(0); strategyIndex<usableStrategies.size(); ++strategyIndex)
		{
			int sum = results[usableStrategies[strategyIndex]].first + results[usableStrategies[strategyIndex]].second;

			if (sum == 0)
			{
				currentStrategy = usableStrategies[strategyIndex];
				return;
			}
		}

		// if we have tried everything once, set the maximizing ucb value
		for (size_t strategyIndex(0); strategyIndex<usableStrategies.size(); ++strategyIndex)
		{
			double ucb = getUCBValue(usableStrategies[strategyIndex]);

			if (ucb > bestUCB)
			{
				bestUCB = ucb;
				bestStrategyIndex = strategyIndex;
			}
		}
		
		currentStrategy = usableStrategies[bestStrategyIndex];
	}
	else
	{
		// otherwise return a random strategy

        std::string enemyName(BWAPI::Broodwar->enemy()->getName());
        
        if (enemyName.compare("Skynet") == 0)
        {
            currentStrategy = ProtossDarkTemplar;
        }
        else
        {
            //currentStrategy = ProtossZealotRush;
			//currentStrategy = ProtossCannonTurtle;
			currentStrategy = ProtossAggressiveTurtle;
        }
	}

}

void StrategyManager::onEnd(const bool isWinner)
{
	// write the win/loss data to file if we're using IO
	if (Options::Modules::USING_STRATEGY_IO)
	{
		// if the game ended before the tournament time limit
		if (BWAPI::Broodwar->getFrameCount() < Options::Tournament::GAME_END_FRAME)
		{
			if (isWinner)
			{
				results[getCurrentStrategy()].first = results[getCurrentStrategy()].first + 1;
			}
			else
			{
				results[getCurrentStrategy()].second = results[getCurrentStrategy()].second + 1;
			}
		}
		// otherwise game timed out so use in-game score
		else
		{
			if (getScore(BWAPI::Broodwar->self()) > getScore(BWAPI::Broodwar->enemy()))
			{
				results[getCurrentStrategy()].first = results[getCurrentStrategy()].first + 1;
			}
			else
			{
				results[getCurrentStrategy()].second = results[getCurrentStrategy()].second + 1;
			}
		}
		
		writeResults();
	}
}

const double StrategyManager::getUCBValue(const size_t & strategy) const
{
	double totalTrials(0);
	for (size_t s(0); s<usableStrategies.size(); ++s)
	{
		totalTrials += results[usableStrategies[s]].first + results[usableStrategies[s]].second;
	}

	double C		= 0.7;
	double wins		= results[strategy].first;
	double trials	= results[strategy].first + results[strategy].second;

	double ucb = (wins / trials) + C * sqrt(std::log(totalTrials) / trials);

	return ucb;
}

const int StrategyManager::getScore(BWAPI::Player * player) const
{
	return player->getBuildingScore() + player->getKillScore() + player->getRazingScore() + player->getUnitScore();
}

const std::string StrategyManager::getOpeningBook() const
{
	if (selfRace == BWAPI::Races::Protoss)
	{
		return protossOpeningBook[currentStrategy];
	}
	else if (selfRace == BWAPI::Races::Terran)
	{
		return terranOpeningBook[currentStrategy];
	}
	else if (selfRace == BWAPI::Races::Zerg)
	{
		return zergOpeningBook[currentStrategy];
	} 

	// something wrong, return the protoss one
	return protossOpeningBook[currentStrategy];
}

// when do we want to defend with our workers?
// this function can only be called if we have no fighters to defend with
const int StrategyManager::defendWithWorkers()
{
	if (!Options::Micro::WORKER_DEFENSE)
	{
		return false;
	}

	// our home nexus position
	BWAPI::Position homePosition = BWTA::getStartLocation(BWAPI::Broodwar->self())->getPosition();;

	// enemy units near our workers
	int enemyUnitsNearWorkers = 0;

	// defense radius of nexus
	int defenseRadius = 300;

	// fill the set with the types of units we're concerned about
	BOOST_FOREACH (BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits())
	{
		// if it's a zergling or a worker we want to defend
		if (unit->getType() == BWAPI::UnitTypes::Zerg_Zergling)
		{
			if (unit->getDistance(homePosition) < defenseRadius)
			{
				enemyUnitsNearWorkers++;
			}
		}
	}

	// if there are enemy units near our workers, we want to defend
	return enemyUnitsNearWorkers;
}

// called by combat commander to determine whether or not to send an attack force
// freeUnits are the units available to do this attack
const bool StrategyManager::doAttack(const std::set<BWAPI::Unit *> & freeUnits)
{
	int ourForceSize = (int)freeUnits.size();
	int enemyForceSize = InformationManager::Instance().numEnemyCombatUnits(BWAPI::Broodwar->enemy());
	int numUnitsNeededForAttack;
	bool doAttack = false;
	int frame = BWAPI::Broodwar->getFrameCount();

	// Calculate the difference between their army size and ours
	currentArmySizeAdvantage = ourForceSize - enemyForceSize;

	// Don't rush with ProtossCannonTurtle strategy
	if (currentStrategy == ProtossCannonTurtle) {
		int numCannon = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);

		// If we are beyond early game and our cannons have been destroyed, attack
		if (frame > 10000 && numCannon == 0) {
			doAttack = true;
		}
		// If our force size is significantly greater than theirs, attack
		else if (enemyForceSize != -1) {
			doAttack = ((ourForceSize - enemyForceSize) >= 20);
		}
		// Could not get enemy force size
		else {
			doAttack = false;
		}
	}
	// Don't rush with ProtossCannonTurtle strategy
	else if (currentStrategy == ProtossAggressiveTurtle)
	{
		if (enemyForceSize != -1 || (frame > 15000))
		{
			// Update the retreat flag based on army sizes
			if (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Leg_Enhancements) > 0)
			{
				// Retreat less frequently if we have leg enhancements
				allowRetreat = (currentArmySizeAdvantage < -5);
			}
			else
			{
				allowRetreat = (currentArmySizeAdvantage < 8);
			}

			// Determine if we should attack
			if (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Leg_Enhancements) > 0)
			{
				// If we have leg enhancements, allow our army to attack a more powerfull army
				// Or if in late game, attack so we don't mine out with our 1 base
				doAttack = (currentArmySizeAdvantage >= 6) || ((frame > 20000) && (currentArmySizeAdvantage > -10));
			}
			else
			{
				doAttack = (currentArmySizeAdvantage >= 10);
			}
		}
		// Could not get enemy force size
		else 
		{
			doAttack = false;
		}

		InformationManager::Instance().setIsAttacking(doAttack);
	}
	// Rush with other strategies
	else {
		numUnitsNeededForAttack = 1;
		doAttack  = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Dark_Templar) >= 1
						|| ourForceSize >= numUnitsNeededForAttack;
	}
	

	if (doAttack)
	{
		firstAttackSent = true;
	}

	return doAttack || firstAttackSent;
}

const bool StrategyManager::expandProtossZealotRush() const
{
	// if there is no place to expand to, we can't expand
	if (MapTools::Instance().getNextExpansion() == BWAPI::TilePositions::None)
	{
		return false;
	}

	int numNexus =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numZealots =			BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	int frame =					BWAPI::Broodwar->getFrameCount();

	// if there are more than 10 idle workers, expand
	if (WorkerManager::Instance().getNumIdleWorkers() > 10)
	{
		return true;
	}

	// 2nd Nexus Conditions:
	//		We have 12 or more zealots
	//		It is past frame 7000
	if ((numNexus < 2) && (numZealots > 12 || frame > 9000))
	{
		return true;
	}

	// 3nd Nexus Conditions:
	//		We have 24 or more zealots
	//		It is past frame 12000
	if ((numNexus < 3) && (numZealots > 24 || frame > 15000))
	{
		return true;
	}

	if ((numNexus < 4) && (numZealots > 24 || frame > 21000))
	{
		return true;
	}

	if ((numNexus < 5) && (numZealots > 24 || frame > 26000))
	{
		return true;
	}

	if ((numNexus < 6) && (numZealots > 24 || frame > 30000))
	{
		return true;
	}

	return false;
}

const bool StrategyManager::expandProtossCannonTurtle() const
{
	// if there is no place to expand to, we can't expand
	if (MapTools::Instance().getNextExpansion() == BWAPI::TilePositions::None)
	{
		return false;
	}

	int numNexus = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numZealots = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	int frame = BWAPI::Broodwar->getFrameCount();
	bool attack = InformationManager::Instance().getIsAttacking();

	if (attack) {
		// if there are more than 10 idle workers, expand
		if (WorkerManager::Instance().getNumIdleWorkers() > 10)
		{
			return true;
		}

		// 2nd Nexus Conditions:
		//		We have 12 or more zealots
		//		It is past frame 7000
		if ((numNexus < 2) && (numZealots > 12 || frame > 9000))
		{
			return true;
		}

		// 3nd Nexus Conditions:
		//		We have 24 or more zealots
		//		It is past frame 12000
		if ((numNexus < 3) && (numZealots > 24 || frame > 15000))
		{
			return true;
		}

		if ((numNexus < 4) && (numZealots > 24 || frame > 21000))
		{
			return true;
		}

		if ((numNexus < 5) && (numZealots > 24 || frame > 26000))
		{
			return true;
		}

		if ((numNexus < 6) && (numZealots > 24 || frame > 30000))
		{
			return true;
		}
	}
	return false;
}

const bool StrategyManager::expandProtossAggressiveTurtle() const
{
	// if there is no place to expand to, we can't expand
	if (MapTools::Instance().getNextExpansion() == BWAPI::TilePositions::None)
	{
		return false;
	}

	int numNexus = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numZealots = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	int frame = BWAPI::Broodwar->getFrameCount();
	bool attack = InformationManager::Instance().getIsAttacking();

	// if there are more than 10 idle workers, expand
	if (WorkerManager::Instance().getNumIdleWorkers() > 8)
	{
		return true;
	}

	if (attack) 
	{
		// 2nd Nexus Conditions:
		//		We have 12 or more zealots
		//		It is past frame 7000
		if ((numNexus < 2) && (numZealots > 12 || frame > 9000))
		{
			return true;
		}

		// 3nd Nexus Conditions:
		//		We have 24 or more zealots
		//		It is past frame 12000
		if ((numNexus < 3) && (numZealots > 24 || frame > 15000))
		{
			return true;
		}

		if ((numNexus < 4) && (numZealots > 24 || frame > 21000))
		{
			return true;
		}

		if ((numNexus < 5) && (numZealots > 24 || frame > 26000))
		{
			return true;
		}

		if ((numNexus < 6) && (numZealots > 24 || frame > 30000))
		{
			return true;
		}
	}
	// If we aren't under pressure in late game
	else if ((frame > 18000) && (numZealots > 25))
	{
		return true;
	}
	return false;
}

const MetaPairVector StrategyManager::getBuildOrderGoal()
{
	if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Protoss)
	{
		if (getCurrentStrategy() == ProtossZealotRush)
		{
			return getProtossZealotRushBuildOrderGoal();
		}
		else if (getCurrentStrategy() == ProtossDarkTemplar)
		{
			return getProtossDarkTemplarBuildOrderGoal();
		}
		else if (getCurrentStrategy() == ProtossDragoons)
		{
			return getProtossDragoonsBuildOrderGoal();
		}
		else if (getCurrentStrategy() == ProtossCannonTurtle) {
			return getProtossCannonTurtleBuildOrderGoal();
		}
		else if (getCurrentStrategy() == ProtossAggressiveTurtle)
		{
			return getProtossAggressiveTurtleBuildOrderGoal();
		}
		else if (getCurrentStrategy() == ProtossExtendedZealotRush)
		{
			return getProtossExtendedZealotRushBuildOrderGoal();
		}

		// if something goes wrong, use zealot goal
		return getProtossZealotRushBuildOrderGoal();
	}
	else if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Terran)
	{
		return getTerranBuildOrderGoal();
	}
	else
	{
		return getZergBuildOrderGoal();
	}
}

const MetaPairVector StrategyManager::getProtossDragoonsBuildOrderGoal() const
{
		// the goal to return
	MetaPairVector goal;

	int numDragoons =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	int numProbes =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numNexusCompleted =		BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numNexusAll =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numCyber =				BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
	int numCannon =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);

	int dragoonsWanted = numDragoons > 0 ? numDragoons + 6 : 2;
	int gatewayWanted = 3;
	int probesWanted = numProbes + 6;

	if (InformationManager::Instance().enemyHasCloakedUnits())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
	
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}
	else
	{
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}

		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}

	if (numNexusAll >= 2 || numDragoons > 6 || BWAPI::Broodwar->getFrameCount() > 9000)
	{
		gatewayWanted = 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
	}

	if (numNexusCompleted >= 3)
	{
		gatewayWanted = 8;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 2));
	}

	if (numNexusAll > 1)
	{
		probesWanted = numProbes + 6;
	}

	if (expandProtossZealotRush())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Nexus, numNexusAll + 1));
	}

	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon,	dragoonsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Gateway,	gatewayWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Probe,	std::min(90, probesWanted)));

	return goal;
}

const MetaPairVector StrategyManager::getProtossDarkTemplarBuildOrderGoal() const
{
	// the goal to return
	MetaPairVector goal;

	int numDarkTeplar =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dark_Templar);
	int numDragoons =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	int numProbes =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numNexusCompleted =		BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numNexusAll =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numCyber =				BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
	int numCannon =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);

	int darkTemplarWanted = 0;
	int dragoonsWanted = numDragoons + 6;
	int gatewayWanted = 3;
	int probesWanted = numProbes + 6;

	if (InformationManager::Instance().enemyHasCloakedUnits())
	{
		
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
		
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}

	if (numNexusAll >= 2 || BWAPI::Broodwar->getFrameCount() > 9000)
	{
		gatewayWanted = 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
	}

	if (numDragoons > 0)
	{
		goal.push_back(MetaPair(BWAPI::UpgradeTypes::Singularity_Charge, 1));
	}

	if (numNexusCompleted >= 3)
	{
		gatewayWanted = 8;
		dragoonsWanted = numDragoons + 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
	}

	if (numNexusAll > 1)
	{
		probesWanted = numProbes + 6;
	}

	if (expandProtossZealotRush())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Nexus, numNexusAll + 1));
	}

	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon,	dragoonsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Gateway,	gatewayWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dark_Templar, darkTemplarWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Probe,	std::min(90, probesWanted)));
	
	return goal;
}

const MetaPairVector StrategyManager::getProtossZealotRushBuildOrderGoal() const
{
	// the goal to return
	MetaPairVector goal;

	int numZealots =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	int numDragoons =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	int numProbes =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numNexusCompleted =		BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numNexusAll =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numCyber =				BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
	int numCannon =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);

	int zealotsWanted = numZealots + 8;
	int dragoonsWanted = numDragoons;
	int gatewayWanted = 3;
	int probesWanted = numProbes + 4;

	if (InformationManager::Instance().enemyHasCloakedUnits())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
		
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}

	if (numNexusAll >= 2 || BWAPI::Broodwar->getFrameCount() > 9000)
	{
		gatewayWanted = 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Assimilator, 1));
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Cybernetics_Core, 1));
	}

	if (numCyber > 0)
	{
		dragoonsWanted = numDragoons + 2;
		goal.push_back(MetaPair(BWAPI::UpgradeTypes::Singularity_Charge, 1));
	}

	if (numNexusCompleted >= 3)
	{
		gatewayWanted = 8;
		dragoonsWanted = numDragoons + 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
	}

	if (numNexusAll > 1)
	{
		probesWanted = numProbes + 6;
	}

	if (expandProtossZealotRush())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Nexus, numNexusAll + 1));
	}

	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon,	dragoonsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Zealot,	zealotsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Gateway,	gatewayWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Probe,	std::min(90, probesWanted)));

	return goal;
}

const MetaPairVector StrategyManager::getProtossExtendedZealotRushBuildOrderGoal() const
{
	// the goal to return
	MetaPairVector goal;

	int numZealots = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	int numDragoons = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	int numProbes = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numNexusCompleted = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numNexusAll = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numCyber = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
	int numCannon = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);

	int zealotsWanted = numZealots + 8;
	int dragoonsWanted = numDragoons;
	int gatewayWanted = 3;
	int probesWanted = numProbes + 4;

	if (InformationManager::Instance().enemyHasCloakedUnits())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));

		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}

	if (numNexusAll >= 2 || BWAPI::Broodwar->getFrameCount() > 9000)
	{
		gatewayWanted = 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Assimilator, 1));
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Cybernetics_Core, 1));

		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Assimilator) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Citadel_of_Adun, 1));
			goal.push_back(MetaPair(BWAPI::UpgradeTypes::Leg_Enhancements, 1));
		}
	}

	if (numCyber > 0)
	{
		dragoonsWanted = numDragoons + 2;
		goal.push_back(MetaPair(BWAPI::UpgradeTypes::Singularity_Charge, 1));
	}

	if (numNexusCompleted >= 3)
	{
		gatewayWanted = 8;
		dragoonsWanted = numDragoons + 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
	}

	if (numNexusAll > 1)
	{
		probesWanted = numProbes + 6;
	}

	if (expandProtossZealotRush())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Nexus, numNexusAll + 1));
	}

	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon, dragoonsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Zealot, zealotsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Gateway, gatewayWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Probe, std::min(90, probesWanted)));

	return goal;
}

const MetaPairVector StrategyManager::getProtossCannonTurtleBuildOrderGoal() const
{
	// the goal to return
	MetaPairVector goal;

	int numZealots = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	int numDragoons = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	int numProbes = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numNexusCompleted = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numNexusAll = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numCyber = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
	int numCannon = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);
	int numAssimilator = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Assimilator);

	int zealotsWanted = numZealots + 8;
	int dragoonsWanted = numDragoons;
	int gatewayWanted = 3;
	int probesWanted = numProbes + 4;
	int cannonsWanted = numCannon + 3;

	// Cloaked unit check
	if (InformationManager::Instance().enemyHasCloakedUnits())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));

		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}

	// Build assimilator when forge is built
	if (BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Forge)) {
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Assimilator, 1));
	}

	// Start pumping out zealots in prep for attack 
	if (BWAPI::Broodwar->getFrameCount() > 10000)
	{
		gatewayWanted = 5;
		zealotsWanted = numZealots + 10;
		//goal.push_back(MetaPair(BWAPI::UpgradeTypes::Protoss_Ground_Weapons, 1));
	}

	// Build Cyber Core for Dragoon support late game
	if ((BWAPI::Broodwar->getFrameCount() > 15000) && (numAssimilator >= 1)) {
		gatewayWanted = 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Cybernetics_Core, 1));
	}

	// Slow down on cannon production late game
	if (numCannon >= 12) {
		cannonsWanted = numCannon + 1;
	}

	// Dragoon production
	if (numCyber > 0)
	{
		dragoonsWanted = numDragoons + 4;
		zealotsWanted = numZealots + 8;
	}

	// Get some observers if we've expanded
	if (numNexusCompleted >= 3)
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
	}

	// Expansion condition
	if (expandProtossCannonTurtle())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Nexus, numNexusAll + 1));
	}

	// Build order goal requirements
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Photon_Cannon, cannonsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon, dragoonsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Zealot, zealotsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Gateway, gatewayWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Probe, std::min(90, probesWanted)));

	return goal;
}

const MetaPairVector StrategyManager::getProtossAggressiveTurtleBuildOrderGoal() const
{
	// the goal to return
	MetaPairVector goal;

	int numGateways = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Gateway);
	int numZealots = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	int numDragoons = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	int numProbes = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numNexusCompleted = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numNexusAll = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numCyber = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
	int numCannon = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);
	int numStargate = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Stargate);
	int numScouts = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Scout);
	int numCorsairs = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Corsair);

	int zealotsWanted = numZealots + 8;
	int dragoonsWanted = numDragoons;
	int gatewayWanted = 3;
	int probesWanted = numProbes + 4;
	int cannonsWanted = numCannon;
	int scoutsWanted = numScouts;
	int corsairsWanted = numCorsairs;

	// Check if the bot is currently attacking
	bool attack = InformationManager::Instance().getIsAttacking();

	// If the enemy has flying units that can attack, prepare an air defense
	bool needAirCounter = InformationManager::Instance().enemyFlyerThreat();

	int freeMinerals = ProductionManager::Instance().getFreeMinerals();
	int freeGas = ProductionManager::Instance().getFreeGas();

	// Set the max probes wanted given the number of nexuses
	int maxProbes = getMaxProbeCount();

	int frame = BWAPI::Broodwar->getFrameCount();

	// Only need detection if the enemy has cloaked units that can attack ground
	if (InformationManager::Instance().enemyHasCloakedGroundCombatUnits())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));

		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}

	// Expansion condition
	if (expandProtossCannonTurtle())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Nexus, numNexusAll + 1));
	}

	if (needAirCounter && (numCyber > 0))
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Stargate, 2));
	}

	// Build assimilator when forge is built
	if (BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Forge)) {
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Assimilator, 1));
	}

	if (frame > 12000)
	{
		gatewayWanted = 6;
		zealotsWanted = numZealots + 8;
	}

	if ((numCyber < 1) && (numCannon > 3))
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Cybernetics_Core, 1));

	// Set a max number of cannons to produces
	if (numCannon < 9)
	{
		cannonsWanted = std::min(numCannon + 3, 8);
	}
	else if (needAirCounter)
	{
		cannonsWanted = std::min(numCannon + 3, 11);
	}

	if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Citadel_of_Adun) > 0)
	{
		goal.push_back(MetaPair(BWAPI::UpgradeTypes::Leg_Enhancements, 1));
	}

	if ((numCyber > 0))
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Citadel_of_Adun, 1));
	}

	if ((numStargate > 0) && needAirCounter)
	{
		dragoonsWanted += 4;
		corsairsWanted = std::min(corsairsWanted + 2, 8);
	}

	// Get some observers if we've expanded
	if (numNexusCompleted >= 3)
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
	}
	else if ((numNexusCompleted >= 2) && !needAirCounter)
	{
		dragoonsWanted += 4;
	}

	// Build order goal requirements
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Photon_Cannon, cannonsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon, dragoonsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Zealot, zealotsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Gateway, gatewayWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Scout, scoutsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Corsair, corsairsWanted));

	// Only build probes if we do not have too many idle probes
	if (WorkerManager::Instance().getNumIdleWorkers() < 8)
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Probe, std::min(maxProbes, probesWanted)));

	return goal;
}

const MetaPairVector StrategyManager::getTerranBuildOrderGoal() const
{
	// the goal to return
	std::vector< std::pair<MetaType, UnitCountType> > goal;

	int numMarines =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Marine);
	int numMedics =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Medic);
	int numWraith =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Wraith);

	int marinesWanted = numMarines + 12;
	int medicsWanted = numMedics + 2;
	int wraithsWanted = numWraith + 4;

	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Marine,	marinesWanted));

	return (const std::vector< std::pair<MetaType, UnitCountType> >)goal;
}

const MetaPairVector StrategyManager::getZergBuildOrderGoal() const
{
	// the goal to return
	std::vector< std::pair<MetaType, UnitCountType> > goal;
	
	int numMutas  =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Zerg_Mutalisk);
	int numHydras  =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Zerg_Hydralisk);

	int mutasWanted = numMutas + 6;
	int hydrasWanted = numHydras + 6;

	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Zergling, 4));
	//goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Stim_Packs,	1));

	//goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Medic,		medicsWanted));

	return (const std::vector< std::pair<MetaType, UnitCountType> >)goal;
}

 const int StrategyManager::getCurrentStrategy()
 {
	 return currentStrategy;
 }

 // Builds and returns a vector of units/buildings/upgrades according to the current strategy. Used in place
 // of the build order search if the build order search is disabled.
 std::vector<MetaType> StrategyManager::getCustomBuildOrder()
 {
	 // Vector of units/buildings/ugrades to build
	 std::vector<MetaType> customBuildOrder;

	 switch (currentStrategy)
	 {
	 case ProtossAggressiveTurtle:
		 customBuildOrder = getProtossAggressiveTurtleCustomBuildOrder();
		 break;
	 case ProtossExtendedZealotRush:
		 customBuildOrder = getProtossExtendedZealotRushCustomBuildOrder();
		 break;
	 default:
		 break;
	 }

	 return customBuildOrder;
 }

 // Builds and returns a vector of units/buildings/upgrades to be built once
 // build order search is disabled for the aggressive turtle build
 std::vector<MetaType> StrategyManager::getProtossAggressiveTurtleCustomBuildOrder()
 {
	 // If the enemy has flying units that can attack, prepare an air defense
	 bool needAirCounter = InformationManager::Instance().enemyFlyerThreat();

	// Vector of units/buildings/ugrades to build
	std::vector<MetaType> customBuildOrder;

	// First build a python if we are low in supply
	int totalSupply = BWAPI::Broodwar->self()->supplyTotal();
	int supplyAvailable = std::max(0, totalSupply - BWAPI::Broodwar->self()->supplyUsed());
	if ((supplyAvailable < 10) && (totalSupply < 200))
	{
		customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Pylon);
		customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Pylon);
	}

	// Only need detection if the enemy has cloaked units that can attack ground
	if (InformationManager::Instance().enemyHasCloakedGroundCombatUnits())
	{
		customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Robotics_Facility);
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Observatory);
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Observer);
		}
	}

	if (needAirCounter)
	{
		if (supplyAvailable > 8)
		{
			customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Dragoon);
			customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Dragoon);
		}
	}
	// Fill remaining supply with zealots
	else if (supplyAvailable > 4)
	{
		customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Zealot);
		customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Zealot);
	}

	return customBuildOrder;
 }

 // Get the max probes allowed given the number of nexuses
 int StrategyManager::getMaxProbeCount() const
 {
	 return std::min(BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus) * 30, 90);
 }

 // Get the max probes allowed given the number of nexuses
 bool StrategyManager::isRetreatEnabled() const
 {
	 return allowRetreat;
 }

 // Get the size advantage of our army over the opponents army
 int StrategyManager::getCurrentArmySizeAdvantage() const
 {
	 return currentArmySizeAdvantage;
 }

 std::vector<MetaType> StrategyManager::getProtossExtendedZealotRushCustomBuildOrder()
 {
	 std::vector<MetaType> customBuildOrder;

	 int supplyAvailable = BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed();
	 // Add as many zealots as possible (up to 4)
	 for (int i = 0; (i < 4) && (supplyAvailable > BWAPI::UnitTypes::Protoss_Zealot.supplyRequired()); ++i)
	 {
		 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Zealot);
		 supplyAvailable -= BWAPI::UnitTypes::Protoss_Zealot.supplyRequired();
	 }

	 // Add 2 probes if possible
	 if (supplyAvailable >= 2 * BWAPI::UnitTypes::Protoss_Probe.supplyRequired())
	 {
		 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Probe);
		 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Probe);
	 }

	 // if we have a completed cybernetics core
	 if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core) > 0)
	 {
		 // make sure singularity charge is upgraded or upgrading
		 if (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Singularity_Charge) > 0
			 || BWAPI::Broodwar->self()->isUpgrading(BWAPI::UpgradeTypes::Singularity_Charge))
		 {
			 // add two dragoons if possible
			 if (supplyAvailable >= 2 * BWAPI::UnitTypes::Protoss_Dragoon.supplyRequired())
			 {
				 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Dragoon);
				 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Dragoon);
			 }
		 }
		 // we need singularity charge
		 else
		 {
			 customBuildOrder.push_back(BWAPI::UpgradeTypes::Singularity_Charge);
		 }
	 }

	 // if we have a forge
	 if (BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Forge) > 0)
	 {
		 // if we are not upgrading weapons already
		 if (!BWAPI::Broodwar->self()->isUpgrading(BWAPI::UpgradeTypes::Protoss_Ground_Weapons))
		 {
			 // if we have not maxed out the weapons upgrade
			 if (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Ground_Weapons)
				 < BWAPI::Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Ground_Weapons))
			 {
				 // if we have the level 1 weapons upgrade
				 if (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Ground_Weapons) <= 1
					 && (BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Templar_Archives) == 0))
				 {
					 // add the templar archives
					 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Templar_Archives);
				 }
				 // if we have level 0 or level 1  and a templar archive
				 if ((BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Templar_Archives) > 0)
					 || (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Ground_Weapons) == 0))
				 {
					 // get the next weapons upgrade level
					 customBuildOrder.push_back(BWAPI::UpgradeTypes::Protoss_Ground_Weapons);
				 }
			 }
		 }
		 // if we only have 1 forge
		 if (BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Forge) == 1)
		 {
			 // build another forge
			 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Forge);
		 }
		 // if armour is not upgrading and we have more than 1 forge
		 if (!BWAPI::Broodwar->self()->isUpgrading(BWAPI::UpgradeTypes::Protoss_Ground_Armor)
			 && BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Forge) > 1)
		 {
			 // if the armour upgrade is not max level
			 if (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Ground_Armor)
				 < BWAPI::Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Ground_Armor))
			 {
				 // if we have level 0 or level 1  and a templar archive
				 if ((BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Templar_Archives) > 0)
					 || (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Ground_Armor) == 0))
				 {
					 // get the armour upgrade
					 customBuildOrder.push_back(BWAPI::UpgradeTypes::Protoss_Ground_Armor);
				 }
			 }
		 }
		 // if we ended up with more than 2 forges
		 if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Forge) > 2)
		 {
			 // if plasma shields are not max level and not upgrading
			 if (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Plasma_Shields) != BWAPI::Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Plasma_Shields)
				 && !BWAPI::Broodwar->self()->isUpgrading(BWAPI::UpgradeTypes::Protoss_Plasma_Shields))
			 {
				 // upgrade plasma shields
				 customBuildOrder.push_back(BWAPI::UpgradeTypes::Protoss_Plasma_Shields);
			 }
		 }
		 // if we have only 2 forges and have maxed out either weapons or armour
		 else if (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Ground_Armor) == BWAPI::Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Ground_Armor)
			 || BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Ground_Weapons) == BWAPI::Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Ground_Weapons))
		 {
			 // if plasma shields are not max level and not upgrading
			 if (BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Plasma_Shields) != BWAPI::Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Plasma_Shields)
				 && !BWAPI::Broodwar->self()->isUpgrading(BWAPI::UpgradeTypes::Protoss_Plasma_Shields))
			 {
				 // upgrade plasma shields
				 customBuildOrder.push_back(BWAPI::UpgradeTypes::Protoss_Plasma_Shields);
			 }
		 }
	 }
	 // we have no forges
	 else
	 {
		 // add a forge and get it to research weapons
		 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Forge);
		 customBuildOrder.push_back(BWAPI::UpgradeTypes::Protoss_Ground_Weapons);
	 }

	 // if we need more supply and have not reached the maximum supply
	 if (supplyAvailable < 20 && BWAPI::Broodwar->self()->supplyTotal() < 200)
	 {
		 // build 2 more pylons
		 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Pylon);
		 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Pylon);
	 }

	 // Add as many zealots as possible (up to 4)
	 for (int i = 0; (i < 4) && (supplyAvailable > BWAPI::UnitTypes::Protoss_Zealot.supplyRequired()); ++i)
	 {
		 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Zealot);
		 supplyAvailable -= BWAPI::UnitTypes::Protoss_Zealot.supplyRequired();
	 }

	 if (customBuildOrder.empty())
	 {
		 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Photon_Cannon);
		 customBuildOrder.push_back(BWAPI::UnitTypes::Protoss_Photon_Cannon);
	 }

	 return customBuildOrder;
 }
