# include <Siv3D.hpp> // Siv3D v0.6.16

// ゲームの状態を管理するための列挙型
enum class GameState
{
	Playing,
	GameOver,
};

// プレイヤーの情報をまとめる構造体
struct Player
{
	s3d::Circle circle;      // プレイヤーの形状と位置
	double velocityY;        // Y軸方向の速度
	int jumpCount;           // 現在のジャンプ回数
	int hp;                  // 体力
	bool isInvincible;       // 無敵状態か
	double invincibleTimer;  // 無敵状態のタイマー

	// 定数
	static constexpr double JUMP_POWER = 500.0; // ジャンプ力
	static constexpr int MAX_JUMP_COUNT = 2;   // 最大ジャンプ回数
	static constexpr double RADIUS = 20.0;      // プレイヤーの半径
	static constexpr int MAX_HP = 100;         // 最大体力
	static constexpr int DAMAGE_PER_HIT = 20;  // 被ダメージ量
	static constexpr double INVINCIBLE_DURATION = 0.5; // 無敵時間
};

// 障害物の情報をまとめる構造体
struct Obstacle
{
	s3d::RectF rect; // 障害物の形状と位置

	// 定数
	static constexpr double SPEED = 200.0;      // 移動速度
	static constexpr double SPAWN_INTERVAL = 2.0; // 出現間隔
	static constexpr double WIDTH = 30.0;       // 幅
};


// プレイヤー関連の変数を初期化する関数
Player initializePlayer(double sceneWidth, double groundY)
{
	return Player{
		s3d::Circle{ s3d::Vec2{ sceneWidth / 4, groundY - Player::RADIUS }, Player::RADIUS },
		0.0,
		0,
		Player::MAX_HP,
		false,
		0.0
	};
}

// プレイヤーステータスを更新する関数
void updatePlayer(Player& player, double gravity, double groundY, double sceneWidth, const double deltaTime)
{
	bool isOnGround = (player.circle.y >= groundY - player.circle.r);

	// ジャンプ処理
	if ((s3d::KeySpace.down() || s3d::KeyUp.down()))
	{
		if (isOnGround)
		{
			player.velocityY = -Player::JUMP_POWER;
			player.jumpCount = 1;
			isOnGround = false; // ジャンプしたら地面から離れるとみなす
		}
		else if (player.jumpCount < Player::MAX_JUMP_COUNT)
		{
			player.velocityY = -Player::JUMP_POWER;
			player.jumpCount++;
		}
	}

	// 重力の影響
	if (!isOnGround)
	{
		player.velocityY += gravity * deltaTime;
	}

	// プレイヤーの位置を更新
	player.circle.y += player.velocityY * deltaTime;

	// 地面との衝突判定
	if (player.circle.y > groundY - player.circle.r)
	{
		player.circle.y = groundY - player.circle.r;
		player.velocityY = 0;
		player.jumpCount = 0;
		// isOnGround はこの関数のローカル変数なので、ここで true にしても呼び出し元には影響しない。
		// 必要であれば、呼び出し元でプレイヤーの状態に基づいて再評価する。
	}
	player.circle.x = sceneWidth / 4; // X座標を常に固定

	// 無敵時間の更新
	if (player.isInvincible)
	{
		player.invincibleTimer += deltaTime;
		if (player.invincibleTimer >= Player::INVINCIBLE_DURATION)
		{
			player.isInvincible = false;
			player.invincibleTimer = 0.0;
		}
	}
}

// 障害物を生成・更新する関数
void updateObstacles(s3d::Array<Obstacle>& obstacles, double& timeSinceLastSpawn, double groundY, double sceneWidth, const double deltaTime)
{
	timeSinceLastSpawn += deltaTime;
	if (timeSinceLastSpawn >= Obstacle::SPAWN_INTERVAL)
	{
		timeSinceLastSpawn = 0.0; // タイマーリセット
		// 障害物の高さをランダムに決定 (例: 50から150の間)
		const double obstacleHeight = s3d::Random(50.0, 150.0);
		// 障害物のY座標を決定 (地面から生える形)
		const double obstacleY = groundY - obstacleHeight;
		obstacles.push_back(Obstacle{ s3d::RectF{sceneWidth, obstacleY, Obstacle::WIDTH, obstacleHeight} });
	}

	// 障害物を移動
	for (auto& obs : obstacles)
	{
		obs.rect.x -= Obstacle::SPEED * deltaTime;
	}

	// 画面外に出た障害物を削除
	obstacles.remove_if([](const Obstacle& obs) {
		return obs.rect.rightX() < 0;
	});
}

// 衝突処理を行う関数
// timeSinceLastSpawn も引数に追加 (ゲームオーバー時にリセットするため)
void handleCollisions(Player& player, s3d::Array<Obstacle>& obstacles, GameState& gameState, double& finalScore, double currentScore, double groundY, double& timeSinceLastSpawn_ref)
{
	for (auto it = obstacles.begin(); it != obstacles.end(); )
	{
		if (!player.isInvincible && player.circle.intersects(it->rect))
		{
			player.hp -= Player::DAMAGE_PER_HIT;
			player.isInvincible = true;
			player.invincibleTimer = 0.0; // 無敵タイマーリセット
			it = obstacles.erase(it); // 当たった障害物を削除

			if (player.hp <= 0)
			{
				player.hp = 0;
				finalScore = currentScore; // 最終スコアを記録
				gameState = GameState::GameOver; // ゲームオーバー状態へ
				obstacles.clear(); // ゲームオーバー時に残りの障害物もクリア
				timeSinceLastSpawn_ref = 0.0; // 障害物スポーンタイマーもリセット
				// プレイヤーを地面に戻す (見た目調整、操作不能にする)
				player.circle.y = groundY - player.circle.r;
				player.velocityY = 0;
				break; // ゲームオーバーになったら現在のフレームの当たり判定ループを抜ける
			}
			continue; // eraseしたのでイテレータは進めない
		}
		++it;
	}
}

