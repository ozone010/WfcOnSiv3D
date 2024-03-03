#pragma once
#include "WfcModel.h"
#include "BitmapHelper.h"
#include "GridHelper.h"

class OverlappingModel : public WfcModel {
public:
	Array<Grid<uint8>> patterns;
	Array<Color> colors;

	OverlappingModel(const String& name, int32 N, const Size& gridSize, bool periodicInput, bool periodic, int32 symmetry, bool ground, Heuristic heuristic)
		: WfcModel(gridSize, N, periodic, heuristic) {
		auto bitmap = BitmapHelper::LoadBitmap(name);

		Grid<uint8> sample(bitmap.size());

		colors.clear();

		for (auto i : step(sample.height())) {
			for (auto d : step(sample.width())) {

				const Color& color = bitmap[i][d];
				int32 k = 0;

				for (; k < colors.size(); k++) {
					if (colors[k] == color) {
						break;
					}
				}
				if (k == colors.size()) {
					colors << color;
				}
				sample[i][d] = static_cast<uint8>(k);
			}
		}

		static auto pattern = [](const std::function<uint8(int32, int32)>& f, int32 N) {
			Grid<uint8> result(N , N);
			for (int32 y = 0; y < N; y++) {
				for (int32 x = 0; x < N; x++) {
					result[y][x] = f(x, y);
				}
			}
			return result;
		};

		static auto hash = [](const Grid<uint8>& p, int32 C) {
			int64 result = 0;
			int64 power = 1;

			for (auto y : step(p.height())) {
				for (auto x : step(p.width())) {
					result += p.asArray()[p.num_elements() - 1 - (x + y * p.width())] * power;
					power *= C;
				}
			}
			return result;
		};

		patterns.clear();
		HashTable<int64, int32> patternIndices;
		Array<double> weightList;

		int32 C = colors.size();
		int32 xmax = periodicInput ? bitmap.width() : bitmap.width() - N + 1;
		int32 ymax = periodicInput ? bitmap.height() : bitmap.height() - N + 1;
		for (int32 y = 0; y < ymax; y++) {
			for (int32 x = 0; x < xmax; x++) {
				Array<Grid<uint8>> ps(8, Grid<uint8>(N,N));

				ps[0] = pattern([&](int32 dx, int32 dy) -> uint8 {return sample[(y + dy) % bitmap.height()][(x + dx) % bitmap.width()]; }, N);
				ps[1] = GridHelper::mirror(ps[0]);
				ps[2] = GridHelper::rotate270(ps[0]);
				ps[3] = GridHelper::mirror(ps[2]);
				ps[4] = GridHelper::rotate270(ps[2]);
				ps[5] = GridHelper::mirror(ps[4]);
				ps[6] = GridHelper::rotate270(ps[4]);
				ps[7] = GridHelper::mirror(ps[6]);

				for (int32 k = 0; k < symmetry; k++) {
					const auto& p = ps[k];
					const auto h = hash(p, C);

					// patternIndicesのキーとしてhが存在するか確認
					auto it = patternIndices.find(h);
					if (it != patternIndices.end()) {

						// キーが存在する場合
						weightList[it->second]+= 1.0;
					}
					else {

						// キーが存在しない場合
						patternIndices[h] = weightList.size();
						weightList << 1.0;
						patterns << p;
					}
				}
			}
		}

		weights = weightList;
		T = weights.size();
		this->ground = ground;

		static auto agrees = [](const Grid<uint8>& p1, const Grid<uint8>& p2, const Point& dxy, int32 N) {
			const int32 xmin = dxy.x < 0 ? 0 : dxy.x, xmax = dxy.x < 0 ? dxy.x + N : N;
			const int32 ymin = dxy.y < 0 ? 0 : dxy.y, ymax = dxy.y < 0 ? dxy.y + N : N;
			for (int32 y = ymin; y < ymax; y++) {
				for (int32 x = xmin; x < xmax; x++) {
					if (p1[y][x] != p2[y - dxy.y][x - dxy.x]) {
						return false;
					}
				}
			}
			return true;
		};

		propagator.resize(4);
		for (int32 d = 0; d < 4; d++) {
			propagator[d].resize(T);
			for (int32 t = 0; t < T; t++) {
				Array<int32> list;
				for (int32 t2 = 0; t2 < T; t2++)
					if (agrees(patterns[t], patterns[t2], dxy[d], N))
						list.push_back(t2);
				propagator[d][t] = list;
			}
		}
	}

	Image ToImage() const
	{
		Grid<Color> bitmap(gridSize);

		if (observed[0][0] >= 0) {
			for (int32 y = 0; y < gridSize.y; y++) {
				int32 dy = y < gridSize.y - N + 1 ? 0 : N - 1;

				for (int32 x = 0; x < gridSize.x; x++) {
					int32 dx = x < gridSize.x - N + 1 ? 0 : N - 1;
					bitmap[y][x] = colors[patterns[observed[y - dy][x - dx]][dy][dx]];
				}
			}
		}
		else {
			for (auto i : step(wave.height())) {
				for (auto d : step(wave.width())) {

					int32 contributors = 0;

					int32 r{ 0 };
					int32 g{ 0 };
					int32 b{ 0 };

					for (int32 dy = 0; dy < N; dy++) {
						for (int32 dx = 0; dx < N; dx++) {
							auto sxy = Point{ d, i } - Point{ dx ,dy};

							if (sxy.x < 0)
								sxy.x += gridSize.x;

							if (sxy.y < 0)
								sxy.y += gridSize.y;

							if (!periodic && (sxy.x + N > gridSize.x || sxy.y + N > gridSize.y || sxy.x < 0 || sxy.y < 0)) {
								continue;
							}

							for (int32 t = 0; t < T; t++) {
								if (wave[sxy][t]) {
									contributors++;
									const auto& argb = colors[patterns[t][dy][dx]];
									r += argb.r;
									g += argb.g;
									b += argb.b;
								}
							}
						}
					}
					bitmap[i][d] = Color(
						static_cast<uint8>(r / contributors),
						static_cast<uint8>(g / contributors),
						static_cast<uint8>(b / contributors)
					);
				}
			}
		}

		return BitmapHelper::ToImage(bitmap);
	}
	Size ImageSize() const
	{
		return gridSize;
	}
};

