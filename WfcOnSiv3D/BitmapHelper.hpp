# pragma once
class BitmapHelper
{
public:

	static Grid<Color> LoadBitmap(const FilePath& path);

	static Image ToImage(Grid<Color> data);
};

