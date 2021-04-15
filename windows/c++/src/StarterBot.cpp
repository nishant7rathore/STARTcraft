#include "Tools.h"
#include "MapTools.h"
#include "StarterBot.h"
#include "../visualstudio/Profiler.hpp"

/**
* Nishant Rathore 
* Rohan Parmar
* 
* We have an extension for this assignment, but after chatting with Dave, I was instructed to submit this. 
* 
*/

std::thread& StarterBot::getThreadHandle()
{
    return std::ref(t1);
}

StarterBot::StarterBot(ResourceManager& rm)
{
    m_resourceManager = rm;
    m_strategyManager = StrategyManager();

}

bool isZerglingRush = false;

// Called when the bot starts!
void StarterBot::onStart()
{
    resourceCount = 100;
    // Set our BWAPI options here    
	BWAPI::Broodwar->setLocalSpeed(10);
    BWAPI::Broodwar->setFrameSkip(0);
    
    // Enable the flag that tells BWAPI top let users enter input while bot plays
    BWAPI::Broodwar->enableFlag(BWAPI::Flag::UserInput);

    //TODO: need to ask Dave about this flag
    //BWAPI::Broodwar->enableFlag(BWAPI::Flag::CompleteMapInformation);

    // Call MapTools OnStart
    //t1 = std::thread(&MapTools::onStart,std::ref(m_mapTools),std::ref(m_resourceManager));
    m_mapTools.onStart(m_resourceManager);
}

// Called whenever the game ends and tells you if you won or not
void StarterBot::onEnd(bool isWinner) 
{
    std::cout << "We " << (isWinner ? "won!" : "lost!") << "\n";
}

// Called on each frame of the game
void StarterBot::onFrame()
{
    PROFILE_FUNCTION();

    // Update our MapTools information
    m_mapTools.onFrame();

    // Send our idle workers to mine minerals so they don't just stand there
    sendIdleWorkersToMinerals();

    // Build more supply if we are going to run out soon
    buildAdditionalSupply();

    // Train more workers so we can gather more income
    trainAdditionalWorkers();

    buildBuildings();

    buildArmy();

    doUpgrades();

    findAdditionalBases();

    // Draw unit health bars, which brood war unfortunately does not do
    Tools::DrawUnitHealthBars();

    // Draw some relevent information to the screen to help us debug the bot
    drawDebugInformation();
}

BWAPI::TilePosition baseLocationTilePos;
BWAPI::TilePosition nexusTilePos;

void StarterBot::findAdditionalBases()
{

    int& workerID = m_strategyManager.getBuildingStrategyManager().getWorkerID();

    int& numOfNexus = m_strategyManager.getNumberOfUnits(BWAPI::UnitTypes::Protoss_Nexus);

    if (defenders.size() < 2 || zealots.size() < 2 || numOfNexus > 1 || workerID != -1) return;

    BWAPI::Unit worker = Tools::GetWorkerExcluding(-1);

    if (!worker) return;

    for (auto baseLocation: m_mapTools.m_baseLocations)
    {
        BWAPI::Unit closestUnit = BWAPI::Broodwar->getClosestUnit(BWAPI::Position(baseLocation), BWAPI::Filter::IsResourceDepot, 512);
        if (!closestUnit)
        {
            if (worker->move(BWAPI::Position(baseLocation)))
            {
                workerID = worker->getID();
                baseLocationTilePos = baseLocation;
                return;
            }
        }
     
    }
}

