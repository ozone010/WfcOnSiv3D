#pragma once
class RandomHelper
{
public:

	static int32 Random(const Array<double>& weights, double r)
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
};

