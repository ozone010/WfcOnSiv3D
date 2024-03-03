#include "stdafx.h"
#include "BitmapHelper.hpp"

Grid<Color> BitmapHelper::LoadBitmap(const FilePath& path) {
	const Image image(path);
	return Grid<Color>(image.width(), image.height(), image.asArray());
}


Image BitmapHelper::ToImage(Grid<Color> data) {
	Image image{ data.size() };
	for (auto y : step(data.height()))
	{
		for (auto x : step(data.width()))
		{
			uint32 pixel = data[y][x].asUint32();

			uint8 alpha = pixel & 0xFF;
			uint8 red = (pixel >> 8) & 0xFF;
			uint8 green = (pixel >> 16) & 0xFF;
			uint8 blue = (pixel >> 24) & 0xFF;

			image[y][x] = Color(alpha, red, green, blue);
		}
	}

	return image;
}