//TODO:: update this code
void StarterBot::doUpgrades()
{
    if (m_strategyManager.getBuildingStrategyManager().isAdditionalSupplyNeeded()) return;

    if (BWAPI::Broodwar->canUpgrade(BWAPI::UpgradeTypes::Protoss_Ground_Weapons))
    {
        Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Forge)->upgrade(BWAPI::UpgradeTypes::Protoss_Ground_Weapons);
        return;
    }
    if (BWAPI::Broodwar->canUpgrade(BWAPI::UpgradeTypes::Protoss_Ground_Armor))
    {
        Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Forge)->upgrade(BWAPI::UpgradeTypes::Protoss_Ground_Armor);
        return;
    }
    if (BWAPI::Broodwar->canUpgrade(BWAPI::UpgradeTypes::Leg_Enhancements))
    {
        Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Citadel_of_Adun)->upgrade(BWAPI::UpgradeTypes::Leg_Enhancements);
        return;
    }
    if (BWAPI::Broodwar->canUpgrade(BWAPI::UpgradeTypes::Singularity_Charge))
    {
        Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Cybernetics_Core)->upgrade(BWAPI::UpgradeTypes::Singularity_Charge);
        return;
    }
}

bool isRefBuilt = false;

// Send our idle workers to mine minerals so they don't just stand there
void StarterBot::sendIdleWorkersToMinerals()
{

    //if (!t1.joinable()) t1.detach();


    if (isZerglingRush)
    {
        if (workers.getClosestUnit(BWAPI::Filter::IsEnemy))
        {
            workers.attack(workers.getClosestUnit(BWAPI::Filter::IsEnemy));
            return;
        }
        isZerglingRush = false;
    }

    PROFILE_FUNCTION();

    // Let's send all of our starting workers to the closest mineral to them
    // First we need to loop over all of the units that we (BWAPI::Broodwar->self()) own
    const BWAPI::Unitset& myUnits = BWAPI::Broodwar->self()->getUnits();
    const BWAPI::UnitType workerType = BWAPI::Broodwar->self()->getRace().getWorker();
    const int workersOwned = Tools::CountUnitsOfType(workerType, BWAPI::Broodwar->self()->getUnits());

    for (auto& unit : myUnits)
    {

        // Get the closest mineral to this worker unit

        auto u = m_resourceManager.getMineralResource(unit->getTilePosition().x, unit->getTilePosition().y);

        //std::string s = std::to_string(unit->getTilePosition().x) + " " + std::to_string(unit->getTilePosition().y);

        //std::string st = std::to_string(u->getTilePosition().x) + " " + std::to_string(u->getTilePosition().y);

        BWAPI::Position pos(BWAPI::TilePosition(unit->getTilePosition().x, unit->getTilePosition().y));
        BWAPI::Unit closestMineral = BWAPI::Broodwar->getUnit(u.m_id);

       // int closMinID = closestMineral->getID();

        //std::string t = std::to_string(closestMineral->getTilePosition().x) + " " + std::to_string(closestMineral->getTilePosition().y);


        // Check the unit type, if it is an idle worker, then we want to send it somewhere
        if (unit->getType().isWorker() && unit->isIdle() && !unit->isMoving() && !unit->isAttacking())
        {
            //int numAssimilitorUnits = BWAPI::Broodwar->getUnitsOnTile(m_resourceManager.getRefineryResource(unit->getTilePosition().x, unit->getTilePosition().y).m_x, m_resourceManager.getRefineryResource(unit->getTilePosition().x, unit->getTilePosition().y).m_y).size();

            if (m_strategyManager.getBaseManager().getBaseofUnit(unit) > 0 && m_strategyManager.getBaseManager().getBuildingsCount(1,BWAPI::UnitTypes::Protoss_Nexus) > 0)
            {
                if (closestMineral)
                {
                    unit->rightClick(closestMineral);
                }              
                else if(m_strategyManager.getNumberOfCompletedUnits(BWAPI::UnitTypes::Protoss_Assimilator) > 1)
                {
                    BWAPI::Unit assimilator = m_strategyManager.getBaseManager().getUnitOfTypeFromBase(1, BWAPI::UnitTypes::Protoss_Assimilator);
                    if (assimilator) unit->rightClick(assimilator);
                }
            }

            else
            {
                if (workersOwned < 16)
                {
                    if (closestMineral) unit->rightClick(closestMineral);
                }
                else
                {
                    BWAPI::Unit refinery = Tools::GetUnitOfType(BWAPI::UnitTypes::Protoss_Assimilator);
                    if (refinery) unit->rightClick(refinery);
                }
            }

         
          
        }
    }
}

