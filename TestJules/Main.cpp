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
	static constexpr double SPEED = 200.0;      // 初期移動速度
	static constexpr double SPAWN_INTERVAL = 2.0; // 出現間隔
	static constexpr double WIDTH = 30.0;       // 幅
};

// 連結障害物関連の定数
constexpr double CONJOINED_OBSTACLE_CHANCE = 0.3; // 連結障害物の出現確率 (30%)
constexpr int MIN_CONJOINED_COUNT = 2;          // 連結する最小個数
constexpr int MAX_CONJOINED_COUNT = 3;          // 連結する最大個数
constexpr double SMALL_OBSTACLE_MIN_HEIGHT = 40.0;
constexpr double SMALL_OBSTACLE_MAX_HEIGHT = 70.0;
constexpr double LARGE_OBSTACLE_MIN_HEIGHT = 90.0;
constexpr double LARGE_OBSTACLE_MAX_HEIGHT = 120.0;
// 連結障害物間のX方向の隙間
constexpr double CONJOINED_OBSTACLE_X_GAP = Obstacle::WIDTH * 0.8;
// 右肩上がりのためのY座標オフセットステップ（次の障害物が前の障害物よりどれだけ高い位置に底辺が来るか）
constexpr double CONJOINED_OBSTACLE_Y_OFFSET_STEP = 15.0;
// 画面上部のマージン（障害物がこれより上にはみ出さないようにするための基準）
constexpr double SCREEN_TOP_MARGIN = 20.0;


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
// currentSpeed 引数を追加
void updateObstacles(s3d::Array<Obstacle>& obstacles, double& timeSinceLastSpawn, double groundY, double sceneWidth, double currentSpeed, const double deltaTime)
{
	timeSinceLastSpawn += deltaTime;
	if (timeSinceLastSpawn >= Obstacle::SPAWN_INTERVAL)
	{
		timeSinceLastSpawn = 0.0; // タイマーリセット

		// 連結障害物を生成するかどうか
		if (s3d::RandomBool(CONJOINED_OBSTACLE_CHANCE))
		{
			// 連結障害物を生成
			const int count = s3d::Random(MIN_CONJOINED_COUNT, MAX_CONJOINED_COUNT);
			double currentX = sceneWidth; // 最初の障害物のX座標
			bool isSmallNext = s3d::RandomBool(); // 最初が小さい障害物か

			for (int i = 0; i < count; ++i)
			{
				double obstacleHeight;
				if (isSmallNext) {
					obstacleHeight = s3d::Random(SMALL_OBSTACLE_MIN_HEIGHT, SMALL_OBSTACLE_MAX_HEIGHT);
				}
				else {
					obstacleHeight = s3d::Random(LARGE_OBSTACLE_MIN_HEIGHT, LARGE_OBSTACLE_MAX_HEIGHT);
				}

				// 「右肩上がり」の配置ロジック:
				// i (連結内の障害物のインデックス) が増えるほど、障害物の底辺のY座標を上昇させる。
				// これにより、見た目上、障害物群が右に進むにつれて上方向に伸びていくように見える。
				const double yPositionBottom = groundY - (i * CONJOINED_OBSTACLE_Y_OFFSET_STEP);
				// 障害物のrect.y（上端のY座標）を計算
				const double obstacleY = yPositionBottom - obstacleHeight;

				// Y座標が画面上部マージンより上に行かないように制限
				const double finalObstacleY = s3d::Max(obstacleY, SCREEN_TOP_MARGIN);
				// Y座標が制限された場合、障害物の高さも調整して底辺の位置を維持する
				const double finalObstacleHeight = yPositionBottom - finalObstacleY;

				// 調整後の高さが極端に小さい場合（例: 元の最小高さの半分未満）は、この障害物の生成をスキップ
				if (finalObstacleHeight < (SMALL_OBSTACLE_MIN_HEIGHT / 2.0))
				{
					// X座標だけは進めておく（次の障害物が重ならないように）
					currentX += Obstacle::WIDTH + CONJOINED_OBSTACLE_X_GAP;
					isSmallNext = !isSmallNext; // 大小の切り替えは行う
					continue;
				}

				obstacles.push_back(Obstacle{ s3d::RectF{currentX, finalObstacleY, Obstacle::WIDTH, finalObstacleHeight} });

				currentX += Obstacle::WIDTH + CONJOINED_OBSTACLE_X_GAP; // 次の障害物のX座標を設定
				isSmallNext = !isSmallNext; // 次の障害物の大小タイプを切り替える
			}
		}
		else
		{
			// 通常の単体障害物を生成
			// 高さのランダム範囲は、連結障害物で定義した小さい方の最小から大きい方の最大までとする
			// これにより、単体でも多様な高さの障害物が出現する
			const double obstacleHeight = s3d::Random(SMALL_OBSTACLE_MIN_HEIGHT, LARGE_OBSTACLE_MAX_HEIGHT);
			const double obstacleY = groundY - obstacleHeight; // 地面から生える形
			obstacles.push_back(Obstacle{ s3d::RectF{sceneWidth, obstacleY, Obstacle::WIDTH, obstacleHeight} });
		}
	}

	// 障害物を移動 (currentSpeed を使用)
	for (auto& obs : obstacles)
	{
		obs.rect.x -= currentSpeed * deltaTime;
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
	double currentObstacleSpeed = Obstacle::SPEED; // 現在の障害物速度
	const double maxObstacleSpeed = Obstacle::SPEED * 1.3; // 障害物の最大速度
	// 速度上昇の係数。スコアが増えるほど速度が上がる。
	// 例えばスコア60 (約60秒) で (1.0 + 60 * 0.005) = 1.3倍 となり、最大速度に達する。
	const double speedIncreaseFactor = 0.005;

	// ゲームの状態
	GameState gameState = GameState::Playing; // 初期状態はプレイ中

	while (s3d::System::Update())
	{
		const double deltaTime = s3d::Scene::DeltaTime(); // 前フレームからの経過時間

		// ゲームの状態に応じた処理分岐
		if (gameState == GameState::Playing)
		{
			score += deltaTime; // スコアを加算

			// 障害物速度の更新
			currentObstacleSpeed = s3d::Min(Obstacle::SPEED * (1.0 + score * speedIncreaseFactor), maxObstacleSpeed);

			// 各コンポーネントの更新
			updatePlayer(player, gravity, groundY, sceneWidth, deltaTime);
			updateObstacles(obstacles, timeSinceLastSpawn, groundY, sceneWidth, currentObstacleSpeed, deltaTime); // currentObstacleSpeed を渡す
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
				currentObstacleSpeed = Obstacle::SPEED; // 障害物速度もリセット
				gameState = GameState::Playing; // プレイ状態に戻す
			}
		}

		// 描画処理 (常に実行)
		drawGame(player, obstacles, score, groundY, sceneWidth, sceneHeight, gameState, finalScore, font, gameOverFont);
	}
}
