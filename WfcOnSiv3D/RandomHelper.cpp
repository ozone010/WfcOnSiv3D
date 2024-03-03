#include "stdafx.h"
#include "RandomHelper.hpp"

int32 RandomHelper::Random(const Array<double>& weights, double r)
{
	double sum = 0;
	for (int32 i = 0; i < weights.size(); i++) {
		sum += weights[i];
	}
	double threshold = r * sum;

	double partialSum = 0;
	for (int32 i = 0; i < weights.size(); i++)
	{
		partialSum += weights[i];
		if (partialSum >= threshold)
			return i;
	}
	return 0;
}
