#pragma once
#include "WfcModel.hpp"
#include "BitmapHelper.hpp"
#include "GridHelper.h"

#pragma once
class SimpleTiledModel : public WfcModel
{
public:

	SimpleTiledModel(const String& jsonPath, const String& subsetName, const Size& gridSize, bool periodic, bool blackBackground, Heuristic heuristic);

	Image ToImage() const;

	inline int32 Tilesize() const {
		return tilesize;
	}

	inline Size ImageSize() const
	{
		return gridSize * tilesize;
	}


private:
	Array<Grid<Color>> tiles;
	Array<String> tilenames;
	int32 tilesize = 0;
	bool blackBackground;
};
