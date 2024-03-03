#pragma once
#include "RandomHelper.h"

class WfcModel {

public:
	enum Heuristic { Entropy, MRV, Scanline };


protected:
	Grid<Array<bool>> wave;

	Array<Array<Array<int32>>> propagator;
	Grid<Array<Array<int32>>> compatible;
	Grid<int32> observed;

	Array<std::pair<Point, int32>> stack;
	int32 stacksize = 0;
	int32 observedSoFar = 0;

	Size gridSize{0,0};

	int32 T = 0;
	int32 N = 0;
	bool periodic = false;
	bool ground = false;

	Array<double> weights;
	Array<double> weightLogWeights;
	Array<double> distribution;

	Grid<int32> sumsOfOnes;
	double sumOfWeights = 0;
	double sumOfWeightLogWeights = 0;
	double startingEntropy = 0;
	Grid<double> sumsOfWeights;
	Grid<double> sumsOfWeightLogWeights;
	Grid<double> entropies;

	Heuristic heuristic;

	const Array<Point> dxy{ { -1, 0}, { 0, 1} , {1, 0},{ 0,-1} };
	const Array<int32> opposite{ 2, 3, 0, 1 };


	WfcModel(const Size& gridSize, int32 N, bool periodic, Heuristic heuristic)
		: gridSize(gridSize), N(N), periodic(periodic), heuristic(heuristic) {}

public:
	void Init() {
		wave.resize(gridSize, Array<bool>(T));
		compatible.resize(wave.size(), Array<Array<int32>>(T, Array<int32>(4)));
		distribution.resize(T);
		observed.resize(gridSize);

		weightLogWeights.resize(T);
		sumOfWeights = 0;
		sumOfWeightLogWeights = 0;

		for (int32 t = 0; t < T; t++) {
			weightLogWeights[t] = weights[t] * log(weights[t]);
			sumOfWeights += weights[t];
			sumOfWeightLogWeights += weightLogWeights[t];
		}

		startingEntropy = Math::Log(sumOfWeights) - sumOfWeightLogWeights / sumOfWeights;

		sumsOfOnes.resize(gridSize);
		sumsOfWeights.resize(gridSize);
		sumsOfWeightLogWeights.resize(gridSize);
		entropies.resize(gridSize);

		stack.resize(wave.num_elements() * T, std::make_pair<Point, int32>({0,0}, 0));
		stacksize = 0;
	}

	void Clear() {
		for (auto i : step(wave.height())){
			for (auto x : step(wave.width())){
				for (int32 t = 0; t < T; t++) {
					wave[i][x][t] = true;
					for (int32 d = 0; d < 4; d++) {
						compatible[i][x][t][d] = propagator[opposite[d]][t].size();
					}
				}

				sumsOfOnes[i][x] = weights.size();
				sumsOfWeights[i][x] = sumOfWeights;
				sumsOfWeightLogWeights[i][x] = sumOfWeightLogWeights;
				entropies[i][x] = startingEntropy;
				observed[i][x] = -1;
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

	bool Run(int32 seed, int32 limit) {
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
				if (!success){
					return false;
				}
			}
			else {
				for (auto i : step(wave.height())) {
					for (auto d : step(wave.width())) {
						for (int32 t = 0; t < T; t++) {
							if (wave[i][d][t]) {
								observed[i][d] = t;
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

	void RunOneStep() {

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

	bool HasCompleted() {
		return not wave.isEmpty() && sumsOfOnes.asArray().sum() == sumsOfOnes.num_elements();
	}

	Point NextUnobservedNode() {
		if (heuristic == Heuristic::Scanline) {
			for (auto i : step(wave.height())) {
				for (auto d : step(wave.width())) {

					if (!periodic && (i % gridSize.x + N > gridSize.x || i / gridSize.x + N > gridSize.y))
						continue;

					if (sumsOfOnes[i][d] > 1) {
						observedSoFar = i + 1;
						return { d , i };;
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
						argmin = {x , y};
					}
				}
			}
		}
		return argmin;
	}
private:

	void Observe(const Point& node) {
		const Array<bool>& w = wave[node.y][node.x];

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
			auto current = stack[stacksize - 1];
			stacksize--;

			auto xy1 = current.first;
			int32 t1 = current.second;

			for (int32 d = 0; d < 4; d++) {
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

	void Ban(const Point& p, int32 t) {
		wave[p][t] = false;

		Array<int32>& comp = compatible[p][t];
		for (int32 d = 0; d < 4; d++) {
			comp[d] = 0;
		}

		stack[stacksize] = std::make_pair(p, t);
		stacksize++;

		sumsOfOnes[p] -= 1;
		sumsOfWeights[p] -= weights[t];
		sumsOfWeightLogWeights[p] -= weightLogWeights[t];

		double sum = sumsOfWeights[p];
		entropies[p] = Math::Log(sum) - sumsOfWeightLogWeights[p] / sum;
	}

};
