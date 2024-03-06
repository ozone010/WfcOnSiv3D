# include "stdafx.h"
# include "WfcModel.hpp"

WfcModel::WfcModel(const Size& gridSize, int32 N, bool periodic, Heuristic heuristic):
	m_gridSize(gridSize), m_N(N), m_periodic(periodic), m_heuristic(heuristic),
	m_observed(gridSize), m_sumsOfOnes(gridSize), m_sumsOfWeights(gridSize), m_sumsOfWeightLogWeights(gridSize), m_entropies(gridSize) {}

void WfcModel::init()
{
	m_wave.resize(m_gridSize, Array<bool>(m_T));
	m_compatible.resize(m_wave.size(), Array<Array<int32>>(m_T, Array<int32>(4)));
	m_distribution.resize(m_T);

	m_weightLogWeights.resize(m_T);
	m_sumOfWeights = 0;
	m_sumOfWeightLogWeights = 0;

	for (int32 t = 0; t < m_T; t++) {
		m_weightLogWeights[t] = m_weights[t] * Math::Log(m_weights[t]);
		m_sumOfWeights += m_weights[t];
		m_sumOfWeightLogWeights += m_weightLogWeights[t];
	}

	m_startingEntropy = Math::Log(m_sumOfWeights) - m_sumOfWeightLogWeights / m_sumOfWeights;

	m_stack.resize(m_wave.num_elements() * m_T, std::make_pair<Point, int32>({ 0,0 }, 0));
	m_stacksize = 0;
}

void WfcModel::clear() {
	for (auto y : step(m_wave.height())) {
		for (auto x : step(m_wave.width())) {
			for (int32 t = 0; t < m_T; t++) {
				m_wave[y][x][t] = true;
				for (int32 d = 0; d < 4; d++) {
					m_compatible[y][x][t][d] = m_propagator[opposite[d]][t].size();
				}
			}

			m_sumsOfOnes[y][x] = m_weights.size();
			m_sumsOfWeights[y][x] = m_sumOfWeights;
			m_sumsOfWeightLogWeights[y][x] = m_sumOfWeightLogWeights;
			m_entropies[y][x] = m_startingEntropy;
			m_observed[y][x] = -1;
		}
	}
	m_observedSoFar = 0;

	if (m_ground) {
		for (int32 x = 0; x < m_gridSize.x; x++) {
			for (int32 t = 0; t < m_T - 1; t++) {
				ban({ x , m_gridSize.y - 1 }, t);
			}
			for (int32 y = 0; y < m_gridSize.y - 1; y++) {
				ban({ x , y }, m_T - 1);
			}
		}
		propagate();
	}
}

bool WfcModel::run(int32 seed, int32 limit) {
	if (m_wave.empty()) {
		init();
	}

	clear();
	Reseed(seed);

	for (auto l = 0; l < limit || limit < 0; l++) {
		auto node = nextUnm_observedNode();
		if (node.x >= 0) {
			observe(node);
			bool success = propagate();
			if (!success) {
				return false;
			}
		}
		else {
			for (auto y : step(m_wave.height())) {
				for (auto x : step(m_wave.width())) {
					for (int32 t = 0; t < m_T; t++) {
						if (m_wave[y][x][t]) {
							m_observed[y][x] = t;
							break;
						}
					}
				}
			}
			return true;
		}
	}

	return true;
}

void WfcModel::runOneStep() {

	if (m_wave.empty()) {
		init();
		clear();
	}

	const auto node = nextUnm_observedNode();
	if (node.x >= 0) {
		observe(node);
		bool success = propagate();
		if (!success) {
			return;
		}
	}
	else {
		for (auto y : step(m_wave.height())) {
			for (auto x : step(m_wave.width())) {
				for (int32 t = 0; t < m_T; t++) {
					if (m_wave[y][x][t] == true) {
						m_observed[y][x] = t;
						break;
					}
				}
			}
		}
		return;
	}
}