// Train more workers so we can gather more income
void StarterBot::trainAdditionalWorkers()
{

    if (m_strategyManager.getBuildingStrategyManager().isAdditionalSupplyNeeded()) return;

    const BWAPI::UnitType workerType = BWAPI::Broodwar->self()->getRace().getWorker();
    const int workersOwned = Tools::CountUnitsOfType(workerType, BWAPI::Broodwar->self()->getUnits());
    // get the unit pointer to my depot
    BWAPI::Unit myDepot = Tools::GetDepot();
    if (workersOwned <= m_workersWanted && m_strategyManager.getNumberOfCompletedUnits(BWAPI::UnitTypes::Protoss_Nexus) < 2)
    {
        // if we have a valid depot unit and it's currently not training something, train a worker
        // there is no reason for a bot to ever use the unit queueing system, it just wastes resources
        if (myDepot && !myDepot->isTraining()) { myDepot->train(workerType); }
    }
    else
    {
        myDepot = Tools::GetDepot(1,m_strategyManager.getBaseManager());
        if (workersOwned <= m_workersWanted && m_strategyManager.getNumberOfCompletedUnits(BWAPI::UnitTypes::Protoss_Nexus) > 1)
        {
            // if we have a valid depot unit and it's currently not training something, train a worker
            // there is no reason for a bot to ever use the unit queueing system, it just wastes resources
            if (myDepot && !myDepot->isTraining()) { myDepot->train(workerType); }
        }
    }
 
}

// Build more supply if we are going to run out soon
void StarterBot::buildAdditionalSupply()
{
    if (m_strategyManager.getBuildingStrategyManager().isBuildingBuiltNeeded()) return;

    PROFILE_FUNCTION();

    // Get the amount of supply supply we currently have unused
    const int unusedSupply = Tools::GetTotalSupply(true) - BWAPI::Broodwar->self()->supplyUsed();

    // If we have a sufficient amount of supply, we don't need to do anything
    if (unusedSupply > 8) 
    {
        return;
    }

    m_strategyManager.getBuildingStrategyManager().isAdditionalSupplyNeeded() = true;

    // Otherwise, we are going to build a supply provider
    const BWAPI::UnitType supplyProviderType = BWAPI::Broodwar->self()->getRace().getSupplyProvider();

    bool startedBuilding = Tools::BuildBuilding(supplyProviderType, m_strategyManager.getBuildingStrategyManager());
    if (startedBuilding)
    {
        BWAPI::Broodwar->printf("Started Building %s", supplyProviderType.getName().c_str());
    }

}


// Draw some relevent information to the screen to help us debug the bot
void StarterBot::drawDebugInformation()
{
    BWAPI::Broodwar->drawTextScreen(BWAPI::Position(10, 10), "RathoreParmar\n");
    Tools::DrawUnitCommands();
    Tools::DrawUnitBoundingBoxes();
}

BWAPI::TilePosition secondaryBasePylon = BWAPI::TilePositions::Invalid;

