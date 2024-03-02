//"https://github.com/mxgmn/WaveFunctionCollapse"のSiv3Dへの移植です。


# include <Siv3D.hpp> // Siv3D v0.6.14
# include "OverlappingModel.h"
# include "SimpleTiledModel.h"

void Main()
{
	Window::Resize(1024, 576);

	//背景色設定
	Scene::SetBackground(ColorF{ 0.6, 0.6, 0.8 });

	//フォント
	const Font font{ FontMethod::MSDF, 16, Typeface::Bold };

	//OverlappingModelの元画像のパス
	const String SRC_IMG_PATH = U"Sewers.png";

	//生成器を作成
	OverlappingModel olModel{
		SRC_IMG_PATH,
		3, //N
		{ 96,96 }, //gridSize
		true, //periodicInput
		true, //periodic
		8, //symmetry
		false, //ground
		WfcModel::Heuristic::Entropy
	};
	olModel.Init();
	olModel.Clear();

	SimpleTiledModel stModel{
		U"tilesets/Summer.json",
		U"", //subsetName
		{ 16 ,16 }, //gridSize
		true, //periodic
		true, //blackBackground
		WfcModel::Heuristic::Entropy
	};
	stModel.Init();
	stModel.Clear();

	SimpleTiledModel st2Model{
		U"tilesets/Circuit.json",
		U"Turnless", //subsetName
		{ 16 ,16 }, //gridSize
		true, //periodic
		false, //blackBackground
		WfcModel::Heuristic::Entropy
	};
	st2Model.Init();
	st2Model.Clear();

	//乱数のシード値
	int32 olSeed = 0;
	int32 stSeed = 0;
	int32 st2Seed = 0;

	//何回生成をリトライしたかのカウンタ
	int32 olRetryCount = 0;
	int32 stRetryCount = 0;
	int32 st2RetryCount = 0;

	//元画像のテクスチャを生成
	Texture srcTexture{ SRC_IMG_PATH };

	//生成した画像をテクスチャに変換
	DynamicTexture olResultTexture(olModel.ImageSize());
	DynamicTexture stResultTexture(stModel.ImageSize());
	DynamicTexture st2ResultTexture(st2Model.ImageSize());

	olResultTexture.fill(olModel.ToImage());
	stResultTexture.fill(stModel.ToImage());
	st2ResultTexture.fill(st2Model.ToImage());



	while (System::Update())
	{
		const ScopedRenderStates2D sampler{ SamplerState::ClampNearest };


		//OverlappingModel
		{
			const int32 shitX = 0;
			//Regenerateボタン
			if (SimpleGUI::Button(U"Generate", Vec2{ 10, 10 } + Vec2{ shitX , 0 })) {

				//成功するまで生成
				for (olRetryCount = 0; not olModel.Run(olSeed = Random<int32>(INT_MIN, INT_MAX), -1); ++olRetryCount);

				//生成した画像をテクスチャに変換
				olResultTexture.fill(olModel.ToImage());
			}
			//Clearボタン
			if (SimpleGUI::Button(U"Clear", Vec2{ 10 , 50 } + Vec2{ shitX , 0 })) {
				olModel.Clear();
				olResultTexture.fill(olModel.ToImage());
			}
			//Stepボタン
			if (SimpleGUI::Button(U"Step", Vec2{ 10 + 100, 50 } + Vec2{ shitX , 0 }, unspecified, not olModel.HasCompleted())) {

				//1ステップ
				olModel.RunOneStep();

				//生成した画像をテクスチャに変換
				olResultTexture.fill(olModel.ToImage());
			}

			//情報の表示
			font(U"seed: {}"_fmt(olSeed)).draw(16, Vec2{ 10, 100 } + Vec2{ shitX , 0 });
			font(U"retryCount: {}"_fmt(olRetryCount)).draw(16, Vec2{ 10, 125 } + Vec2{ shitX , 0 });
			font(U"hasCompleted: {}"_fmt(olModel.HasCompleted())).draw(16, Vec2{ 10, 150 } + Vec2{ shitX , 0 });
			srcTexture.scaled(3).draw(Vec2{ 200, 100 } + Vec2{ shitX , 0 });

			//生成画像を表示
			olResultTexture.resized(300).draw(Vec2{10, 180} + Vec2{ shitX , 0 });
		}

		//SimpleTiledModel(Subsetなし)
		{
			const int32 shitX = 352;

			//Regenerateボタン
			if (SimpleGUI::Button(U"Generate", Vec2{ 10, 10 } + Vec2{ shitX , 0 })) {

				//成功するまで生成
				for (stRetryCount = 0; not stModel.Run(stSeed = Random<int32>(INT_MIN, INT_MAX), -1); ++stRetryCount);

				//生成した画像をテクスチャに変換
				stResultTexture.fill(stModel.ToImage());
			}
			//Clearボタン
			if (SimpleGUI::Button(U"Clear", Vec2{ 10 , 50 } + Vec2{ shitX , 0 })) {
				stModel.Clear();
				stResultTexture.fill(stModel.ToImage());
			}
			//Stepボタン
			if (SimpleGUI::Button(U"Step", Vec2{ 10 + 100, 50 } + Vec2{ shitX , 0 }, unspecified, not stModel.HasCompleted())) {

				//1ステップ
				stModel.RunOneStep();

				//生成した画像をテクスチャに変換
				stResultTexture.fill(stModel.ToImage());
			}

			//情報の表示
			font(U"seed: {}"_fmt(stSeed)).draw(16, Vec2{ 10 , 100 } + Vec2{ shitX , 0 });
			font(U"retryCount: {}"_fmt(stRetryCount)).draw(16, Vec2{ 10 , 125 } + Vec2{ shitX , 0 });
			font(U"hasCompleted: {}"_fmt(stModel.HasCompleted())).draw(16, Vec2{ 10 , 150 } + Vec2{ shitX , 0 });

			//生成画像を表示
			stResultTexture.resized(300).draw(Vec2{ 10, 180 } + Vec2{ shitX , 0 });
		}


		//SimpleTiledModel(Subsetあり)
		{
			const int32 shitX = 704;

			//Regenerateボタン
			if (SimpleGUI::Button(U"Generate", Vec2{ 10, 10 } + Vec2{ shitX , 0 })) {

				//成功するまで生成
				for (st2RetryCount = 0; not st2Model.Run(st2Seed = Random<int32>(INT_MIN, INT_MAX), -1); ++st2RetryCount);

				//生成した画像をテクスチャに変換
				st2ResultTexture.fill(st2Model.ToImage());
			}
			//Clearボタン
			if (SimpleGUI::Button(U"Clear", Vec2{ 10 , 50 } + Vec2{ shitX , 0 })) {
				st2Model.Clear();
				st2ResultTexture.fill(st2Model.ToImage());
			}
			//Stepボタン
			if (SimpleGUI::Button(U"Step", Vec2{ 10 + 100, 50 } + Vec2{ shitX , 0 }, unspecified, not st2Model.HasCompleted())) {

				//1ステップ
				st2Model.RunOneStep();

				//生成した画像をテクスチャに変換
				st2ResultTexture.fill(st2Model.ToImage());
			}

			//情報の表示
			font(U"seed: {}"_fmt(st2Seed)).draw(16, Vec2{ 10 , 100 } + Vec2{ shitX , 0 });
			font(U"retryCount: {}"_fmt(st2RetryCount)).draw(16, Vec2{ 10 , 125 } + Vec2{ shitX , 0 });
			font(U"hasCompleted: {}"_fmt(st2Model.HasCompleted())).draw(16, Vec2{ 10 , 150 } + Vec2{ shitX , 0 });

			//生成画像を表示
			st2ResultTexture.resized(300).draw(Vec2{ 10, 180 } + Vec2{ shitX , 0 });
		}
	}
}
