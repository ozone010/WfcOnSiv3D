# include "stdafx.h"
# include "SimpleTiledModel.hpp"

SimpleTiledModel::SimpleTiledModel(const String& jsonPath, const String& subsetName, const Size& m_gridSize, bool periodic, bool blackBackground, Heuristic heuristic)
	: WfcModel(m_gridSize, 1, periodic, heuristic), m_blackBackground(blackBackground)
{
	const auto jsonFileName = FileSystem::BaseName(jsonPath);

	const JSON json = JSON::Load(jsonPath);
	const JSON& jroot = json[U"set"];

	bool unique = jroot.hasElement(U"unique") ? jroot[U"unique"].get<bool>() : false;

	Array<String> subset;
	if (not subsetName.isEmpty())
	{
		JSON selectedJSubset;
		for (auto&& [key, jSubset] : jroot[U"subsets"][U"subset"]) {
			if (jSubset[U"name"].getString() == subsetName) {
				selectedJSubset = jSubset;
				break;
			}
		}
		for (auto&& [key, tile] : selectedJSubset[U"tile"]) {
			subset << tile[U"name"].getString();
		}
	}

	Array<double> weightList;
	Array<Array<int32>> action;

	HashTable<String, int32> firstOccurrence;

	for (const auto&& [key, jTile] : jroot[U"tiles"][U"tile"]) {

		auto const tilename = jTile[U"name"].getString();
		if (not subset.isEmpty() && not subset.contains(tilename)) {
			continue;
		}

		//a is 90 degrees rotation
		std::function<int32(int32)> a;

		//b is reflection
		std::function<int32(int32)> b;

		const auto sym = jTile.hasElement(U"symmetry") ? jTile[U"symmetry"].getString() : U"C";

		int32 cardinality = 0;
		if (sym == U"L")
		{
			cardinality = 4;
			a = [](int32 i) { return (i + 1) % 4; };
			b = [](int32 i) { return i % 2 == 0 ? i + 1 : i - 1; };
		}
		else if (sym == U"T")
		{
			cardinality = 4;
			a = [](int32 i) { return (i + 1) % 4; };
			b = [](int32 i) { return i % 2 == 0 ? i : 4 - i; };
		}
		else if (sym == U"I")
		{
			cardinality = 2;
			a = [](int32 i) { return 1 - i; };
			b = [](int32 i) { return i; };
		}
		else if (sym == U"\\")
		{
			cardinality = 2;
			a = [](int32 i) { return 1 - i; };
			b = [](int32 i) { return 1 - i; };
		}
		else if (sym == U"F")
		{
			cardinality = 8;
			a = [](int32 i) { return i < 4 ? (i + 1) % 4 : 4 + (i - 1) % 4; };
			b = [](int32 i) { return i < 4 ? i + 4 : i - 4; };
		}
		else
		{
			cardinality = 1;
			a = [](int32 i) { return i; };
			b = [](int32 i) { return i; };
		}

		m_T = action.size();
		firstOccurrence.emplace(tilename, m_T);

		Array<Array<int32>> map(cardinality, Array<int32>(8, 0));
		for (int t = 0; t < cardinality; ++t)
		{
			map[t][0] = t;
			map[t][1] = a(t);
			map[t][2] = a(a(t));
			map[t][3] = a(a(a(t)));
			map[t][4] = b(t);
			map[t][5] = b(a(t));
			map[t][6] = b(a(a(t)));
			map[t][7] = b(a(a(a(t))));

			for (int s = 0; s < 8; ++s) {
				map[t][s] += m_T;
			}

			action << map[t];
		}

		if (unique)
		{
			for (auto t = 0; t < cardinality; ++t)
			{
				auto x = U"tilesets/{}/{} {}.png"_fmt(jsonFileName, tilename, t);
				auto bitmap = BitmapHelper::LoadBitmap(U"tilesets/{}/{} {}.png"_fmt(jsonFileName, tilename, t));
				m_tilesize = bitmap.width();

				m_tiles << bitmap;
				m_tilenames << U"{} {}"_fmt(tilename, t);
			}
		}
		else
		{
			auto bitmap = BitmapHelper::LoadBitmap(U"tilesets/{}/{}.png"_fmt(jsonFileName, tilename));
			m_tilesize = bitmap.width();

			m_tiles << bitmap;
			m_tilenames << U"{} 0"_fmt(tilename);

			for (auto t = 1; t < cardinality; ++t)
			{
				if (t <= 3) {
					m_tiles << GridHelper::rotated270(m_tiles[m_T + t - 1]);
				}
				if (t >= 4) {
					m_tiles << GridHelper::mirrored(m_tiles[m_T + t - 4]);
				}
				m_tilenames << U"{} {}"_fmt(tilename, t);
			}
		}

		for (auto t = 0; t < cardinality; ++t) {
			weightList << (jTile.hasElement(U"weight") ? jTile[U"weight"].get<double>() : 1.0);
		}
	}


	m_T = action.size();
	m_weights = Array<double>(weightList);

	m_propagator.resize(4);
	Array<Array<Array<bool>>> densem_propagator(4, Array<Array<bool>>(m_T, Array<bool>(m_T)));

	for (auto d = 0; d < 4; ++d) {
		m_propagator[d].resize(m_T);
		for (auto t = 0; t < m_T; ++t) {
			densem_propagator[d][t] = Array<bool>(m_T, false);
		}
	}

	for (const auto&& [key, jNeighbor] : jroot[U"neighbors"][U"neighbor"]) {
		const Array<String> left = jNeighbor[U"left"].getString().split(U' ');
		const Array<String> right = jNeighbor[U"right"].getString().split(U' ');

		if (not subset.isEmpty() && (not subset.contains(left[0]) || not subset.contains(right[0]))) {
			continue;
		}

		const int32 L = action[firstOccurrence[left[0]]][left.size() == 1 ? 0 : Parse<int32>(left[1])];
		const int32 D = action[L][1];
		const int32 R = action[firstOccurrence[right[0]]][right.size() == 1 ? 0 : Parse<int32>(right[1])];
		const int32 U = action[R][1];

		densem_propagator[0][R][L] = true;
		densem_propagator[0][action[R][6]][action[L][6]] = true;
		densem_propagator[0][action[L][4]][action[R][4]] = true;
		densem_propagator[0][action[L][2]][action[R][2]] = true;

		densem_propagator[1][U][D] = true;
		densem_propagator[1][action[D][6]][action[U][6]] = true;
		densem_propagator[1][action[U][4]][action[D][4]] = true;
		densem_propagator[1][action[D][2]][action[U][2]] = true;
	}

	for (auto t2 = 0; t2 < m_T; ++t2) {
		for (auto t1 = 0; t1 < m_T; ++t1) {
			densem_propagator[2][t2][t1] = densem_propagator[0][t1][t2];
			densem_propagator[3][t2][t1] = densem_propagator[1][t1][t2];
		}
	}


	Array<Array<Array<int32>>> sparsem_propagator(4, Array<Array<int32>>(m_T));
	for (auto d = 0; d < 4; ++d) {
		for (auto t = 0; t < m_T; ++t) {
			sparsem_propagator[d][t] = Array<int>();
		}
	}

	for (auto d = 0; d < 4; ++d) {
		for (auto t1 = 0; t1 < m_T; ++t1) {
			Array<int32>& sp = sparsem_propagator[d][t1];
			Array<bool>& tp = densem_propagator[d][t1];

			for (int32 t2 = 0; t2 < m_T; ++t2) {
				if (tp[t2]) {
					sp << (t2);
				}
			}

			const int32 ST = sp.size();
			if (ST == 0) {
				std::cout << "ERROR: tile " << m_tilenames[t1] << " has no neighbors in direction " << d << std::endl;
			}

			m_propagator[d][t1].resize(ST);
			for (int st = 0; st < ST; ++st) {
				m_propagator[d][t1][st] = sp[st];
			}
		}
	}
}

