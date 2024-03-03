#pragma once
#include "RandomHelper.hpp"

class WfcModel {

public:

	enum Heuristic { Entropy, MRV, Scanline };

protected:

	WfcModel(const Size& gridSize, int32 N, bool periodic, Heuristic heuristic);

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

public:
	void Init();

	void Clear();

	bool Run(int32 seed, int32 limit);

	void RunOneStep();

	bool HasCompleted();

private:

	Point NextUnobservedNode();

	void Observe(const Point& node);

	bool Propagate();

	void Ban(const Point& p, int32 t);

};
