#pragma once
#include "WfcModel.hpp"
#include "BitmapHelper.hpp"
#include "GridHelper.h"

class OverlappingModel : public WfcModel {

public:
	OverlappingModel(const String& name, int32 N, const Size& gridSize, bool periodicInput, bool periodic, int32 symmetry, bool ground, Heuristic heuristic);

	Image ToImage() const;

	inline const Size& ImageSize() const
	{
		return gridSize;
	}

private:
	Array<Grid<uint8>> patterns;

	Array<Color> colors;
};