void StarterBot::buildBuildings()
{

    if (m_strategyManager.getNumberOfCompletedUnits(BWAPI::UnitTypes::Protoss_Pylon) < 1) return;

    std::map<BWAPI::UnitType, int> map = m_strategyManager.getBuildingStrategyManager().getBuildingOrderMap();
    std::map<BWAPI::UnitType, int>::iterator it;

    bool built = false;

    for (it = map.begin(); it != map.end(); it++)
    {

        int mineralsLeft = BWAPI::Broodwar->self()->minerals();

        int mineralPrice = it->first.mineralPrice();

        int gasLeft = BWAPI::Broodwar->self()->gas();

        int gasPrice = it->first.gasPrice();

        int& numUnits = m_strategyManager.getNumberOfUnits(it->first);


        if (Tools::checkIfBuildCommandAlreadyIssued(it->first))
        {
            continue;
        }

        if (it->first == BWAPI::UnitTypes::Protoss_Assimilator)
        {
            numUnits = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Assimilator, BWAPI::Broodwar->self()->getUnits());
        }

        if (BWAPI::Broodwar->self()->isUnitAvailable(it->first) && numUnits < m_strategyManager.getBuildingStrategyManager().getNumberOfBuildings(it->first) && mineralsLeft >= mineralPrice && gasLeft >= gasPrice)
        {
            if (it->first == BWAPI::UnitTypes::Protoss_Assimilator && Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Probe, BWAPI::Broodwar->self()->getUnits()) < 16)
            {
                continue;
            }

            built = Tools::BuildBuilding(it->first, m_strategyManager.getBuildingStrategyManager());
        

            if (built)
            {
                //numUnits = Tools::CountBuildingUnitsOfType(BWAPI::UnitTypes::Protoss_Gateway, BWAPI::Broodwar->self()->getUnits());
                //std::cout << "Gateway Built " << numUnits << std::endl;
                BWAPI::Broodwar->printf("Started Building %s", it->first.getName().c_str());
                break;
                //m_strategyManager.getUnitTypesMap()[BWAPI::UnitTypes::Protoss_Gateway]= numUnits;
            }
        }
    }


    if (!built && m_strategyManager.getBuildingStrategyManager().getWorkerID() != -1 && BWAPI::Broodwar->getUnit(m_strategyManager.getBuildingStrategyManager().getWorkerID())->exists())
    {
        if (Tools::checkIfBuildCommandAlreadyIssued(BWAPI::UnitTypes::Protoss_Nexus))
        {
            return;
        }

        built = BWAPI::Broodwar->getUnit(m_strategyManager.getBuildingStrategyManager().getWorkerID())->build(BWAPI::UnitTypes::Protoss_Nexus, baseLocationTilePos);
        if (built)
        {
            //numUnits = Tools::CountBuildingUnitsOfType(BWAPI::UnitTypes::Protoss_Gateway, BWAPI::Broodwar->self()->getUnits());
            //std::cout << "Gateway Built " << numUnits << std::endl;
            //BWAPI::Broodwar->printf("Started Building %s", BWAPI::UnitTypes::Protoss_Nexus.c_str());
            //return;
            //m_strategyManager.getUnitTypesMap()[BWAPI::UnitTypes::Protoss_Gateway]= numUnits;
            m_strategyManager.getBuildingStrategyManager().getLastBuiltLocation(1) = baseLocationTilePos;
        }
    }

    else if (!built && m_strategyManager.getNumberOfCompletedUnits(BWAPI::UnitTypes::Protoss_Nexus) > 1 && !m_strategyManager.getBuildingStrategyManager().isAdditionalSupplyNeeded())
    {
        int num = m_strategyManager.getNumberOfCompletedUnits(BWAPI::UnitTypes::Protoss_Nexus) - 1;

        BWAPI::Unit builder = m_strategyManager.getBaseManager().getWorkerFromBase(num);

        if (!builder) return;

        std::map<BWAPI::UnitType, int> map = m_strategyManager.getBuildingStrategyManager().getAdditionalBaseBuildingOrderMap();
        std::map<BWAPI::UnitType, int>::iterator it;

        bool built = false;

        for (it = map.begin(); it != map.end(); it++)
        {

            int mineralsLeft = BWAPI::Broodwar->self()->minerals();

            int mineralPrice = it->first.mineralPrice();

            int gasLeft = BWAPI::Broodwar->self()->gas();

            int gasPrice = it->first.gasPrice();

            int numUnits = m_strategyManager.getBaseManager().getBuildingsCount(num,it->first);


            if (it->first == BWAPI::UnitTypes::Protoss_Assimilator)
            {
                numUnits = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Assimilator, BWAPI::Broodwar->self()->getUnits());
            }


            if (BWAPI::Broodwar->self()->isUnitAvailable(it->first) && numUnits < m_strategyManager.getBuildingStrategyManager().getSecondaryBaseNumberOfBuildings(it->first) && mineralsLeft >= mineralPrice && gasLeft >= gasPrice)
            {

                if (it->first != BWAPI::UnitTypes::Protoss_Pylon  && m_strategyManager.getBaseManager().getBuildingsCount(num, BWAPI::UnitTypes::Protoss_Pylon) < 2)
                {
                    continue;
                }


 /*               if (Tools::checkIfBuildCommandAlreadyIssued(it->first, builder->getID()) && it->first != BWAPI::UnitTypes::Protoss_Pylon)
                {
                    continue;
                }*/

                secondaryBasePylon = m_strategyManager.getBuildingStrategyManager().getBuildingLocation(it->first, builder, num);
                built = builder->build(it->first, secondaryBasePylon);
                

                if (built)
                {
                    //numUnits = Tools::CountBuildingUnitsOfType(BWAPI::UnitTypes::Protoss_Gateway, BWAPI::Broodwar->self()->getUnits());
                    //std::cout << "Gateway Built " << numUnits << std::endl;
                    BWAPI::Broodwar->printf("Started Building %s", it->first.getName().c_str());
                    break;
                    //m_strategyManager.getUnitTypesMap()[BWAPI::UnitTypes::Protoss_Gateway]= numUnits;
                }
                else
                {
                    secondaryBasePylon = BWAPI::TilePositions::Invalid;
                }
            }
        }
    }

    if (built)
    {
        m_strategyManager.getBuildingStrategyManager().isBuildingBuiltNeeded() = true;
    }

}
bool isUnderAttack = false;
bool isScoutingAllowed = false;
int lastEnemyUnitID = -1;

