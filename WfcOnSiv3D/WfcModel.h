﻿#pragma once
#include "RandomHelper.h"

class WfcModel {

public:
	enum Heuristic { Entropy, MRV, Scanline };


protected:
	Array<Array<bool>> wave;

	Array<Array<Array<int32>>> propagator;
	Array<Array<Array<int32>>> compatible;
	Array<int32> observed;

	Array<std::pair<int32, int32>> stack;
	int32 stacksize = 0;
	int32 observedSoFar = 0;

	Size gridSize{0,0};

	int32 T = 0;
	int32 N = 0;
	bool periodic = false;
	bool ground = false;

	Array<double> weights;
	Array<double> weightLogWeights, distribution;

	Array<int32> sumsOfOnes;
	double sumOfWeights = 0;
	double sumOfWeightLogWeights = 0;
	double startingEntropy = 0;
	Array<double> sumsOfWeights;
	Array<double> sumsOfWeightLogWeights;
	Array<double> entropies;

	Heuristic heuristic;

	const Array<int32> dx{ -1, 0, 1, 0 };
	const Array<int32> dy{ 0, 1, 0, -1 };
	const Array<int32> opposite{ 2, 3, 0, 1 };


	WfcModel(const Size& gridSize, int32 N, bool periodic, Heuristic heuristic)
		: gridSize(gridSize), N(N), periodic(periodic), heuristic(heuristic) {}

public:
	void Init() {
		wave.resize(gridSize.x * gridSize.y, Array<bool>(T));
		compatible.resize(wave.size(), Array<Array<int32>>(T, Array<int32>(4)));

		for (int32 i = 0; i < wave.size(); i++) {
			for (int32 t = 0; t < T; t++) {
				compatible[i][t].resize(4);
			}
		}

		distribution.resize(T);
		observed.resize(gridSize.x * gridSize.y);

		weightLogWeights.resize(T);
		sumOfWeights = 0;
		sumOfWeightLogWeights = 0;

		for (int32 t = 0; t < T; t++) {
			weightLogWeights[t] = weights[t] * log(weights[t]);
			sumOfWeights += weights[t];
			sumOfWeightLogWeights += weightLogWeights[t];
		}

		startingEntropy = log(sumOfWeights) - sumOfWeightLogWeights / sumOfWeights;

		sumsOfOnes.resize(gridSize.x * gridSize.y);
		sumsOfWeights.resize(gridSize.x * gridSize.y);
		sumsOfWeightLogWeights.resize(gridSize.x * gridSize.y);
		entropies.resize(gridSize.x * gridSize.y);

		stack.resize(wave.size() * T, std::make_pair<int32, int32>(0, 0));
		stacksize = 0;
	}

	void Clear() {
		for (int32 i = 0; i < wave.size(); i++) {
			for (int32 t = 0; t < T; t++) {
				wave[i][t] = true;
				for (int32 d = 0; d < 4; d++)
					compatible[i][t][d] = propagator[opposite[d]][t].size();
			}

			sumsOfOnes[i] = weights.size();
			sumsOfWeights[i] = sumOfWeights;
			sumsOfWeightLogWeights[i] = sumOfWeightLogWeights;
			entropies[i] = startingEntropy;
			observed[i] = -1;
		}
		observedSoFar = 0;

		if (ground) {
			for (int32 x = 0; x < gridSize.x; x++) {
				for (int32 t = 0; t < T - 1; t++) {
					Ban(x + (gridSize.y - 1) * gridSize.x, t);
				}
				for (int32 y = 0; y < gridSize.y - 1; y++) {
					Ban(x + y * gridSize.x, T - 1);
				}
			}
			Propagate();
		}
	}

	bool Run(int32 seed, int32 limit) {
		if (wave.empty()) {
			Init();
		}

		Clear();
		Reseed(seed);

		for (int32 l = 0; l < limit || limit < 0; l++) {
			int32 node = NextUnobservedNode();
			if (node >= 0) {
				Observe(node);
				bool success = Propagate();
				if (!success){
					return false;
				}
			}
			else {
				for (int32 i = 0; i < wave.size(); i++) {
					for (int32 t = 0; t < T; t++) {
						if (wave[i][t]) {
							observed[i] = t;
							break;
						}
					}
				}
				return true;
			}
		}

		return true;
	}

