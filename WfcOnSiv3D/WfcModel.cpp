#include "stdafx.h"
#include "WfcModel.hpp"

WfcModel::WfcModel(const Size& gridSize, int32 N, bool periodic, Heuristic heuristic):
	gridSize(gridSize), N(N), periodic(periodic), heuristic(heuristic),
	observed(gridSize), sumsOfOnes(gridSize), sumsOfWeights(gridSize), sumsOfWeightLogWeights(gridSize), entropies(gridSize) {}

void WfcModel::Init()
{
	wave.resize(gridSize, Array<bool>(T));
	compatible.resize(wave.size(), Array<Array<int32>>(T, Array<int32>(4)));
	distribution.resize(T);

	weightLogWeights.resize(T);
	sumOfWeights = 0;
	sumOfWeightLogWeights = 0;

	for (int32 t = 0; t < T; t++) {
		weightLogWeights[t] = weights[t] * Math::Log(weights[t]);
		sumOfWeights += weights[t];
		sumOfWeightLogWeights += weightLogWeights[t];
	}

	startingEntropy = Math::Log(sumOfWeights) - sumOfWeightLogWeights / sumOfWeights;

	stack.resize(wave.num_elements() * T, std::make_pair<Point, int32>({ 0,0 }, 0));
	stacksize = 0;
}

void WfcModel::Clear() {
	for (auto y : step(wave.height())) {
		for (auto x : step(wave.width())) {
			for (int32 t = 0; t < T; t++) {
				wave[y][x][t] = true;
				for (int32 d = 0; d < 4; d++) {
					compatible[y][x][t][d] = propagator[opposite[d]][t].size();
				}
			}

			sumsOfOnes[y][x] = weights.size();
			sumsOfWeights[y][x] = sumOfWeights;
			sumsOfWeightLogWeights[y][x] = sumOfWeightLogWeights;
			entropies[y][x] = startingEntropy;
			observed[y][x] = -1;
		}
	}
	observedSoFar = 0;

	if (ground) {
		for (int32 x = 0; x < gridSize.x; x++) {
			for (int32 t = 0; t < T - 1; t++) {
				Ban({ x , gridSize.y - 1 }, t);
			}
			for (int32 y = 0; y < gridSize.y - 1; y++) {
				Ban({ x , y }, T - 1);
			}
		}
		Propagate();
	}
}

bool WfcModel::Run(int32 seed, int32 limit) {
	if (wave.empty()) {
		Init();
	}

	Clear();
	Reseed(seed);

	for (int32 l = 0; l < limit || limit < 0; l++) {
		auto node = NextUnobservedNode();
		if (node.x >= 0) {
			Observe(node);
			bool success = Propagate();
			if (!success) {
				return false;
			}
		}
		else {
			for (auto y : step(wave.height())) {
				for (auto x : step(wave.width())) {
					for (int32 t = 0; t < T; t++) {
						if (wave[y][x][t]) {
							observed[y][x] = t;
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

void WfcModel::RunOneStep() {

	if (wave.empty()) {
		Init();
		Clear();
	}

	auto node = NextUnobservedNode();
	if (node.x >= 0) {
		Observe(node);
		bool success = Propagate();
		if (!success) {
			return;
		}
	}
	else {
		for (auto y : step(wave.height())) {
			for (auto x : step(wave.width())) {
				for (int32 t = 0; t < T; t++) {
					if (wave[y][x][t] == true) {
						observed[y][x] = t;
						break;
					}
				}
			}
		}
		return;
	}
}

bool WfcModel::HasCompleted() {
	return not wave.isEmpty() && sumsOfOnes.asArray().sum() == sumsOfOnes.num_elements();
}

Point WfcModel::NextUnobservedNode() {
	if (heuristic == Heuristic::Scanline) {
		for (auto y : step(wave.height())) {
			for (auto x : step(wave.width())) {

				if (!periodic && (y % gridSize.x + N > gridSize.x || y / gridSize.x + N > gridSize.y))
					continue;

				if (sumsOfOnes[y][x] > 1) {
					observedSoFar = y + 1;
					return { x , y };;
				}
			}
		}
		return { -1, -1 };
	}

	double min = 1E+4;
	Point argmin{ -1, -1 };
	for (auto y : step(wave.height())) {
		for (auto x : step(wave.width())) {
			if (!periodic && (x % gridSize.x + N > gridSize.x || x / gridSize.x + N > gridSize.y))
				continue;

			int32 remainingValues = sumsOfOnes[y][x];
			double entropy = heuristic == Heuristic::Entropy ? entropies[y][x] : remainingValues;

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

void WfcModel::Observe(const Point& node) {
	const Array<bool>& w = wave[node.y][node.x];

	for (int32 t = 0; t < T; ++t)
		distribution[t] = w[t] ? weights[t] : 0.0;

	int32 r = RandomHelper::Random(distribution, Random<double>(0, 1.0));

	for (int32 t = 0; t < T; ++t) {
		if (w[t] != (t == r)) {
			Ban(node, t);
		}
	}
}

bool WfcModel::Propagate() {
	while (stacksize > 0) {
		auto current = stack[stacksize - 1];
		stacksize--;

		auto xy1 = current.first;
		int32 t1 = current.second;

		for (int32 d = 0; d < 4; ++d) {
			auto xy2 = xy1 + dxy[d];


			if (!periodic && (xy2.x < 0 || xy2.y < 0 || xy2.x + N > gridSize.x || xy2.y + N > gridSize.y))
				continue;

			if (xy2.x < 0)
				xy2.x += gridSize.x;
			else if (xy2.x >= gridSize.x)
				xy2.x -= gridSize.x;

			if (xy2.y < 0)
				xy2.y += gridSize.y;
			else if (xy2.y >= gridSize.y)
				xy2.y -= gridSize.y;

			Array<int32>& p = propagator[d][t1];
			Array<Array<int32>>& compat = compatible[xy2];

			for (int32 l = 0; l < p.size(); l++) {
				int32 t2 = p[l];
				Array<int32>& comp = compat[t2];

				comp[d]--;
				if (comp[d] == 0) {
					Ban(xy2, t2);
				}
			}
		}
	}

	return sumsOfOnes[0][0] > 0;
}

void WfcModel::Ban(const Point& p, int32 t) {
	wave[p][t] = false;

	Array<int32>& comp = compatible[p][t];
	for (int32 d = 0; d < 4; ++d) {
		comp[d] = 0;
	}

	stack[stacksize++] = std::make_pair(p, t);

	sumsOfOnes[p] -= 1;
	sumsOfWeights[p] -= weights[t];
	sumsOfWeightLogWeights[p] -= weightLogWeights[t];

	double sum = sumsOfWeights[p];
	entropies[p] = Math::Log(sum) - sumsOfWeightLogWeights[p] / sum;
}