void StarterBot::buildArmy()
{
    if (m_strategyManager.getBuildingStrategyManager().isAdditionalSupplyNeeded()) return;
    if (m_strategyManager.getBuildingStrategyManager().getWorkerID() != -1) return;

    std::cout << BWAPI::Broodwar->enemy()->getStartLocation() << std::endl;

    std::cout << BWAPI::Broodwar->enemy()->allUnitCount() << std::endl;

    BWAPI::Unit builder = Tools::GetTrainerUnitNotFullOfType(BWAPI::UnitTypes::Protoss_Zealot);
    if (builder)
    {
        if (!Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Templar_Archives, BWAPI::Broodwar->self()->getUnits()))
        {
            if (Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Zealot, BWAPI::Broodwar->self()->getUnits()) <= 10)
            {
                builder->train(BWAPI::UnitTypes::Protoss_Zealot);
            }
            if (Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Dragoon, BWAPI::Broodwar->self()->getUnits()) <= 10)
            {
                builder->train(BWAPI::UnitTypes::Protoss_Dragoon);
            }
        }
        else if (Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Dark_Templar, BWAPI::Broodwar->self()->getUnits()) < 10)
        {
            builder->train(BWAPI::UnitTypes::Protoss_Dark_Templar);
        }
        if (Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Corsair, BWAPI::Broodwar->self()->getUnits()) < 5)
        {
            builder = Tools::GetTrainerUnitNotFullOfType(BWAPI::UnitTypes::Protoss_Corsair);
            if (builder) builder->train(BWAPI::UnitTypes::Protoss_Corsair);
        }
        if (Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Robotics_Facility, BWAPI::Broodwar->self()->getUnits()) < 2)
        {
            builder = Tools::GetTrainerUnitNotFullOfType(BWAPI::UnitTypes::Protoss_Robotics_Facility);
            if (builder) builder->train(BWAPI::UnitTypes::Protoss_Observer);
        }

    }

    //if (isUnderAttack)
    //{
    //    isUnderAttack = false;

    //    for (auto it = zealots.begin(); it != zealots.end(); it++)
    //    {
    //        BWAPI::Unit unit = *it;
    //        BWAPI::Unit enemeyUnit = zealots.getClosestUnit(BWAPI::Filter::IsAttacking);
    //        if (enemeyUnit && enemeyUnit->getPlayer()->isEnemy(BWAPI::Broodwar->self()))
    //        {
    //            if(unit->getLastCommand().getUnit() != enemeyUnit) unit->attack(enemeyUnit);
    //            isUnderAttack = true;
    //        }
    //        else
    //        {
    //            enemeyUnit = zealots.getClosestUnit(BWAPI::Filter::IsEnemy);
    //            if (enemeyUnit)
    //            {
    //                if (unit->getLastCommand().getUnit() != enemeyUnit) unit->attack(enemeyUnit);
    //                isUnderAttack = true;
    //            }
    //        }
    //    }
    //}

    //else


    if (isUnderAttack)
    {
        isUnderAttack = false;

        //BWAPI::Unit enemeyUnit = zealots.getClosestUnit(BWAPI::Filter::IsEnemy, 512);
        for (auto it = zealots.begin(); it != zealots.end(); it++)
        {
            BWAPI::Unit u = *it;
            if (u && u->exists())
            {
                BWAPI::Unit enemyUnit = u->getClosestUnit(BWAPI::Filter::IsEnemy, 512);
                if (enemyUnit && u->getLastCommand().getTarget() && u->getLastCommand().getTarget()->getID() != enemyUnit->getID())
                {
                    u->attack(enemyUnit);
                    isUnderAttack = true;
                }
            }
        }
    }
    else if (!isUnderAttack && isScoutingAllowed)
    {
        bool isScoutingCompleted = true;

        //isUnderAttack = false;
        for (BWAPI::TilePosition pos : BWAPI::Broodwar->getStartLocations())
        {
            if (!BWAPI::Broodwar->isExplored(pos))
            {
                for (auto it = zealots.begin(); it != zealots.end(); it++)
                {
                    BWAPI::Unit u = *it;
                    if (u && u->exists())
                    {
                        BWAPI::Unit enemyUnit = u->getClosestUnit(BWAPI::Filter::IsEnemy, 512);
                        if (enemyUnit && (u->getLastCommand().getType() != BWAPI::UnitCommandTypes::Attack_Unit || (u->getLastCommand().getTarget() && u->getLastCommand().getTarget()->getID() != enemyUnit->getID())))
                        {
                            u->attack(enemyUnit);
                            isUnderAttack = true;
                        }
                    }
                }

            }
            if (isUnderAttack)
            {
                break;
            }
            else if (!BWAPI::Broodwar->isExplored(pos))
            {
                zealots.move(BWAPI::Position(pos));
                isScoutingCompleted = false;
            }

        }

        if (isScoutingCompleted)
        {
            
        }

    }


}

