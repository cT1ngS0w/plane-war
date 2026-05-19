#pragma once

#include <cstdint>
#include <string>
#include <vector>

// ---------- 常量 ----------
constexpr int kFieldWidth  = 34;
constexpr int kFieldHeight = 22;

constexpr int kPlayerStartX = kFieldWidth / 2;
constexpr int kPlayerStartY = kFieldHeight - 3;

constexpr int kMaxLives        = 5;
constexpr int kMaxPlayerBullets = 12;
constexpr int kMaxEnemyBullets   = 40;
constexpr int kMaxEnemies        = 10;
constexpr int kMaxPowerUps       = 3;
constexpr int kMaxObstacles      = 6;

constexpr int kPlayerShootCooldown = 8;
constexpr int kEnemyShootInterval  = 45;
constexpr int kEnemySpawnInterval  = 50;
constexpr int kObstacleSpawnBase   = 200;

constexpr int kScorePerKill    = 100;
constexpr int kKillsPerLevel   = 10;
constexpr int kBossScoreBonus  = 1000;

constexpr int kPlayerW = 3, kPlayerH = 2;
constexpr int kEnemyW  = 3, kEnemyH  = 2;

// ---------- 枚举 ----------
enum class CellType { Empty, Player, Enemy, PlayerBullet, EnemyBullet,
                      Border, Obstacle, PowerUp, Boss, Particle };
enum class Dir       { kNone, kUp, kDown, kLeft, kRight };
enum class GameState { kMenu, kPlaying, kPaused, kGameOver, kLevelClear };
enum class PowerUpType { Health, FireRate, DualShot };

// ---------- 精灵 ----------
struct SpriteCell { int dx, dy; char ch; };

// 玩家：3x2 战斗机
inline constexpr SpriteCell kPlayerSprite[] = {
    { 0,-1,'^'}, {-1,0,'/'}, {0,0,'H'}, {1,0,'\\'},
};
// 敌机：3x2
inline constexpr SpriteCell kEnemySprite[] = {
    {-1,0,'\\'}, {0,0,'M'}, {1,0,'/'}, {0,1,'v'},
};
// Boss：5x3 重型轰炸机
inline constexpr SpriteCell kBossSprite[] = {
    {-2,0,'/'}, {-1,0,' '}, {0,0,'^'}, {1,0,' '}, {2,0,'\\'},
    {-2,1,'|'}, {-1,1,'-'}, {0,1,'B'}, {1,1,'-'}, {2,1,'|'},
    {-2,2,'\\'}, {-1,2,'_'}, {0,2,'_'}, {1,2,'_'}, {2,2,'/'},
};

// ---------- 实体 ----------
struct Bullet {
    int x, y, dx = 0, dy = 0;
    bool active = false;
    bool from_player;
};

struct Enemy {
    int x, y, hp = 2;
    bool active = false;
    int shoot_timer = 0;
    int speed_counter = 0;
};

struct Player {
    int x = kPlayerStartX, y = kPlayerStartY;
    int lives = 3;
    int shoot_cd = 0;
};

struct PowerUp {
    int x, y;
    PowerUpType type;
    bool active = false;
    int speed_counter = 0;
};

struct Obstacle {
    int x, y;
    int speed_counter = 0;
};

struct Particle {
    int x, y, dx, dy;
    int life;
    char ch;
};

struct Boss {
    int x, y;
    int hp = 0, max_hp = 0;
    bool active = false;
    int move_dir = 1;
    int shoot_timer = 0;
    int pattern_timer = 0;
};

// ---------- 游戏主类 ----------
class Game {
public:
    Game();

    void Update(Dir move_dir, bool shooting);
    std::string Render() const;
    CellType GetCellType(int x, int y) const;

    GameState state() const { return state_; }
    int score()        const { return score_; }
    int level()        const { return level_; }
    int lives()        const { return player_.lives; }
    int kills()        const { return total_kills_; }
    int boss_hp()      const { return boss_.hp; }
    int boss_max_hp()  const { return boss_.max_hp; }
    bool boss_alive()  const { return boss_.active; }
    int fire_boost()   const { return fire_rate_boost_; }
    int dual_shot()    const { return dual_shot_; }

    void StartGame();
    void NextLevel();
    void TogglePause();

private:
    void SpawnEnemy();
    void SpawnEnemyBullet(const Enemy& e);
    void SpawnObstacle();
    void FirePlayerBullet();
    void MovePlayer(Dir d);
    void MoveBullets();
    void MoveEnemies();
    void MovePowerUps();
    void MoveObstacles();
    void MoveBoss();
    void CheckCollisions();
    void CheckLevelProgress();
    void CheckPowerUpCollect();

    void SpawnBoss();
    void BossShootCircle();
    void BossShootSpread();
    void BossDie();

    void SpawnPowerUp();
    void SpawnExplosion(int x, int y);
    void UpdateParticles();

    bool IsPlayerCell(int x, int y) const;
    bool IsEnemyCell(const Enemy& e, int x, int y) const;
    bool IsBossCell(int x, int y) const;
    bool IsObstacle(int x, int y) const;

    GameState state_ = GameState::kMenu;
    Player    player_;
    std::vector<Bullet>   bullets_;
    std::vector<Enemy>    enemies_;
    std::vector<PowerUp>  powerups_;
    std::vector<Obstacle>  obstacles_;
    std::vector<Particle> particles_;
    Boss      boss_;

    int score_ = 0, high_score_ = 0;
    int level_ = 1, total_kills_ = 0;

    int enemy_spawn_timer_    = 0;
    int enemy_speed_          = 12;
    int frame_count_          = 0;
    int obstacle_spawn_timer_ = 80;
    int powerup_spawn_timer_  = 200;
    bool boss_spawned_        = false;

    int fire_rate_boost_ = 0;
    int dual_shot_       = 0;
};
