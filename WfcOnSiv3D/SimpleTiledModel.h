#pragma once
#include "WfcModel.h"
#include "BitmapHelper.h"

#pragma once
class SimpleTiledModel : public WfcModel
{

private:
	Array<Array<int32>> tiles;
	Array<String> tilenames;
	int32 tilesize = 0;
	bool blackBackground;

public:

	SimpleTiledModel(String jsonPath, String subsetName, int32 width, int32 height, bool periodic, bool blackBackground, Heuristic heuristic)
		: WfcModel(width, height, 1, periodic, heuristic) , blackBackground(blackBackground)
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

		static auto tile = [](const std::function<int32(int32, int32)>& f, int32 size) {
			Array<int32> result(size * size, 0);
			for (int32 y = 0; y < size; y++) {
				for (int32 x = 0; x < size; x++) {
					result[x + y * size] = f(x, y);
				}
			}
			return result;
		};

		static auto rotate = [](const Array<int32>& array, int32 size) {
			return tile([&](int32 x, int32 y) { return array[size - 1 - y + x * size]; }, size);
		};

		static auto reflect = [](const Array<int32>& array, int32 size) {
			return tile([&](int32 x, int32 y) { return array[size - 1 - x + y * size]; }, size);
		};




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

			auto sym = jTile.hasElement(U"symmetry") ? jTile[U"symmetry"].getString() : U"C";

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

			T = action.size();
			firstOccurrence.emplace(tilename, T);

			Array<Array<int32>> map(cardinality, Array<int32>(8,0));
			for (int t = 0; t < cardinality; t++)
			{
				map[t][0] = t;
				map[t][1] = a(t);
				map[t][2] = a(a(t));
				map[t][3] = a(a(a(t)));
				map[t][4] = b(t);
				map[t][5] = b(a(t));
				map[t][6] = b(a(a(t)));
				map[t][7] = b(a(a(a(t))));

				for (int s = 0; s < 8; s++) {
					map[t][s] += T;
				}

				action << map[t];
			}

			if (unique)
			{
				for (int t = 0; t < cardinality; t++)
				{
					auto x = U"tilesets/{}/{} {}.png"_fmt(jsonFileName, tilename, t);
					auto [bitmap, _w, _h] = BitmapHelper::LoadBitmap(U"tilesets/{}/{} {}.png"_fmt(jsonFileName, tilename, t));
					tilesize = _w;

					tiles << bitmap;
					tilenames << U"{} {}"_fmt(tilename,t);
				}
			}
			else
			{
				auto [bitmap, _w, _h] = BitmapHelper::LoadBitmap(U"tilesets/{}/{}.png"_fmt(jsonFileName, tilename));
				tilesize = _w;

				tiles << bitmap;
				tilenames << U"{} 0"_fmt(tilename);

				for (int32 t = 1; t < cardinality; t++)
				{
					if (t <= 3) {
						auto test = tiles[T + t - 1];
						tiles << (rotate(tiles[T + t - 1], tilesize));
					}
					if (t >= 4) {
						tiles << (reflect(tiles[T + t - 4], tilesize));
					}
					tilenames << U"{} {}"_fmt(tilename, t);
				}
			}

