﻿//"https://github.com/mxgmn/WaveFunctionCollapse"のSiv3Dへの移植です。


# include <Siv3D.hpp> // Siv3D v0.6.14
# include "OverlappingModel.hpp"
# include "SimpleTiledModel.hpp"

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
	olModel.init();
	olModel.clear();

	SimpleTiledModel stModel{
		U"tilesets/Summer.json",
		U"", //subsetName
		{ 16 ,16 }, //gridSize
		true, //periodic
		true, //blackBackground
		WfcModel::Heuristic::Entropy
	};
	stModel.init();
	stModel.clear();

	SimpleTiledModel st2Model{
		U"tilesets/FloorPlan.json",
		U"", //subsetName
		{ 16 ,16 }, //gridSize
		true, //periodic
		false, //blackBackground
		WfcModel::Heuristic::Entropy
	};
	st2Model.init();
	st2Model.clear();

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
	DynamicTexture olResultTexture(olModel.imageSize());
	DynamicTexture stResultTexture(stModel.imageSize());
	DynamicTexture st2ResultTexture(st2Model.imageSize());

	olResultTexture.fill(olModel.toImage());
	stResultTexture.fill(stModel.toImage());
	st2ResultTexture.fill(st2Model.toImage());



	while (System::Update())
	{
		const ScopedRenderStates2D sampler{ SamplerState::ClampNearest };


		//OverlappingModel
		{
			const int32 shitX = 0;
			//Regenerateボタン
			if (SimpleGUI::Button(U"Generate", Vec2{ 10, 10 } + Vec2{ shitX , 0 })) {

				//成功するまで生成
				for (olRetryCount = 0; not olModel.run(olSeed = Random<int32>(INT_MIN, INT_MAX), -1); ++olRetryCount);

				//生成した画像をテクスチャに変換
				olResultTexture.fill(olModel.toImage());
			}
			//Clearボタン
			if (SimpleGUI::Button(U"Clear", Vec2{ 10 , 50 } + Vec2{ shitX , 0 })) {
				olModel.clear();
				olResultTexture.fill(olModel.toImage());
			}
			//Stepボタン
			if (SimpleGUI::Button(U"Step", Vec2{ 10 + 100, 50 } + Vec2{ shitX , 0 }, unspecified, not olModel.hasCompleted())) {

				//1ステップ
				olModel.runOneStep();

				//生成した画像をテクスチャに変換
				olResultTexture.fill(olModel.toImage());
			}

			//情報の表示
			font(U"seed: {}"_fmt(olSeed)).draw(16, Vec2{ 10, 100 } + Vec2{ shitX , 0 });
			font(U"retryCount: {}"_fmt(olRetryCount)).draw(16, Vec2{ 10, 125 } + Vec2{ shitX , 0 });
			font(U"hasCompleted: {}"_fmt(olModel.hasCompleted())).draw(16, Vec2{ 10, 150 } + Vec2{ shitX , 0 });
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
				for (stRetryCount = 0; not stModel.run(stSeed = Random<int32>(INT_MIN, INT_MAX), -1); ++stRetryCount);

				//生成した画像をテクスチャに変換
				stResultTexture.fill(stModel.toImage());
			}
			//Clearボタン
			if (SimpleGUI::Button(U"Clear", Vec2{ 10 , 50 } + Vec2{ shitX , 0 })) {
				stModel.clear();
				stResultTexture.fill(stModel.toImage());
			}
			//Stepボタン
			if (SimpleGUI::Button(U"Step", Vec2{ 10 + 100, 50 } + Vec2{ shitX , 0 }, unspecified, not stModel.hasCompleted())) {

				//1ステップ
				stModel.runOneStep();

				//生成した画像をテクスチャに変換
				stResultTexture.fill(stModel.toImage());
			}

			//情報の表示
			font(U"seed: {}"_fmt(stSeed)).draw(16, Vec2{ 10 , 100 } + Vec2{ shitX , 0 });
			font(U"retryCount: {}"_fmt(stRetryCount)).draw(16, Vec2{ 10 , 125 } + Vec2{ shitX , 0 });
			font(U"hasCompleted: {}"_fmt(stModel.hasCompleted())).draw(16, Vec2{ 10 , 150 } + Vec2{ shitX , 0 });

			//生成画像を表示
			stResultTexture.resized(300).draw(Vec2{ 10, 180 } + Vec2{ shitX , 0 });
		}


		//SimpleTiledModel(Subsetあり)
		{
			const int32 shitX = 704;

			//Regenerateボタン
			if (SimpleGUI::Button(U"Generate", Vec2{ 10, 10 } + Vec2{ shitX , 0 })) {

				//成功するまで生成
				for (st2RetryCount = 0; not st2Model.run(st2Seed = Random<int32>(INT_MIN, INT_MAX), -1); ++st2RetryCount);

				//生成した画像をテクスチャに変換
				st2ResultTexture.fill(st2Model.toImage());
			}
			//Clearボタン
			if (SimpleGUI::Button(U"Clear", Vec2{ 10 , 50 } + Vec2{ shitX , 0 })) {
				st2Model.clear();
				st2ResultTexture.fill(st2Model.toImage());
			}
			//Stepボタン
			if (SimpleGUI::Button(U"Step", Vec2{ 10 + 100, 50 } + Vec2{ shitX , 0 }, unspecified, not st2Model.hasCompleted())) {

				//1ステップ
				st2Model.runOneStep();

				//生成した画像をテクスチャに変換
				st2ResultTexture.fill(st2Model.toImage());
			}

			//情報の表示
			font(U"seed: {}"_fmt(st2Seed)).draw(16, Vec2{ 10 , 100 } + Vec2{ shitX , 0 });
			font(U"retryCount: {}"_fmt(st2RetryCount)).draw(16, Vec2{ 10 , 125 } + Vec2{ shitX , 0 });
			font(U"hasCompleted: {}"_fmt(st2Model.hasCompleted())).draw(16, Vec2{ 10 , 150 } + Vec2{ shitX , 0 });

			//生成画像を表示
			st2ResultTexture.resized(300).draw(Vec2{ 10, 180 } + Vec2{ shitX , 0 });
		}
	}
}
