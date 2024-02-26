//"https://github.com/mxgmn/WaveFunctionCollapse"のSiv3Dへの移植です。


# include <Siv3D.hpp> // Siv3D v0.6.14
# include "OverlappingModel.h"

void Main()
{
	//背景色設定
	Scene::SetBackground(ColorF{ 0.6, 0.6, 0.8 });

	//フォント
	const Font font{ FontMethod::MSDF, 16, Typeface::Bold };

	//元画像のパス
	const String SRC_IMG_PATH = U"Sewers.png";

	//生成器を作成
	OverlappingModel model{
		SRC_IMG_PATH,
		3, //N
		96, //width
		96, //height
		true, //periodicInput
		true, //periodic
		8, //symmetry
		false, //ground
		WfcModel::Heuristic::Entropy
	};

	//乱数のシード値
	int32 seed = Random<int32>(INT_MIN, INT_MAX);

	//何回生成をリトライしたかのカウンタ
	int32 retryCount = 0;



	//成功するまで生成（このアルゴリズムはたまに失敗する）
	for (retryCount = 0; not model.Run(seed = Random<int32>(INT_MIN, INT_MAX), -1); ++retryCount);

	//元画像のテクスチャを生成
	Texture srcTexture{ SRC_IMG_PATH };

	//生成した画像をテクスチャに変換
	Texture resultTexture{ model.ToImage() };


	while (System::Update())
	{
		//Regenerateボタン
		if (SimpleGUI::Button(U"Regenerate", Vec2{ 10, 10 })){

			//成功するまで生成
			for (retryCount = 0; not model.Run(seed = Random<int32>(INT_MIN, INT_MAX), -1); ++retryCount);

			//生成した画像をテクスチャに変換
			resultTexture = Texture{ model.ToImage() };
		}

		//情報の表示
		font(U"seed: {}"_fmt(seed)).draw(16, Vec2{ 10, 60 });
		font(U"retryCount: {}"_fmt(retryCount)).draw(16, Vec2{ 10, 90 });


		const ScopedRenderStates2D sampler{ SamplerState::ClampNearest };

		//元画像を表示
		srcTexture.scaled(4).draw(10, 130);

		//生成画像を表示
		resultTexture.scaled(4).draw(200, 130);

	}
}
