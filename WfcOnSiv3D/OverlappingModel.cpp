# include "stdafx.h"
# include "OverlappingModel.hpp"

OverlappingModel::OverlappingModel(const String& name, int32 N, const Size& m_gridSize, bool periodicInput, bool periodic, int32 symmetry, bool ground, Heuristic heuristic)
	: WfcModel(m_gridSize, N, periodic, heuristic) {
	auto bitmap = BitmapHelper::LoadBitmap(name);

	Grid<uint8> sample(bitmap.size());

	m_colors.clear();

	for (auto y : step(sample.height())) {
		for (auto x : step(sample.width())) {

			const Color& color = bitmap[y][x];
			int32 k = 0;

			for (; k < m_colors.size(); k++) {
				if (m_colors[k] == color) {
					break;
				}
			}
			if (k == m_colors.size()) {
				m_colors << color;
			}
			sample[y][x] = static_cast<uint8>(k);
		}
	}

	static auto pattern = [](const std::function<uint8(int32, int32)>& f, int32 N) {
		Grid<uint8> result(N, N);
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

	m_patterns.clear();
	HashTable<int64, int32> patternIndices;
	Array<double> weightList;

	const int32 C = m_colors.size();
	const int32 xmax = periodicInput ? bitmap.width() : bitmap.width() - N + 1;
	const int32 ymax = periodicInput ? bitmap.height() : bitmap.height() - N + 1;
	for (auto y = 0; y < ymax; y++) {
		for (auto x = 0; x < xmax; x++) {
			Array<Grid<uint8>> ps(8, Grid<uint8>(N, N));

			ps[0] = pattern([&](int32 dx, int32 dy) -> uint8 {return sample[(y + dy) % bitmap.height()][(x + dx) % bitmap.width()]; }, N);
			ps[1] = GridHelper::mirrored(ps[0]);
			ps[2] = GridHelper::rotated270(ps[0]);
			ps[3] = GridHelper::mirrored(ps[2]);
			ps[4] = GridHelper::rotated270(ps[2]);
			ps[5] = GridHelper::mirrored(ps[4]);
			ps[6] = GridHelper::rotated270(ps[4]);
			ps[7] = GridHelper::mirrored(ps[6]);

			for (int32 k = 0; k < symmetry; k++) {
				const auto& p = ps[k];
				const auto h = hash(p, C);

				// patternIndicesのキーとしてhが存在するか確認
				auto it = patternIndices.find(h);
				if (it != patternIndices.end()) {

					// キーが存在する場合
					weightList[it->second] += 1.0;
				}
				else {

					// キーが存在しない場合
					patternIndices[h] = weightList.size();
					weightList << 1.0;
					m_patterns << p;
				}
			}
		}
	}

	m_weights = weightList;
	m_T = m_weights.size();
	this->m_ground = ground;

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

	m_propagator.resize(4);
	for (int32 d = 0; d < 4; d++) {
		m_propagator[d].resize(m_T);
		for (int32 t = 0; t < m_T; t++) {
			Array<int32> list;
			for (int32 t2 = 0; t2 < m_T; t2++)
				if (agrees(m_patterns[t], m_patterns[t2], dxy[d], N))
					list.push_back(t2);
			m_propagator[d][t] = list;
		}
	}
}

Image OverlappingModel::toImage() const
{
	Grid<Color> bitmap(m_gridSize);

	if (m_observed[0][0] >= 0) {
		for (int32 y = 0; y < m_gridSize.y; y++) {
			int32 dy = y < m_gridSize.y - m_N + 1 ? 0 : m_N - 1;

			for (int32 x = 0; x < m_gridSize.x; x++) {
				int32 dx = x < m_gridSize.x - m_N + 1 ? 0 : m_N - 1;
				bitmap[y][x] = m_colors[m_patterns[m_observed[y - dy][x - dx]][dy][dx]];
			}
		}
	}
	else {
		for (auto y : step(m_wave.height())) {
			for (auto x : step(m_wave.width())) {

				int32 contributors = 0;

				int32 r{ 0 };
				int32 g{ 0 };
				int32 b{ 0 };

				for (int32 dy = 0; dy < m_N; dy++) {
					for (int32 dx = 0; dx < m_N; dx++) {
						auto sxy = Point{ x, y } - Point{ dx ,dy };

						if (sxy.x < 0)
							sxy.x += m_gridSize.x;

						if (sxy.y < 0)
							sxy.y += m_gridSize.y;

						if (!m_periodic && (sxy.x + m_N > m_gridSize.x || sxy.y + m_N > m_gridSize.y || sxy.x < 0 || sxy.y < 0)) {
							continue;
						}

						for (int32 t = 0; t < m_T; ++t) {
							if (m_wave[sxy][t]) {
								contributors++;
								const auto& argb = m_colors[m_patterns[t][dy][dx]];
								r += argb.r;
								g += argb.g;
								b += argb.b;
							}
						}
					}
				}
				bitmap[y][x] = Color(
					static_cast<uint8>(r / contributors),
					static_cast<uint8>(g / contributors),
					static_cast<uint8>(b / contributors)
				);
			}
		}
	}

	return BitmapHelper::ToImage(bitmap);
}
