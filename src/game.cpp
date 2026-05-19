#include "game.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Game::Game() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    bullets_.reserve(kMaxPlayerBullets + kMaxEnemyBullets + 60);
    enemies_.reserve(kMaxEnemies);
    powerups_.reserve(kMaxPowerUps);
    obstacles_.reserve(kMaxObstacles);
}

// ===================================================================
void Game::StartGame() {
    player_          = Player{};
    bullets_.clear();
    enemies_.clear();
    powerups_.clear();
    obstacles_.clear();
    particles_.clear();
    boss_            = Boss{};
    score_           = 0;
    level_           = 1;
    total_kills_     = 0;
    enemy_spawn_timer_    = 0;
    enemy_speed_          = 12;
    obstacle_spawn_timer_ = 80;
    powerup_spawn_timer_  = 200;
    boss_spawned_   = false;
    fire_rate_boost_ = 0;
    dual_shot_       = 0;
    state_ = GameState::kPlaying;
}

void Game::NextLevel() {
    ++level_;
    player_.lives      = kMaxLives;
    total_kills_       = 0;
    bullets_.clear();
    enemies_.clear();
    powerups_.clear();
    obstacles_.clear();
    particles_.clear();
    boss_              = Boss{};
    enemy_spawn_timer_ = 0;
    enemy_speed_       = std::max(3, 12 - (level_ - 1) * 2);
    obstacle_spawn_timer_ = std::max(40, 80 - level_ * 5);
    powerup_spawn_timer_  = std::max(100, kEnemySpawnInterval * 4 - level_ * 25);
    boss_spawned_      = false;
    fire_rate_boost_   = 0;
    dual_shot_         = 0;
    state_ = GameState::kPlaying;
}

void Game::TogglePause() {
    if (state_ == GameState::kPlaying) state_ = GameState::kPaused;
    else if (state_ == GameState::kPaused) state_ = GameState::kPlaying;
}

// ===================================================================
bool Game::IsPlayerCell(int x, int y) const {
    for (auto& c : kPlayerSprite)
        if (player_.x + c.dx == x && player_.y + c.dy == y) return true;
    return false;
}
bool Game::IsEnemyCell(const Enemy& e, int x, int y) const {
    for (auto& c : kEnemySprite)
        if (e.x + c.dx == x && e.y + c.dy == y) return true;
    return false;
}
bool Game::IsBossCell(int x, int y) const {
    if (!boss_.active) return false;
    for (auto& c : kBossSprite)
        if (boss_.x + c.dx == x && boss_.y + c.dy == y) return true;
    return false;
}
bool Game::IsObstacle(int x, int y) const {
    for (auto& o : obstacles_)
        if (x >= o.x && x < o.x + 2 && y >= o.y && y < o.y + 2) return true;
    return false;
}

CellType Game::GetCellType(int x, int y) const {
    if (x < 0 || x >= kFieldWidth || y < 0 || y >= kFieldHeight) return CellType::Empty;
    if (state_ != GameState::kMenu && player_.lives > 0 && IsPlayerCell(x, y))
        return CellType::Player;
    if (boss_.active && IsBossCell(x, y)) return CellType::Boss;
    for (auto& e : enemies_)
        if (e.active && IsEnemyCell(e, x, y)) return CellType::Enemy;
    for (auto& b : bullets_) {
        if (!b.active || b.x != x || b.y != y) continue;
        return b.from_player ? CellType::PlayerBullet : CellType::EnemyBullet;
    }
    for (auto& p : powerups_)
        if (p.active && p.x == x && p.y == y) return CellType::PowerUp;
    for (auto& p : particles_)
        if (p.x == x && p.y == y) return CellType::Particle;
    if (IsObstacle(x, y)) return CellType::Obstacle;
    return CellType::Empty;
}

// ===================================================================
void Game::SpawnObstacle() {
    if (static_cast<int>(obstacles_.size()) >= kMaxObstacles) return;
    int ox = 2 + rand() % (kFieldWidth - 6);
    obstacles_.push_back({ox, 0, 0});
}

