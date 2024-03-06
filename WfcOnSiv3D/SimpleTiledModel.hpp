# pragma once
# include "WfcModel.hpp"
# include "BitmapHelper.hpp"
# include "GridHelper.h"

# pragma once
class SimpleTiledModel : public WfcModel
{
public:

	SimpleTiledModel(const String& jsonPath, const String& subsetName, const Size& gridSize, bool periodic, bool blackBackground, Heuristic heuristic);

	Image toImage() const;

	inline int32 tilesize() const {
		return m_tilesize;
	}

	inline Size imageSize() const
	{
		return m_gridSize * m_tilesize;
	}


private:
	Array<Grid<Color>> m_tiles;
	Array<String> m_tilenames;
	int32 m_tilesize = 0;
	bool m_blackBackground;
};
