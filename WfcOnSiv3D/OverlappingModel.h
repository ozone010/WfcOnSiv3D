#pragma once
#include "WfcModel.h"
#include "BitmapHelper.h"

class OverlappingModel : public WfcModel {
public:
	Array<Array<uint8>> patterns;
	Array<int32> colors;

	OverlappingModel(const String& name, int32 N, const Size& gridSize, bool periodicInput, bool periodic, int32 symmetry, bool ground, Heuristic heuristic)
		: WfcModel(gridSize, N, periodic, heuristic) {
		auto [bitmap, SX, SY] = BitmapHelper::LoadBitmap(name);
		Array<uint8> sample(bitmap.begin(), bitmap.end());

		colors.clear();
		for (int32 i = 0; i < sample.size(); i++) {
			int32 color = bitmap[i];
			int32 k = 0;

			for (; k < colors.size(); k++) {
				if (colors[k] == color) {
					break;
				}
			}
			if (k == colors.size()) {
				colors << color;
			}
			sample[i] = static_cast<uint8>(k);
		}

		static auto pattern = [](const std::function<uint8(int32, int32)>& f, int32 N) {
			Array<uint8> result(N * N);
			for (int32 y = 0; y < N; y++) {
				for (int32 x = 0; x < N; x++) {
					result[x + y * N] = f(x, y);
				}
			}
			return result;
		};

		static auto rotate = [](const Array<uint8>& p, int32 N) {
			return pattern([p, N](int32 x, int32 y) { return p[N - 1 - y + x * N]; }, N);
		};

		static auto reflect = [](const Array<uint8>& p, int32 N) {
			return pattern([p, N](int32 x, int32 y) { return p[N - 1 - x + y * N]; }, N);
		};

		static auto hash = [](const Array<uint8>& p, int32 C) {
			int64 result = 0, power = 1;
			for (int32 i = 0; i < p.size(); i++) {
				result += p[p.size() - 1 - i] * power;
				power *= C;
			}
			return result;
		};

		patterns.clear();
		HashTable<int64, int32> patternIndices;
		Array<double> weightList;

		int32 C = colors.size();
		int32 xmax = periodicInput ? SX : SX - N + 1;
		int32 ymax = periodicInput ? SY : SY - N + 1;
		for (int32 y = 0; y < ymax; y++) {
			for (int32 x = 0; x < xmax; x++) {
				Array<Array<uint8>> ps(8);

				const auto& test = pattern([&](int32 dx, int32 dy) -> uint8 {return sample[(x + dx) % SX + (y + dy) % SY * SX]; }, N);

				ps[0] = pattern([&](int32 dx, int32 dy) -> uint8 {return sample[(x + dx) % SX + (y + dy) % SY * SX]; }, N);
				ps[1] = reflect(ps[0], N);
				ps[2] = rotate(ps[0], N);
				ps[3] = reflect(ps[2], N);
				ps[4] = rotate(ps[2], N);
				ps[5] = reflect(ps[4], N);
				ps[6] = rotate(ps[4], N);
				ps[7] = reflect(ps[6], N);

				for (int32 k = 0; k < symmetry; k++) {
					auto& p = ps[k];
					auto h = hash(p, C);

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

		static auto agrees = [](const Array<uint8>& p1, const Array<uint8>& p2, int32 dx, int32 dy, int32 N) {
			int32 xmin = dx < 0 ? 0 : dx, xmax = dx < 0 ? dx + N : N, ymin = dy < 0 ? 0 : dy, ymax = dy < 0 ? dy + N : N;
			for (int32 y = ymin; y < ymax; y++) {
				for (int32 x = xmin; x < xmax; x++) {
					if (p1[x + N * y] != p2[x - dx + N * (y - dy)]) {
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
					if (agrees(patterns[t], patterns[t2], dx[d], dy[d], N))
						list.push_back(t2);
				propagator[d][t] = list;
			}
		}
	}

	Image ToImage()
	{
		Grid<int32> bitmap(gridSize, 0);

		if (observed[0] >= 0) {
			for (int32 y = 0; y < gridSize.y; y++) {
				int32 dy = y < gridSize.y - N + 1 ? 0 : N - 1;
				for (int32 x = 0; x < gridSize.x; x++) {
					int32 dx = x < gridSize.x - N + 1 ? 0 : N - 1;

					bitmap[y][x] = colors[patterns[observed[x - dx + (y - dy) * gridSize.x]][dx + dy * N]];
				}
			}
		}
		else {
			for (int32 i = 0; i < wave.size(); i++) {
				int32 contributors = 0, r = 0, g = 0, b = 0;
				int32 x = i % gridSize.x;
				int32 y = i / gridSize.x;

				for (int32 dy = 0; dy < N; dy++) {
					for (int32 dx = 0; dx < N; dx++) {
						int32 sx = x - dx;
						if (sx < 0) sx += gridSize.x;

						int32 sy = y - dy;
						if (sy < 0) sy += gridSize.y;

						int32 s = sx + sy * gridSize.x;

						if (!periodic && (sx + N > gridSize.x || sy + N > gridSize.y || sx < 0 || sy < 0)) {
							continue;
						}

						for (int32 t = 0; t < T; t++) {
							if (wave[s][t]) {
								contributors++;
								int32 argb = colors[patterns[t][dx + dy * N]];
								r += (argb & 0xff0000) >> 16;
								g += (argb & 0xff00) >> 8;
								b += argb & 0xff;
							}
						}
					}
				}
				bitmap[y][x] = 0xff000000 | ((r / contributors) << 16) | ((g / contributors) << 8) | b / contributors;
			}
		}

		return BitmapHelper::ToImage(bitmap);
	}
};