void Game::SpawnPowerUp() {
    if (static_cast<int>(powerups_.size()) >= kMaxPowerUps) return;
    int r = rand() % 10;
    PowerUpType t;
    if (r < 4)      t = PowerUpType::Health;
    else if (r < 7) t = PowerUpType::FireRate;
    else            t = PowerUpType::DualShot;
    int px = 2 + rand() % (kFieldWidth - 6);
    powerups_.push_back({px, 0, t, true, 0});
}

void Game::CheckPowerUpCollect() {
    for (auto& p : powerups_) {
        if (!p.active) continue;
        bool hit = false;
        for (auto& c : kPlayerSprite)
            if (player_.x + c.dx == p.x && player_.y + c.dy == p.y) { hit = true; break; }
        if (hit) {
            p.active = false;
            switch (p.type) {
                case PowerUpType::Health:
                    if (player_.lives < kMaxLives) ++player_.lives;
                    break;
                case PowerUpType::FireRate:
                    fire_rate_boost_ = 600;
                    break;
                case PowerUpType::DualShot:
                    dual_shot_ = 750;
                    break;
            }
        }
    }
}

// ===================================================================
void Game::SpawnExplosion(int x, int y) {
    const char chars[] = {'*', '+', '.', 'x', ':'};
    for (int i = 0; i < 8; ++i) {
        int dx = (rand() % 3) - 1;
        int dy = (rand() % 3) - 1;
        int life = 6 + rand() % 10;
        char ch = chars[rand() % 5];
        particles_.push_back({x, y, dx, dy, life, ch});
    }
}

void Game::UpdateParticles() {
    for (auto& p : particles_) {
        p.x += p.dx;
        p.y += p.dy;
        --p.life;
    }
    // 移除过期粒子
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
                       [](const Particle& p) { return p.life <= 0; }),
        particles_.end());
}

// ===================================================================
void Game::MovePowerUps() {
    for (auto& p : powerups_) {
        if (!p.active) continue;
        ++p.speed_counter;
        if (p.speed_counter >= 8) { p.speed_counter = 0; ++p.y; }
        if (p.y >= kFieldHeight) p.active = false;
    }
}

// 障碍物：从顶部掉落，速度与敌机一致，不伤玩家
void Game::MoveObstacles() {
    for (auto& o : obstacles_) {
        ++o.speed_counter;
        if (o.speed_counter >= enemy_speed_) {
            o.speed_counter = 0;
            ++o.y;
        }
        if (o.y >= kFieldHeight) {
            // 移除越界的障碍物，在顶部重新生成
            o.y = 0;
            o.x = 2 + rand() % (kFieldWidth - 6);
            o.speed_counter = 0;
        }
    }
}

// ===================================================================
void Game::SpawnBoss() {
    boss_.x        = kFieldWidth / 2;
    boss_.y        = 2;
    boss_.hp       = 18 + level_ * 10;
    boss_.max_hp   = boss_.hp;
    boss_.active   = true;
    boss_.move_dir = 1;
    boss_.shoot_timer   = 35;
    boss_.pattern_timer = 0;
}

void Game::BossShootCircle() {
    for (int i = 0; i < 16; ++i) {
        double angle = i * 2.0 * M_PI / 16.0;
        int dx = static_cast<int>(std::round(std::cos(angle)));
        int dy = static_cast<int>(std::round(std::sin(angle)));
        if (dx == 0 && dy == 0) continue;
        for (auto& b : bullets_)
            if (!b.active) { b = Bullet{boss_.x, boss_.y, dx, dy, true, false}; goto next_b; }
        if (static_cast<int>(bullets_.size()) < kMaxPlayerBullets + kMaxEnemyBullets + 30)
            bullets_.push_back(Bullet{boss_.x, boss_.y, dx, dy, true, false});
        next_b:;
    }
}

void Game::BossShootSpread() {
    int tx = player_.x - boss_.x;
    int ty = player_.y - boss_.y;
    double base = std::atan2(ty, tx);
    for (int i = -1; i <= 1; ++i) {
        double a = base + i * 0.25;
        int dx = static_cast<int>(std::round(std::cos(a)));
        int dy = static_cast<int>(std::round(std::sin(a)));
        for (auto& b : bullets_)
            if (!b.active) { b = Bullet{boss_.x, boss_.y, dx, dy, true, false}; goto next_s; }
        if (static_cast<int>(bullets_.size()) < kMaxPlayerBullets + kMaxEnemyBullets + 30)
            bullets_.push_back(Bullet{boss_.x, boss_.y, dx, dy, true, false});
        next_s:;
    }
}