Image SimpleTiledModel::toImage() const
{
	Grid<Color> bitmapData(m_gridSize * m_tilesize);
	if (m_observed[0][0] >= 0)
	{
		for (int32 x = 0; x < m_gridSize.x; ++x) {
			for (int32 y = 0; y < m_gridSize.y; ++y)
			{
				const auto& tile = m_tiles[m_observed[y][x]];
				for (int32 dy = 0; dy < m_tilesize; ++dy) {
					for (int32 dx = 0; dx < m_tilesize; ++dx) {
						const auto& pixel = tile[dy][dx];
						bitmapData[y * m_tilesize + dy][x * m_tilesize + dx] = pixel;
					}
				}
			}
		}
	}
	else
	{
		for (auto x : step(m_wave.height())) {
			for (auto y : step(m_wave.width())) {
				if (m_blackBackground && m_sumsOfOnes[y][x] == m_T) {
					for (int32 yt = 0; yt < m_tilesize; ++yt) {
						for (int32 xt = 0; xt < m_tilesize; ++xt) {
							bitmapData[y * m_tilesize + yt][x * m_tilesize + xt] = Color(0, 0, 0, 255);
						}
					}
				}
				else
				{
					Array<bool> w = m_wave[y][x];
					double normalization{ 1.0 / m_sumsOfWeights[y][x] };
					for (int32 yt = 0; yt < m_tilesize; ++yt) {
						for (int32 xt = 0; xt < m_tilesize; ++xt) {
							double r{ 0 };
							double g{ 0 };
							double b{ 0 };

							for (int32 t = 0; t < m_T; ++t) {
								if (w[t])
								{
									const auto& argb = m_tiles[t][yt][xt];
									r += argb.r * m_weights[t] * normalization;
									g += argb.g * m_weights[t] * normalization;
									b += argb.b * m_weights[t] * normalization;
								}
							}
							bitmapData[y * m_tilesize + yt][x * m_tilesize + xt] =
								Color(
									static_cast<uint8>(r),
									static_cast<uint8>(g),
									static_cast<uint8>(b)
								);
						}
					}
				}
			}
		}
	}
	return BitmapHelper::ToImage(bitmapData);
}
