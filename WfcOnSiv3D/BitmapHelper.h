#pragma once
class BitmapHelper
{
public:
	static std::tuple<Array<int32>, int32, int32> LoadBitmap(const FilePath& path) {

		const Image image{ path };
		Array<int32> arr;

		;

		for (auto& pixel : image)
		{
			arr << static_cast<int32>(pixel.asUint32());
		}

		return std::make_tuple(arr, image.width(), image.height());
	}

	static Image ToImage(Array<int32> data, size_t width, size_t height)
	{
		Image image{ width, height};
		for (int32 y = 0; y < image.height(); ++y)
		{
			for (int32 x = 0; x < image.width(); ++x)
			{
				uint32 pixel = data[y * width + x];

				uint8 alpha = pixel & 0xFF;
				uint8 red = (pixel >> 8) & 0xFF;
				uint8 green = (pixel >> 16) & 0xFF;
				uint8 blue = (pixel >> 24) & 0xFF;


				image[y][x] = Color(alpha, red, green, blue);
			}
		}

		return image;
	}
};