	void RunOneStep() {

		if (wave.empty()) {
			Init();
			Clear();
		}

		int32 node = NextUnobservedNode();
		if (node >= 0) {
			Observe(node);
			bool success = Propagate();
			if (!success) {
				return;
			}
		}
		else {
			for (int32 i = 0; i < wave.size(); i++) {
				for (int32 t = 0; t < T; t++) {
					if (wave[i][t]) {
						observed[i] = t;
						break;
					}
				}
			}
			return;
		}
	}

	bool HasCompleted() {
		return not wave.isEmpty() && sumsOfOnes.sum() == sumsOfOnes.size();
	}

	int32 NextUnobservedNode() {
		if (heuristic == Heuristic::Scanline) {
			for (int32 i = observedSoFar; i < wave.size(); i++) {

				if (!periodic && (i % gridSize.x + N > gridSize.x || i / gridSize.x + N > gridSize.y))
					continue;

				if (sumsOfOnes[i] > 1) {
					observedSoFar = i + 1;
					return i;
				}
			}
			return -1;
		}

		double min = 1E+4;
		int32 argmin = -1;
		for (int32 i = 0; i < wave.size(); i++) {
			if (!periodic && (i % gridSize.x + N > gridSize.x || i / gridSize.x + N > gridSize.y))
				continue;

			int32 remainingValues = sumsOfOnes[i];
			double entropy = heuristic == Heuristic::Entropy ? entropies[i] : remainingValues;

			if (remainingValues > 1 && entropy <= min) {
				double noise = 1E-6 * Random<double>(0, 1.0);
				if (entropy + noise < min) {
					min = entropy + noise;
					argmin = i;
				}
			}
		}
		return argmin;
	}
private:

	bool next() {
		int32 node = NextUnobservedNode();
		if (node >= 0) {
			Observe(node);
			bool success = Propagate();
			if (!success) {
				return false;
			}
		}
		else {
			for (int32 i = 0; i < wave.size(); i++) {
				for (int32 t = 0; t < T; t++) {
					if (wave[i][t]) {
						observed[i] = t;
						break;
					}
				}
			}
			return true;
		}
	}

	void Observe(int32 node) {
		const Array<bool>& w = wave[node];

		for (int32 t = 0; t < T; t++)
			distribution[t] = w[t] ? weights[t] : 0.0;

		int32 r = RandomHelper::Random(distribution, Random<double>(0, 1.0));

		for (int32 t = 0; t < T; t++) {
			if (w[t] != (t == r)) {
				Ban(node, t);
			}
		}
	}

	bool Propagate() {
		while (stacksize > 0) {
			std::pair<int32, int32> current = stack[stacksize - 1];
			stacksize--;

			int32 x1 = current.first % gridSize.x;
			int32 y1 = current.first / gridSize.x;
			int32 t1 = current.second;

			for (int32 d = 0; d < 4; d++) {
				int32 x2 = x1 + dx[d];
				int32 y2 = y1 + dy[d];

				if (!periodic && (x2 < 0 || y2 < 0 || x2 + N > gridSize.x || y2 + N > gridSize.y))
					continue;

				if (x2 < 0)
					x2 += gridSize.x;
				else if (x2 >= gridSize.x)
					x2 -= gridSize.x;

				if (y2 < 0)
					y2 += gridSize.y;
				else if (y2 >= gridSize.y)
					y2 -= gridSize.y;

				int32 i2 = x2 + y2 * gridSize.x;
				Array<int32>& p = propagator[d][t1];
				Array<Array<int32>>& compat = compatible[i2];

				for (int32 l = 0; l < p.size(); l++) {
					int32 t2 = p[l];
					Array<int32>& comp = compat[t2];

					comp[d]--;
					if (comp[d] == 0) Ban(i2, t2);
				}
			}
		}

		return sumsOfOnes[0] > 0;
	}

	void Ban(int32 i, int32 t) {
		wave[i][t] = false;

		Array<int32>& comp = compatible[i][t];
		for (int32 d = 0; d < 4; d++) comp[d] = 0;
		stack[stacksize] = std::make_pair(i, t);
		stacksize++;

		sumsOfOnes[i] -= 1;
		sumsOfWeights[i] -= weights[t];
		sumsOfWeightLogWeights[i] -= weightLogWeights[t];

		double sum = sumsOfWeights[i];
		entropies[i] = Math::Log(sum) - sumsOfWeightLogWeights[i] / sum;
	}

};
