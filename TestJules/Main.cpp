# include <Siv3D.hpp> // Siv3D v0.6.16

void Main()
{
	// 背景の色を設定する | Set the background color
	s3d::Scene::SetBackground(s3d::ColorF{ 0.6, 0.8, 0.7 });

	s3d::Vec2 playerPosition{ 400, 300 };

	while (s3d::System::Update())
	{
		// WASD key checks for playerPosition
		if (s3d::KeyW.pressed()) { playerPosition.y -= 5.0; }
		if (s3d::KeyA.pressed()) { playerPosition.x -= 5.0; }
		if (s3d::KeyS.pressed()) { playerPosition.y += 5.0; }
		if (s3d::KeyD.pressed()) { playerPosition.x += 5.0; }

		// Draw the new player circle
		s3d::Circle(playerPosition, 30).draw(s3d::Palette::Orange);
	}
}

//
// - Debug ビルド: プログラムの最適化を減らす代わりに、エラーやクラッシュ時に詳細な情報を得られます。
//
// - Release ビルド: 最大限の最適化でビルドします。
//
// - [デバッグ] メニュー → [デバッグの開始] でプログラムを実行すると、[出力] ウィンドウに詳細なログが表示され、エラーの原因を探せます。
//
// - Visual Studio を更新した直後は、プログラムのリビルド（[ビルド]メニュー → [ソリューションのリビルド]）が必要な場合があります。
//
