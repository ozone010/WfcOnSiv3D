# pragma once
# include "RandomHelper.hpp"

class WfcModel {

public:

	enum class Heuristic { Entropy, MRV, Scanline };

	void init();

	void clear();

	bool run(int32 seed, int32 limit);

	void runOneStep();

	bool hasCompleted() const;

protected:

	WfcModel(const Size& gridSize, int32 N, bool periodic, Heuristic heuristic);

	Grid<Array<bool>> m_wave;

	Array<Array<Array<int32>>> m_propagator;
	Grid<Array<Array<int32>>> m_compatible;
	Grid<int32> m_observed;

	int32 m_observedSoFar = 0;

	Size m_gridSize{0,0};

	int32 m_T = 0;
	int32 m_N = 0;
	bool m_periodic = false;
	bool m_ground = false;

	Array<double> m_weights;
	Array<double> m_distribution;

	Grid<int32> m_sumsOfOnes;
	Grid<double> m_sumsOfWeights;


	Heuristic m_heuristic;

	const Array<Point> dxy{ { -1, 0}, { 0, 1} , {1, 0},{ 0,-1} };
	const Array<int32> opposite{ 2, 3, 0, 1 };

private:

	Point nextUnm_observedNode();

	void observe(const Point& node);

	bool propagate();

	void ban(const Point& p, int32 t);

	Array<std::pair<Point, int32>> m_stack;
	int32 m_stacksize = 0;

	Array<double> m_weightLogWeights;

	Grid<double> m_sumsOfWeightLogWeights;
	double m_sumOfWeightLogWeights = 0;
	double m_sumOfWeights = 0;

	Grid<double> m_entropies;
	double m_startingEntropy = 0;
};