// Called whenever a unit is destroyed, with a pointer to the unit
void StarterBot::onUnitDestroy(BWAPI::Unit unit)
{
    if (unit->getType().isBuilding() && !unit->getPlayer()->isEnemy(BWAPI::Broodwar->self()))
    {
        int& num = m_strategyManager.getNumberOfUnits(unit->getType());
        num--;

        int& completedNum = m_strategyManager.getNumberOfCompletedUnits(unit->getType());
        completedNum--;
    }
    if (unit != nullptr && !unit->getPlayer()->isEnemy(BWAPI::Broodwar->self()) && (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot || unit->getType() == BWAPI::UnitTypes::Protoss_Dragoon || unit->getType() == BWAPI::UnitTypes::Protoss_Dark_Templar))
    {
        if (zealots.size() > 0)
        {
            zealots.erase(unit);
        }
        if (zealots.size() == 0) isScoutingAllowed = false;
        if (defenders.size() > 0)
        {
            defenders.erase(unit);
        }

        //for (auto it = defenders.begin(); it != defenders.end(); it++)
        //{
        //    BWAPI::Unit u = *it;
        //    if (unit->getID() == u->getID()) defenders.erase(*it);
        //}

        //for (auto it = zealots.begin(); it != zealots.end(); it++)
        //{
        //    BWAPI::Unit u = *it;
        //    if (unit->getID() == u->getID()) zealots.erase(*it);
        //}

    }
    if (unit->getPlayer()->isEnemy(BWAPI::Broodwar->self()))
    {
        std::cout << "Enemy killed: " << unit->getType() <<std::endl;
    }

    if (!unit->getPlayer()->isEnemy(BWAPI::Broodwar->self()))
    {
        m_strategyManager.getBaseManager().removeUnitFromBase(unit);
    }


}

