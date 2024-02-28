//"https://github.com/mxgmn/WaveFunctionCollapse"のSiv3Dへの移植です。


# include <Siv3D.hpp> // Siv3D v0.6.14
# include "OverlappingModel.h"
# include "SimpleTiledModel.h"

void Main()
{
	//背景色設定
	Scene::SetBackground(ColorF{ 0.6, 0.6, 0.8 });

	//フォント
	const Font font{ FontMethod::MSDF, 16, Typeface::Bold };

	//元画像のパス
	const String SRC_IMG_PATH = U"Sewers.png";

	//生成器を作成
	const Size olSize{ 96,96 };
	OverlappingModel olModel{
		SRC_IMG_PATH,
		3, //N
		olSize.x, //width
		olSize.y, //height
		true, //periodicInput
		true, //periodic
		8, //symmetry
		false, //ground
		WfcModel::Heuristic::Entropy
	};

	const Size stGridSize{ 20,20 };
	const Size stTileTextureSize{ 48,48 };

	SimpleTiledModel stModel{
		U"tilesets/Summer.json",
		U"", //subsetName
		stGridSize.x, //width
		stGridSize.y, //height
		true, //periodic
		true, //blackBackground
		WfcModel::Heuristic::Entropy
	};

	//乱数のシード値
	int32 olSeed = 0;
	int32 stSeed = 0;

	//何回生成をリトライしたかのカウンタ
	int32 olRetryCount = 0;
	int32 stRetryCount = 0;

	//元画像のテクスチャを生成
	Texture srcTexture{ SRC_IMG_PATH };

	//生成した画像をテクスチャに変換
	DynamicTexture olResultTexture(olSize);
	DynamicTexture stResultTexture(stGridSize * stTileTextureSize);

	while (System::Update())
	{
		//Regenerateボタン
		if (SimpleGUI::Button(U"Generate", Vec2{ 10, 10 })){

			//成功するまで生成
			for (olRetryCount = 0; not olModel.Run(olSeed = Random<int32>(INT_MIN, INT_MAX), -1); ++olRetryCount);

			//生成した画像をテクスチャに変換
			olResultTexture.fill(olModel.ToImage());
		}

		//Regenerateボタン
		if (SimpleGUI::Button(U"Generate", Vec2{ 10 + 430, 10 } )) {

			//成功するまで生成
			for (stRetryCount = 0; not stModel.Run(stSeed = Random<int32>(INT_MIN, INT_MAX), -1); ++stRetryCount);

			//生成した画像をテクスチャに変換
			stResultTexture.fill(stModel.ToImage());
		}


		//情報の表示
		font(U"seed: {}"_fmt(olSeed)).draw(16, Vec2{ 10, 60 });
		font(U"retryCount: {}"_fmt(olRetryCount)).draw(16, Vec2{ 10, 90 });

		font(U"seed: {}"_fmt(stSeed)).draw(16, Vec2{ 10 + 430, 60 });
		font(U"retryCount: {}"_fmt(stRetryCount)).draw(16, Vec2{ 10 + 430, 90 });


		const ScopedRenderStates2D sampler{ SamplerState::ClampNearest };

		//元画像を表示
		srcTexture.scaled(3).draw(10, 130);

		//生成画像を表示
		olResultTexture.scaled(3).draw(100, 130);

		stResultTexture.scaled(0.33).draw(10 + 430, 130);
	}
}
