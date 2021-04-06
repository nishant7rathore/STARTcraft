#include "BuildingStrategyManager.h"

BuildingStrategyManager::BuildingStrategyManager() : m_lastBuiltLocation (BWAPI::Broodwar->self()->getStartLocation())
{
    this->m_buildingBuidOrder.clear();

    this->m_buildingBuidOrder.emplace(BWAPI::UnitTypes::Protoss_Cybernetics_Core, 1);
    this->m_buildingBuidOrder.emplace(BWAPI::UnitTypes::Protoss_Citadel_of_Adun, 1);
    this->m_buildingBuidOrder.emplace(BWAPI::UnitTypes::Protoss_Photon_Cannon, 5);
    this->m_buildingBuidOrder.emplace(BWAPI::UnitTypes::Protoss_Gateway, 2);
    this->m_buildingBuidOrder.emplace(BWAPI::UnitTypes::Protoss_Forge, 1);
    this->m_buildingBuidOrder.emplace(BWAPI::UnitTypes::Protoss_Stargate, 1);
    this->m_buildingBuidOrder.emplace(BWAPI::UnitTypes::Protoss_Templar_Archives, 1);
}

//BFS Node
struct BFSNode
{
    int x;
    int y;
    int position;
    BFSNode* parent;

    BFSNode(int x, int y, int position, BFSNode* parent)
    {
        this->x = x;
        this->y = y;
        this->position = position;
        this->parent = parent;
    }
};

//Direction struct for BFS
struct Direction
{
    int x = 0;
    int y = 0;
};

struct Direction directions[4] = { {1,0},{0,1},{-1,0},{0,-1} }; // legal Directions for BFS 

std::vector<BFSNode> openList; // BFS open list
std::map<std::string, bool> closedList; // BFS closed map

std::map<std::string, bool>::iterator it; // iterator to loop over closed list
std::vector<BFSNode> path; // final path 


BWAPI::TilePosition& BuildingStrategyManager::getLastBuiltLocation()
{
    return m_lastBuiltLocation;
}


BWAPI::TilePosition BuildingStrategyManager::getBuildingLocation(BWAPI::UnitType building, BWAPI::Unit builder, BWAPI::TilePosition lastBuiltLocation)
{

    openList.clear();
    closedList.clear();


    openList.push_back(BFSNode(m_lastBuiltLocation.x, m_lastBuiltLocation.y, 0, nullptr));

    size_t size = openList.size();

    for (size_t i = 0; i < size; i++)
    {
        BFSNode node = openList[i];

        it = closedList.find(std::to_string(node.x) + std::to_string(node.y));
        if (it != closedList.end()) continue;
            
        closedList.emplace(std::to_string(node.x) + std::to_string(node.y), true);

        for (size_t d = 0; d < 4; d++)
        {
            
            int x = node.x + directions[d].x;
            int y = node.y + directions[d].y;

            it = closedList.find(std::to_string(x) + std::to_string(y));

            BWAPI::TilePosition childPos = BWAPI::TilePosition(x, y);

            if (childPos.isValid() && BWAPI::Broodwar->canBuildHere(childPos, building))
            {
                bool isSafe = true;

                for (int tileX = 0; tileX<building.tileWidth(); tileX++)
                {
                    for (int tileY = 0; tileY < building.tileHeight(); tileY++)
                    {
                        if (BWAPI::Broodwar->getUnitsOnTile(childPos.x+tileX, childPos.y+tileY).size() > 0)
                        {
                            isSafe = false;
                        }
                    }
                }
                
                if (isSafe || building.isRefinery())
                {
                    m_lastBuiltLocation.x = childPos.x;
                    m_lastBuiltLocation.y = childPos.y;
                    return m_lastBuiltLocation;
                }
            }

            if (childPos.isValid() && it == closedList.end())
            {
                openList.push_back(BFSNode(x,y,node.position++,&node));
                size++;
            }

        }

    }
    return BWAPI::TilePositions::None;
}

int BuildingStrategyManager::getNumberOfBuildings(BWAPI::UnitType building)
{
    return m_buildingBuidOrder[building];
}

std::map<BWAPI::UnitType, int> BuildingStrategyManager::getBuildingOrderMap()
{
    return m_buildingBuidOrder;
}