// Called whenever a unit is morphed, with a pointer to the unit
// Zerg units morph when they turn into other units
void StarterBot::onUnitMorph(BWAPI::Unit unit)
{

}

// Called whenever a text is sent to the game by a user
void StarterBot::onSendText(std::string text)
{
    if (text == "map")
    {
        m_mapTools.toggleDraw();
    }
    BWAPI::Broodwar->sendText("%s", text.c_str());
}

// Called whenever a unit is created, with a pointer to the destroyed unit
// Units are created in buildings like barracks before they are visible, 
// so this will trigger when you issue the build command for most units
void StarterBot::onUnitCreate(BWAPI::Unit unit)
{
    if (unit->getPlayer()->isEnemy(BWAPI::Broodwar->self()))
    {
        return;
    }
    
    int& numUnits = m_strategyManager.getNumberOfUnits(unit->getType());
    numUnits++;

    if (unit->getType() == BWAPI::UnitTypes::Protoss_Nexus)
    {
        int& ID = m_strategyManager.getBuildingStrategyManager().getWorkerID();
        ID = -1;
        m_strategyManager.getBuildingStrategyManager().getLastBuiltLocation(numUnits - 1) = unit->getTilePosition();
        m_strategyManager.getBaseManager().addOrUpdateBase(unit, numUnits == 1);
        m_strategyManager.getBaseManager().addUnitToBase(unit, numUnits - 1);
    }
    else
    {
        BWAPI::Unit builder = unit->getBuildUnit();

        if (unit->getType().isBuilding())
        {
            for (auto u : BWAPI::Broodwar->self()->getUnits())
            {
                if (u->isConstructing() && u->getType().isWorker())
                {
                    builder = u;
                    break;
                }
            }
        }
        
        m_strategyManager.getBaseManager().addUnitToBase(unit, builder);

        if (unit->getType().isBuilding() && unit->getType() != BWAPI::UnitTypes::Protoss_Pylon)
        {
            m_strategyManager.getBuildingStrategyManager().isBuildingBuiltNeeded() = false;;
        }
    }

}