			for (int t = 0; t < cardinality; t++) {
				weightList << (jTile.hasElement(U"weight") ? jTile[U"weight"].get<double>() : 1.0);
			}
		}

		
		T = action.size();
		weights = Array<double>(weightList);

		propagator.resize(4);
		Array<Array<Array<bool>>> densePropagator(4, Array<Array<bool>>(T, Array<bool>(T)));

		for (int32 d = 0; d < 4; ++d) {
			propagator[d].resize(T);
			for (int32 t = 0; t < T; ++t) {
				densePropagator[d][t] = Array<bool>(T, false);
			}
		}

		for (const auto&& [key, jNeighbor] : jroot[U"neighbors"][U"neighbor"]) {
			Array<String> left = jNeighbor[U"left"].getString().split(U' ');
			Array<String> right = jNeighbor[U"right"].getString().split(U' ');

			if (not subset.isEmpty() && (not subset.contains(left[0]) || not subset.contains(right[0]))) {
				continue;
			}

			int32 L = action[firstOccurrence[left[0]]][left.size() == 1 ? 0 : Parse<int32>(left[1])];
			int32 D = action[L][1];
			int32 R = action[firstOccurrence[right[0]]][right.size() == 1 ? 0 : Parse<int32>(right[1])];
			int32 U = action[R][1];

			densePropagator[0][R][L] = true;
			densePropagator[0][action[R][6]][action[L][6]] = true;
			densePropagator[0][action[L][4]][action[R][4]] = true;
			densePropagator[0][action[L][2]][action[R][2]] = true;

			densePropagator[1][U][D] = true;
			densePropagator[1][action[D][6]][action[U][6]] = true;
			densePropagator[1][action[U][4]][action[D][4]] = true;
			densePropagator[1][action[D][2]][action[U][2]] = true;
		}

		for (int t2 = 0; t2 < T; ++t2) {
			for (int t1 = 0; t1 < T; ++t1) {
				densePropagator[2][t2][t1] = densePropagator[0][t1][t2];
				densePropagator[3][t2][t1] = densePropagator[1][t1][t2];
			}
		}


		Array<Array<Array<int32>>> sparsePropagator(4, Array<Array<int32>>(T));
		for (int d = 0; d < 4; ++d) {
			for (int t = 0; t < T; ++t) {
				sparsePropagator[d][t] = Array<int>();
			}
		}

		for (int32 d = 0; d < 4; ++d) {
			for (int32 t1 = 0; t1 < T; ++t1) {
				Array<int32>& sp = sparsePropagator[d][t1];
				Array<bool>& tp = densePropagator[d][t1];

				for (int32 t2 = 0; t2 < T; ++t2) {
					if (tp[t2]) {
						sp << (t2);
					}
				}

				int32 ST = sp.size();
				if (ST == 0) {
					std::cout << "ERROR: tile " << tilenames[t1] << " has no neighbors in direction " << d << std::endl;
				}

				propagator[d][t1].resize(ST);
				for (int st = 0; st < ST; ++st) {
					propagator[d][t1][st] = sp[st];
				}
			}
		}
	}

	Image ToImage()
	{
		Array<int32> bitmapData(MX * MY * tilesize * tilesize, 0);
		if (observed[0] >= 0)
		{
			for (int32 x = 0; x < MX; x++) {
				for (int32 y = 0; y < MY; y++)
				{
					const auto& tile = tiles[observed[x + y * MX]];
					for (int32 dy = 0; dy < tilesize; dy++) {
						for (int32 dx = 0; dx < tilesize; dx++) {
							const int32& pixel = tile[dx + dy * tilesize];
							bitmapData[x * tilesize + dx + (y * tilesize + dy) * MX * tilesize] = pixel;
						}
					}
				}
			}
		}
		else
		{
			for (int32 i = 0; i < wave.size(); i++)
			{
				int32 x = i % MX, y = i / MX;
				if (blackBackground && sumsOfOnes[i] == T) {
					for (int32 yt = 0; yt < tilesize; yt++) {
						for (int32 xt = 0; xt < tilesize; xt++) {
							bitmapData[x * tilesize + xt + (y * tilesize + yt) * MX * tilesize] = 255 << 24;
						}
					}
				}
				else
				{
					Array<bool> w = wave[i];
					double normalization{ 1.0 / sumsOfWeights[i] };
					for (int32 yt = 0; yt < tilesize; yt++) {
						for (int32 xt = 0; xt < tilesize; xt++){
							int32 idi = x * tilesize + xt + (y * tilesize + yt) * MX * tilesize;

							double r{ 0 };
							double g{ 0 };
							double b{ 0 };

							for (int32 t = 0; t < T; t++) {
								if (w[t])
								{
									int32 argb = tiles[t][xt + yt * tilesize];
									r += ((argb & 0xff0000) >> 16) * weights[t] * normalization;
									g += ((argb & 0x00ff00) >>  8) * weights[t] * normalization;
									b += ((argb & 0x0000ff) >>  0) * weights[t] * normalization;
								}
							}
							bitmapData[idi] =
								(static_cast<int32>(0xff000000)) |
								(static_cast<int32>(r) << 16) |
								(static_cast<int32>(g) <<  8) |
								(static_cast<int32>(b) <<  0);
						}
					}
				}
			}
		}
		return BitmapHelper::ToImage(bitmapData, MX * tilesize, MY * tilesize);
	}

	Image ToTileImage(int32 index) {
		Array<int32> bitmapData(tilesize * tilesize, 0);
		const auto& tile = tiles[index];
		for (int32 y = 0; y < tilesize; y++) {
			for (int32 x = 0; x < tilesize; x++) {
				const int32& pixel = tile[x + y * tilesize];
				bitmapData[x + y *  tilesize] = pixel;
			}
		}
		return BitmapHelper::ToImage(bitmapData, tilesize, tilesize);
	}

};