void Game::BossDie() {
    boss_.active = false;
    // 大爆炸特效
    for (auto& c : kBossSprite)
        SpawnExplosion(boss_.x + c.dx, boss_.y + c.dy);
    score_ += kBossScoreBonus * level_;
    for (auto& b : bullets_)
        if (!b.from_player) b.active = false;
    state_ = GameState::kLevelClear;
}

void Game::MoveBoss() {
    if (!boss_.active) return;
    boss_.x += boss_.move_dir;
    if (boss_.x <= 3) boss_.move_dir = 1;
    if (boss_.x >= kFieldWidth - 3) boss_.move_dir = -1;

    if (boss_.shoot_timer > 0) --boss_.shoot_timer;
    if (boss_.shoot_timer <= 0) {
        ++boss_.pattern_timer;
        if (boss_.pattern_timer % 3 == 0) BossShootCircle();
        else BossShootSpread();
        boss_.shoot_timer = std::max(12, 35 - level_ * 2);
    }
}

// ===================================================================
void Game::Update(Dir move_dir, bool shooting) {
    if (state_ != GameState::kPlaying) return;
    ++frame_count_;

    if (fire_rate_boost_ > 0) --fire_rate_boost_;
    if (dual_shot_ > 0) --dual_shot_;

    MovePlayer(move_dir);

    int cd = (fire_rate_boost_ > 0) ? 3 : kPlayerShootCooldown;
    if (shooting && player_.shoot_cd <= 0) {
        FirePlayerBullet();
        player_.shoot_cd = cd;
    }
    if (player_.shoot_cd > 0) --player_.shoot_cd;

    MoveBullets();
    MovePowerUps();
    MoveObstacles();
    CheckPowerUpCollect();
    UpdateParticles();

    if (!boss_.active) {
        if (enemy_spawn_timer_ <= 0) {
            SpawnEnemy();
            enemy_spawn_timer_ = std::max(10, kEnemySpawnInterval - level_ * 4);
        } else --enemy_spawn_timer_;

        if (obstacle_spawn_timer_ <= 0) {
            SpawnObstacle();
            obstacle_spawn_timer_ = std::max(60, kObstacleSpawnBase - level_ * 10);
        } else --obstacle_spawn_timer_;

        if (powerup_spawn_timer_ > 0) --powerup_spawn_timer_;
        if (powerup_spawn_timer_ <= 0) {
            SpawnPowerUp();
            powerup_spawn_timer_ = std::max(150, kEnemySpawnInterval * 4 - level_ * 25);
        }
    } else {
        MoveBoss();
    }

    MoveEnemies();
    CheckCollisions();
    CheckLevelProgress();
}

// ===================================================================
void Game::MovePlayer(Dir d) {
    int nx = player_.x, ny = player_.y;
    switch (d) {
        case Dir::kUp: --ny; break;
        case Dir::kDown: ++ny; break;
        case Dir::kLeft: --nx; break;
        case Dir::kRight: ++nx; break;
        default: return;
    }
    if (nx >= 1 && nx < kFieldWidth - 1 && ny >= 1 && ny < kFieldHeight) {
        bool blocked = false;
        for (auto& c : kPlayerSprite)
            if (IsObstacle(nx + c.dx, ny + c.dy)) { blocked = true; break; }
        if (!blocked) { player_.x = nx; player_.y = ny; }
    }
}

void Game::FirePlayerBullet() {
    auto spawn_at = [&](int bx) {
        for (auto& b : bullets_)
            if (!b.active) { b = Bullet{bx, player_.y - 2, 0, -1, true, true}; return; }
        if (static_cast<int>(bullets_.size()) < kMaxPlayerBullets + kMaxEnemyBullets + 30)
            bullets_.push_back(Bullet{bx, player_.y - 2, 0, -1, true, true});
    };
    if (dual_shot_ > 0) { spawn_at(player_.x - 1); spawn_at(player_.x + 1); }
    spawn_at(player_.x);
}

