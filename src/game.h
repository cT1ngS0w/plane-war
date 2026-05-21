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
constexpr int kMaxPlayerBullets = 20;
constexpr int kMaxEnemyBullets   = 40;
constexpr int kMaxWingmen        = 2;
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
                      Border, Obstacle, PowerUp, Boss, Particle, Wingman };
enum class Dir       { kNone, kUp, kDown, kLeft, kRight };
enum class GameState { kMenu, kSelect, kPlaying, kPaused, kGameOver, kLevelClear };
enum class PowerUpType { Health, FireRate, DualShot, Shield, Bomb, Magnet };
enum class PlaneType { kFighter, kBomber, kStealth };

// ---------- 精灵 ----------
struct SpriteCell { int dx, dy; char ch; };

// 玩家精灵 — 三种战机
// Fighter "Valkyrie" 迅风战机：敏捷型 3x2
inline constexpr SpriteCell kFighterSprite[] = {
    { 0,-1,'^'}, {-1,0,'/'}, {0,0,'F'}, {1,0,'\\'},
};
// Bomber "Fortress" 堡垒重舰：重装型 5x2
inline constexpr SpriteCell kBomberSprite[] = {
    {-2,0,'['}, {-1,0,'#'}, {0,0,'B'}, {1,0,'#'}, {2,0,']'},
    {-2,1,'/'}, {-1,1,'='}, {0,1,'='}, {1,1,'='}, {2,1,'\\'},
};
// Stealth "Phantom" 暗影战机：刺客型 3x2
inline constexpr SpriteCell kStealthSprite[] = {
    { 0,-1,'V'}, {-1,0,'<'}, {0,0,'S'}, {1,0,'>'},
};
constexpr int kFighterSpriteCnt = sizeof(kFighterSprite) / sizeof(SpriteCell);
constexpr int kBomberSpriteCnt  = sizeof(kBomberSprite)  / sizeof(SpriteCell);
constexpr int kStealthSpriteCnt = sizeof(kStealthSprite) / sizeof(SpriteCell);
// 敌机：3x2 Normal
inline constexpr SpriteCell kEnemySprite[] = {
    {-1,0,'\\'}, {0,0,'M'}, {1,0,'/'}, {0,1,'v'},
};
// 敌机：Diver 俯冲型 3x2
inline constexpr SpriteCell kDiverSprite[] = {
    { 0,-1,'V'}, {-1,0,'/'}, {0,0,'D'}, {1,0,'\\'},
};
constexpr int kEnemySpriteCnt  = sizeof(kEnemySprite)  / sizeof(SpriteCell);
constexpr int kDiverSpriteCnt  = sizeof(kDiverSprite)  / sizeof(SpriteCell);
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

enum class EnemyType { Normal, Diver };

struct Enemy {
    int x, y, hp = 2;
    bool active = false;
    EnemyType etype = EnemyType::Normal;
    int shoot_timer = 0;
    int speed_counter = 0;
    bool diving = false;
};

struct Player {
    int x = kPlayerStartX, y = kPlayerStartY;
    int lives = 3;
    int shoot_cd = 0;
    PlaneType plane_type = PlaneType::kFighter;
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

struct Wingman {
    int x, y;
    bool active = false;
    int shoot_timer = 0;
    int phase = 0;  // offset for formation
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
    int high_score()   const { return high_score_; }
    int level()        const { return level_; }
    int lives()        const { return player_.lives; }
    int kills()        const { return total_kills_; }
    int combo()        const { return combo_count_; }
    int kills_needed()  const { return 10 + (level_ - 1) * 2; }
    int boss_hp()      const { return boss_.hp; }
    int boss_max_hp()  const { return boss_.max_hp; }
    bool boss_alive()  const { return boss_.active; }
    bool boss_dying()  const { return boss_death_timer_ > 0; }
    int fire_boost()   const { return fire_rate_boost_; }
    int dual_shot()    const { return dual_shot_; }

    PlaneType plane_type()    const { return player_.plane_type; }
    float ultimate_charge()   const { return ultimate_charge_; }
    int   ultimate_duration() const { return ultimate_duration_; }
    int   invincible()        const { return invincible_frames_; }
    int   wingmen_active()    const { return wingmen_count_; }
    bool  shield_up()         const { return shield_active_; }
    int   magnet_left()       const { return magnet_timer_; }
    bool  ultimate_ready()    const { return ultimate_charge_ >= 1.0f; }

    void StartGame();
    void NextLevel();
    void TogglePause();
    void EnterSelect();
    void BackToMenu();
    void SelectPlane(int idx);
    void ActivateUltimate();

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
    void SaveHighScore();

    void SpawnBoss();
    void BossShootCircle();
    void BossShootSpread();
    void BossDie();

    void SpawnPowerUp();
    void SpawnExplosion(int x, int y);
    void UpdateParticles();
    void UpdateWingmen();
    bool IsWingmanCell(int x, int y) const;

    bool IsPlayerCell(int x, int y) const;
    bool IsEnemyCell(const Enemy& e, int x, int y) const;
    bool IsBossCell(int x, int y) const;
    bool IsObstacle(int x, int y) const;

    const SpriteCell* PlayerSprite() const;
    int PlayerSpriteCount() const;

    GameState state_ = GameState::kMenu;
    Player    player_;
    std::vector<Bullet>   bullets_;
    std::vector<Enemy>    enemies_;
    std::vector<PowerUp>  powerups_;
    std::vector<Obstacle>  obstacles_;
    std::vector<Particle> particles_;
    std::vector<Wingman>  wingmen_;
    Boss      boss_;

    int score_ = 0, high_score_ = 0;
    int level_ = 1, total_kills_ = 0;
    int combo_count_ = 0, combo_timer_ = 0;

    int enemy_spawn_timer_    = 0;
    int enemy_speed_          = 12;
    int frame_count_          = 0;
    int obstacle_spawn_timer_ = 80;
    int powerup_spawn_timer_  = 200;
    bool boss_spawned_        = false;
    int  boss_death_timer_    = 0;

    int fire_rate_boost_ = 0;
    int dual_shot_       = 0;
    int wingmen_count_   = 0;
    bool shield_active_  = false;
    int magnet_timer_    = 0;

    PlaneType selected_plane_ = PlaneType::kFighter;
    float ultimate_charge_    = 0.0f;
    int ultimate_duration_    = 0;
    int invincible_frames_    = 0;

    static constexpr float kUltChargePassive = 1.0f / 1800.0f;
    static constexpr float kUltChargePerKill = 1.0f / 12.0f;
    static constexpr int   kUltFighterDur    = 90;
    static constexpr int   kUltBomberDur     = 120;
    static constexpr int   kUltStealthWingmenDur = 300;
    static constexpr int   kWingmanShootInterval  = 15;
    static constexpr int   kWingmanOffsetX = 2;
    static constexpr int   kWingmanOffsetY = 1;
};
