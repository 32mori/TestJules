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