// Called whenever a unit finished construction, with a pointer to the unit
void StarterBot::onUnitComplete(BWAPI::Unit unit)
{

    int& numCompletedUnits = m_strategyManager.getNumberOfCompletedUnits(unit->getType());
    numCompletedUnits++;

    if (unit->getType() == BWAPI::UnitTypes::Protoss_Nexus && numCompletedUnits > 1)
    {
        m_workersWanted += 5;
    }

    if (unit->getType() == BWAPI::UnitTypes::Protoss_Pylon)
    {
        m_strategyManager.getBuildingStrategyManager().isAdditionalSupplyNeeded() = false;
    }
    if (unit->getType().isBuilding())
    {
        if (unit->getType().isRefinery())
        {
            m_workersWanted += 2;
        }

        //m_strategyManager.getBaseManager().addUnitToBase(unit,m_strategyManager.getNumberOfCompletedUnits(BWAPI::UnitTypes::Protoss_Nexus));

        //if (unit->getType().buildsWhat().count(BWAPI::UnitTypes::Protoss_Zealot))
        //{
        //    int& numUnits = m_strategyManager.getNumberOfUnits(BWAPI::UnitTypes::Protoss_Zealot);
        //    int& numDragoonUnits = m_strategyManager.getNumberOfUnits(BWAPI::UnitTypes::Protoss_Dragoon);
        //    if (numUnits < m_strategyManager.getUnitStrategyManager().getNumberOfUnits(BWAPI::UnitTypes::Protoss_Zealot))
        //    {
        //        //numUnits++;
        //        if (unit->train(BWAPI::UnitTypes::Protoss_Zealot)) m_strategyManager.getUnitTypesMap()[BWAPI::UnitTypes::Protoss_Zealot] = numUnits;
        //        numUnits = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Zealot,BWAPI::Broodwar->self()->getUnits());
        //    }
        //    if (numDragoonUnits < m_strategyManager.getUnitStrategyManager().getNumberOfUnits(BWAPI::UnitTypes::Protoss_Dragoon))
        //    {
        //        //numDragoonUnits++;
        //        if (unit->train(BWAPI::UnitTypes::Protoss_Dragoon)) m_strategyManager.getUnitTypesMap()[BWAPI::UnitTypes::Protoss_Dragoon] = numDragoonUnits;
        //        numUnits = Tools::CountUnitsOfType(BWAPI::UnitTypes::Protoss_Dragoon, BWAPI::Broodwar->self()->getUnits());
        //    }
        //}
        //else if (unit->getType().isRefinery())
        //{
        //    Tools::GetDepot()->train(BWAPI::UnitTypes::Protoss_Probe);
        //    Tools::GetDepot()->train(BWAPI::UnitTypes::Protoss_Probe);
        //}
    }

    else if (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot || unit->getType() == BWAPI::UnitTypes::Protoss_Dragoon || unit->getType() == BWAPI::UnitTypes::Protoss_Dark_Templar)
    {
        if (defenders.size() < 10) 
        {
            defenders.emplace(unit);
        }
        else if ((zealots.size() < 10 && !isUnderAttack))
        {
            if(!isScoutingAllowed) zealots.emplace(unit);
            //isUnderAttack = false;

            if (zealots.size() == 10)
            {
                isScoutingAllowed = true;
            }
        }
        else
        {
            //defenders.emplace(unit);
        }
    }
    else
    {
        workers.emplace(unit);
    }
   
}

// Called whenever a unit appears, with a pointer to the destroyed unit
// This is usually triggered when units appear from fog of war and become visible
void StarterBot::onUnitShow(BWAPI::Unit unit)
{ 
    //std::cout << unit->getType().getName();
}

void StarterBot::onUnitDiscover(BWAPI::Unit unit)
{
    if (unit->getPlayer()->isEnemy(BWAPI::Broodwar->self()) && unit->getType() == BWAPI::UnitTypes::Zerg_Zergling && unit->getDistance(BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation())) < 1000 && defenders.size() < 2)
    {
        isZerglingRush = true;
    }
    else if (unit->getPlayer()->isEnemy(BWAPI::Broodwar->self()) && unit->getDistance(BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation())) < 1000)
    {
        if (unit->getDistance(zealots.getPosition()) < 1000)
        {
            for (auto u: zealots)
            {
                if (u && (u->getLastCommand().getType() != BWAPI::UnitCommandTypes::Attack_Unit) || ( u->getLastCommand().getTarget() && u->getLastCommand().getTarget()->getID() != unit->getID()))
                {
                    std::cout << "base attackers in action " << std::endl;
                    u->attack(unit);
                }
            }
        }
        //BWAPI::Unit u = unit->getClosestUnit(BWAPI::Filter::IsEnemy);
        //if(u && !u->getType().isBuilding() && u->canAttack() && !u->isAttacking() && !u->getType().isWorker()) defenders.attack(unit);
        for (auto u : defenders)
        {
            if (u && (u->getLastCommand().getType() != BWAPI::UnitCommandTypes::Attack_Unit) || (u->getLastCommand().getTarget() && u->getLastCommand().getTarget()->getID() != unit->getID()))
            {
                std::cout << "Defenders in action " << std::endl;
                u->attack(unit);
            }
        }
    }

}

// Called whenever a unit gets hidden, with a pointer to the destroyed unit
// This is usually triggered when units enter the fog of war and are no longer visible
void StarterBot::onUnitHide(BWAPI::Unit unit)
{ 
	
}

// Called whenever a unit switches player control
// This usually happens when a dark archon takes control of a unit
void StarterBot::onUnitRenegade(BWAPI::Unit unit)
{ 
	
}

