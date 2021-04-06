#pragma once
#include "map"
#include "BWAPI.h"
class UnitStrategyManager
{
public:
	UnitStrategyManager();

	std::map<BWAPI::UnitType,int> m_unitBuildOrder;

	//BWAPI::TilePosition getUnitLocation(BWAPI::UnitType unit);
	int getNumberOfUnits(BWAPI::UnitType unit);
};