void Game::SpawnEnemy() {
    int cnt = 0;
    for (auto& e : enemies_) if (e.active) ++cnt;
    if (cnt >= kMaxEnemies) return;

    int hp = (level_ >= 5) ? 3 : ((level_ >= 3) ? 2 : 1);
    int x = 1 + (frame_count_ * 7 + rand() % 11) % (kFieldWidth - 2);

    for (auto& e : enemies_) {
        if (!e.active) { e = Enemy{x, -1, hp, true, kEnemyShootInterval + rand() % 20, 0}; return; }
    }
    if (static_cast<int>(enemies_.size()) < kMaxEnemies)
        enemies_.push_back(Enemy{x, -1, hp, true, kEnemyShootInterval + rand() % 20, 0});
}

void Game::SpawnEnemyBullet(const Enemy& e) {
    for (auto& b : bullets_)
        if (!b.active) { b = Bullet{e.x, e.y + 1, 0, 1, true, false}; return; }
    if (static_cast<int>(bullets_.size()) < kMaxPlayerBullets + kMaxEnemyBullets + 30)
        bullets_.push_back(Bullet{e.x, e.y + 1, 0, 1, true, false});
}

void Game::MoveBullets() {
    for (auto& b : bullets_) {
        if (!b.active) continue;
        if (!b.from_player && (frame_count_ & 1)) continue;
        b.x += b.dx; b.y += b.dy;
        if (b.x < 0 || b.x >= kFieldWidth || b.y < 0 || b.y >= kFieldHeight)
            { b.active = false; continue; }
        if (IsObstacle(b.x, b.y)) b.active = false;
    }
}

void Game::MoveEnemies() {
    for (auto& e : enemies_) {
        if (!e.active) continue;
        if (e.shoot_timer > 0) --e.shoot_timer;
        if (e.shoot_timer <= 0 && e.y > 1 && e.y < kFieldHeight - 4) {
            SpawnEnemyBullet(e);
            e.shoot_timer = std::max(15, kEnemyShootInterval - level_ * 3);
        }
        ++e.speed_counter;
        if (e.speed_counter >= enemy_speed_) { e.speed_counter = 0; ++e.y; }
        if (e.y >= kFieldHeight - 1) {
            e.active = false;
            if (--player_.lives <= 0) {
                state_ = GameState::kGameOver;
                if (score_ > high_score_) high_score_ = score_;
                return;
            }
        }
    }
}

// ===================================================================
void Game::CheckCollisions() {
    // 玩家子弹 vs 敌机
    for (auto& b : bullets_) {
        if (!b.active || !b.from_player) continue;
        for (auto& e : enemies_) {
            if (!e.active) continue;
            for (auto& c : kEnemySprite)
                if (b.x == e.x + c.dx && b.y == e.y + c.dy) { b.active = false; goto hit; }
            continue;
        hit:
            if (--e.hp <= 0) { e.active = false; score_ += kScorePerKill * level_; ++total_kills_; SpawnExplosion(e.x, e.y); }
        }
    }
    // 玩家子弹 vs Boss
    for (auto& b : bullets_) {
        if (!b.active || !b.from_player || !boss_.active) continue;
        if (IsBossCell(b.x, b.y)) { b.active = false; --boss_.hp; if (boss_.hp <= 0) { BossDie(); return; } }
    }
    // 敌方子弹 vs 玩家（含 Boss 弹幕）
    for (auto& b : bullets_) {
        if (!b.active || b.from_player) continue;
        if (IsPlayerCell(b.x, b.y)) {
            b.active = false;
            if (--player_.lives <= 0) { state_ = GameState::kGameOver; if (score_ > high_score_) high_score_ = score_; return; }
        }
    }
    // 敌机撞玩家
    for (auto& e : enemies_) {
        if (!e.active) continue;
        for (auto& c : kEnemySprite)
            if (IsPlayerCell(e.x + c.dx, e.y + c.dy)) {
                e.active = false;
                if (--player_.lives <= 0) { state_ = GameState::kGameOver; if (score_ > high_score_) high_score_ = score_; return; }
                break;
            }
    }
    // Boss 撞玩家
    if (boss_.active) {
        for (auto& c : kBossSprite)
            if (IsPlayerCell(boss_.x + c.dx, boss_.y + c.dy)) {
                if (--player_.lives <= 0) { state_ = GameState::kGameOver; if (score_ > high_score_) high_score_ = score_; return; }
                break;
            }
    }
}

