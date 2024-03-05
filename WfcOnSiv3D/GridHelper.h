#pragma once
class GridHelper
{
public:

	template <typename T>
	static Grid<T> rotated270(const Grid<T>& src) {
		Grid<T> rotated{src.height() , src.width() };
		for (size_t i = 0; i < rotated.height(); ++i) {
			for (size_t j = 0; j < rotated.width(); ++j) {
				rotated[rotated.width() - 1 - j][i] = src[i][j];
			}
		}
		return rotated;
	}

	template <typename T>
	static Grid<T> mirrored(const Grid<T>& src) {
		Grid<T> reflected(src.width(), src.height());
		for (size_t i = 0; i < src.height(); ++i) {
			for (size_t j = 0; j < src.width(); ++j) {
				reflected[i][j] = src[i][src.width()- 1 - j];
			}
		}
		return reflected;
	}
};