bool WfcModel::hasCompleted() const {
	return not m_wave.isEmpty() && m_sumsOfOnes.asArray().sum() == m_sumsOfOnes.num_elements();
}

Point WfcModel::nextUnm_observedNode() {
	if (m_heuristic == Heuristic::Scanline) {
		for (auto y : step(m_wave.height())) {
			for (auto x : step(m_wave.width())) {

				if (!m_periodic && (y % m_gridSize.x + m_N > m_gridSize.x || y / m_gridSize.x + m_N > m_gridSize.y))
					continue;

				if (m_sumsOfOnes[y][x] > 1) {
					m_observedSoFar = y + 1;
					return { x , y };;
				}
			}
		}
		return { -1, -1 };
	}

	double min = 1E+4;
	Point argmin{ -1, -1 };
	for (auto y : step(m_wave.height())) {
		for (auto x : step(m_wave.width())) {
			if (!m_periodic && (x % m_gridSize.x + m_N > m_gridSize.x || x / m_gridSize.x + m_N > m_gridSize.y))
				continue;

			int32 remainingValues = m_sumsOfOnes[y][x];
			double entropy = m_heuristic == Heuristic::Entropy ? m_entropies[y][x] : remainingValues;

			if (remainingValues > 1 && entropy <= min) {
				double noise = 1E-6 * Random<double>(0, 1.0);
				if (entropy + noise < min) {
					min = entropy + noise;
					argmin = { x , y };
				}
			}
		}
	}
	return argmin;
}

void WfcModel::observe(const Point& node) {
	const Array<bool>& w = m_wave[node.y][node.x];

	for (auto t = 0; t < m_T; ++t)
		m_distribution[t] = w[t] ? m_weights[t] : 0.0;

	auto r = RandomHelper::Random(m_distribution, Random<double>(0, 1.0));

	for (auto t = 0; t < m_T; ++t) {
		if (w[t] != (t == r)) {
			ban(node, t);
		}
	}
}

bool WfcModel::propagate() {
	while (m_stacksize > 0) {
		auto current = m_stack[m_stacksize - 1];
		--m_stacksize;

		auto xy1 = current.first;
		int32 t1 = current.second;

		for (auto d = 0; d < 4; ++d) {
			auto xy2 = xy1 + dxy[d];

			if (!m_periodic && (xy2.x < 0 || xy2.y < 0 || xy2.x + m_N > m_gridSize.x || xy2.y + m_N > m_gridSize.y))
				continue;

			if (xy2.x < 0)
				xy2.x += m_gridSize.x;
			else if (xy2.x >= m_gridSize.x)
				xy2.x -= m_gridSize.x;

			if (xy2.y < 0)
				xy2.y += m_gridSize.y;
			else if (xy2.y >= m_gridSize.y)
				xy2.y -= m_gridSize.y;

			Array<int32>& p = m_propagator[d][t1];
			Array<Array<int32>>& compat = m_compatible[xy2];

			for (auto l = 0; l < p.size(); l++) {
				int32 t2 = p[l];
				Array<int32>& comp = compat[t2];

				comp[d]--;
				if (comp[d] == 0) {
					ban(xy2, t2);
				}
			}
		}
	}

	return m_sumsOfOnes[0][0] > 0;
}

void WfcModel::ban(const Point& p, int32 t) {
	m_wave[p][t] = false;

	Array<int32>& comp = m_compatible[p][t];
	for (int32 d = 0; d < 4; ++d) {
		comp[d] = 0;
	}

	m_stack[m_stacksize++] = std::make_pair(p, t);

	m_sumsOfOnes[p] -= 1;
	m_sumsOfWeights[p] -= m_weights[t];
	m_sumsOfWeightLogWeights[p] -= m_weightLogWeights[t];

	double sum = m_sumsOfWeights[p];
	m_entropies[p] = Math::Log(sum) - m_sumsOfWeightLogWeights[p] / sum;
}