void Game::CheckLevelProgress() {
    if (total_kills_ >= kKillsPerLevel && !boss_spawned_) {
        boss_spawned_ = true;
        for (auto& e : enemies_) e.active = false;
        for (auto& b : bullets_) if (!b.from_player) b.active = false;
        SpawnBoss();
    }
}

// ===================================================================
std::string Game::Render() const {
    char grid[kFieldHeight][kFieldWidth + 1];
    for (int y = 0; y < kFieldHeight; ++y) {
        for (int x = 0; x < kFieldWidth; ++x) grid[y][x] = ' ';
        grid[y][kFieldWidth] = '\0';
    }

    // 障碍物（先画，让敌机盖在上面）
    for (auto& o : obstacles_) {
        for (int dy = 0; dy < 2; ++dy)
            for (int dx = 0; dx < 2; ++dx) {
                int cx = o.x + dx, cy = o.y + dy;
                if (cx >= 0 && cx < kFieldWidth && cy >= 0 && cy < kFieldHeight)
                    grid[cy][cx] = '#';
            }
    }

    // 道具
    for (auto& p : powerups_) {
        if (!p.active) continue;
        if (p.x >= 0 && p.x < kFieldWidth && p.y >= 0 && p.y < kFieldHeight) {
            switch (p.type) {
                case PowerUpType::Health:   grid[p.y][p.x] = '+'; break;
                case PowerUpType::FireRate: grid[p.y][p.x] = '~'; break;
                case PowerUpType::DualShot: grid[p.y][p.x] = '='; break;
            }
        }
    }

    // 子弹
    for (auto& b : bullets_) {
        if (!b.active) continue;
        if (b.x >= 0 && b.x < kFieldWidth && b.y >= 0 && b.y < kFieldHeight) {
            if (grid[b.y][b.x] == ' ' || grid[b.y][b.x] == '#') {
                char ch;
                if (b.from_player && b.dy < 0)      ch = '|';
                else if (!b.from_player && b.dy > 0) ch = 'O';
                else                                 ch = '*';
                grid[b.y][b.x] = ch;
            }
        }
    }

    // 爆炸粒子
    for (auto& p : particles_) {
        if (p.x >= 0 && p.x < kFieldWidth && p.y >= 0 && p.y < kFieldHeight)
            if (grid[p.y][p.x] == ' ' || grid[p.y][p.x] == '#')
                grid[p.y][p.x] = p.ch;
    }

    // 敌机（画在障碍物上面）
    for (auto& e : enemies_) {
        if (!e.active) continue;
        for (auto& c : kEnemySprite) {
            int cx = e.x + c.dx, cy = e.y + c.dy;
            if (cx >= 0 && cx < kFieldWidth && cy >= 0 && cy < kFieldHeight)
                grid[cy][cx] = c.ch;
        }
    }

    // Boss
    if (boss_.active) {
        for (auto& c : kBossSprite) {
            int cx = boss_.x + c.dx, cy = boss_.y + c.dy;
            if (cx >= 0 && cx < kFieldWidth && cy >= 0 && cy < kFieldHeight)
                grid[cy][cx] = c.ch;
        }
    }

    // 玩家（最上层）
    if (player_.lives > 0) {
        for (auto& c : kPlayerSprite) {
            int cx = player_.x + c.dx, cy = player_.y + c.dy;
            if (cx >= 0 && cx < kFieldWidth && cy >= 0 && cy < kFieldHeight)
                grid[cy][cx] = c.ch;
        }
    }

    std::ostringstream oss;
    auto border_line = [&]() {
        oss << '+';
        for (int x = 0; x < kFieldWidth; ++x) oss << '~';
        oss << "+\n";
    };
    border_line();
    for (int y = 0; y < kFieldHeight; ++y)
        oss << ':' << grid[y] << ":\n";
    border_line();
    return oss.str();
}