// 描画処理を行う関数
void drawGame(const Player& player, const s3d::Array<Obstacle>& obstacles, double score, double groundY, double sceneWidth, double sceneHeight, GameState gameState, double finalScore, const s3d::Font& font, const s3d::Font& gameOverFont)
{
	// 地面を描画
	s3d::RectF{ 0, groundY, sceneWidth, sceneHeight - groundY }.draw(s3d::Palette::Lightgreen);

	// プレイヤーを描画
	if (gameState == GameState::Playing)
	{
		if (player.isInvincible)
		{
			// 無敵中は点滅 (0.2秒周期で0.1秒表示)
			if (fmod(s3d::Scene::Time(), 0.2) < 0.1)
			{
				player.circle.draw(s3d::Palette::Orange);
			}
		}
		else
		{
			player.circle.draw(s3d::Palette::Orange);
		}
	}
	else if (gameState == GameState::GameOver)
	{
		// ゲームオーバー時は常に表示 (または点滅させても良い)
		// 例えば、ゲームオーバー時も点滅させるなら以下のようにする
		// if (fmod(s3d::Scene::Time(), 0.2) < 0.1) { // 点滅させる場合の例
		//    player.circle.draw(s3d::Palette::Orange);
		// }
		player.circle.draw(s3d::Palette::Orange); // ここでは常に表示
	}


	// 障害物を描画
	for (const auto& obs : obstacles)
	{
		obs.rect.draw(s3d::Palette::Gray);
	}

	// UI描画 (スコアとHP)
	font(U"Score: {:.1f}"_fmt(score)).draw(sceneWidth - 200, 20, s3d::Palette::Black);
	font(U"HP: {}"_fmt(player.hp)).draw(20, 20, s3d::Palette::Red);

	// ゲームオーバー表示
	if (gameState == GameState::GameOver)
	{
		// 画面中央にゲームオーバー関連のテキストを表示
		gameOverFont(U"Game Over").drawAt(s3d::Vec2{ sceneWidth / 2, sceneHeight / 2 - 30 }, s3d::Palette::Black);
		font(U"Final Score: {:.1f}"_fmt(finalScore)).drawAt(s3d::Vec2{ sceneWidth / 2, sceneHeight / 2 + 30 }, s3d::Palette::Black);
		font(U"Press 'R' to Restart").drawAt(s3d::Vec2{ sceneWidth / 2, sceneHeight / 2 + 70 }, s3d::Palette::Darkgray);
	}
}


void Main()
{
	// 背景の色を設定する
	s3d::Scene::SetBackground(s3d::ColorF{ 0.6, 0.8, 0.7 });

	// 画面サイズ取得
	const double sceneWidth = s3d::Scene::Width();
	const double sceneHeight = s3d::Scene::Height();

	// 定数定義
	const double groundY = sceneHeight - 50.0; // 地面のY座標
	const double gravity = 1000.0;             // 重力加速度

	// プレイヤーの初期化
	Player player = initializePlayer(sceneWidth, groundY);

	// スコア関連
	double score = 0.0;        // 現在のスコア
	double finalScore = 0.0;   // ゲームオーバー時の最終スコア

	// フォントの準備
	const s3d::Font font{ 30 }; // 通常表示用フォント
	const s3d::Font gameOverFont{ 60, s3d::Typeface::Bold }; // ゲームオーバー表示用フォント

	// 障害物関連
	s3d::Array<Obstacle> obstacles;          // 障害物リスト
	double timeSinceLastSpawn = 0.0;         // 前回の障害物出現からの経過時間

	// ゲームの状態
	GameState gameState = GameState::Playing; // 初期状態はプレイ中

	while (s3d::System::Update())
	{
		const double deltaTime = s3d::Scene::DeltaTime(); // 前フレームからの経過時間

		// ゲームの状態に応じた処理分岐
		if (gameState == GameState::Playing)
		{
			score += deltaTime; // スコアを加算

			// 各コンポーネントの更新
			updatePlayer(player, gravity, groundY, sceneWidth, deltaTime);
			updateObstacles(obstacles, timeSinceLastSpawn, groundY, sceneWidth, deltaTime);
			handleCollisions(player, obstacles, gameState, finalScore, score, groundY, timeSinceLastSpawn);
		}
		else if (gameState == GameState::GameOver)
		{
			// ゲームオーバー時のリスタート処理
			if (s3d::KeyR.down()) // Rキーが押されたら
			{
				// ゲーム状態をリセット
				player = initializePlayer(sceneWidth, groundY);
				score = 0.0;
				finalScore = 0.0;
				obstacles.clear();
				timeSinceLastSpawn = 0.0; // 障害物出現タイミングもリセット
				gameState = GameState::Playing; // プレイ状態に戻す
			}
		}

		// 描画処理 (常に実行)
		drawGame(player, obstacles, score, groundY, sceneWidth, sceneHeight, gameState, finalScore, font, gameOverFont);
	}
}
