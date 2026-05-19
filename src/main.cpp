#include "game.h"

#include <iomanip>
#include <atomic>
#include <chrono>
#include <mutex>
#include <sstream>
#include <thread>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

using namespace ftxui;

static Game                    g_game;
static std::recursive_mutex    g_mutex;
static std::atomic<bool>       g_quit{false};
static Dir                     g_cur_dir  = Dir::kNone;
static std::atomic<bool>       g_shooting{false};

// ===================================================================
Color CellColor(CellType ct) {
    switch (ct) {
        case CellType::Player:       return Color(Color::Cyan);
        case CellType::Enemy:        return Color(Color::Red);
        case CellType::PlayerBullet: return Color(Color::Yellow);
        case CellType::EnemyBullet:  return Color(Color::RedLight);
        case CellType::Border:       return Color(Color::Blue);
        case CellType::Obstacle:     return Color(Color::Grey50);
        case CellType::PowerUp:      return Color(Color::GreenLight);
        case CellType::Boss:         return Color(Color::Orange1);
        case CellType::Particle:     return Color(Color::Yellow);
        default:                     return Color(Color::Default);
    }
}

// ===================================================================
Element BuildColoredField() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    std::string field = g_game.Render();
    std::vector<Element> lines;
    std::istringstream iss(field);
    std::string line_str;
    int y = -1;

    while (std::getline(iss, line_str)) {
        std::vector<Element> segments;
        char prev_type = -1;
        std::string run;
        int x = -1;

        for (char ch : line_str) {
            char tk;
            if (y < 0 || y >= kFieldHeight || x < 0 || x >= kFieldWidth) {
                tk = 'B';
            } else {
                switch (g_game.GetCellType(x, y)) {
                    case CellType::Player:       tk = 'P'; break;
                    case CellType::Enemy:        tk = 'E'; break;
                    case CellType::PlayerBullet: tk = 'p'; break;
                    case CellType::EnemyBullet:  tk = 'e'; break;
                    case CellType::Obstacle:     tk = 'O'; break;
                    case CellType::PowerUp:      tk = 'U'; break;
                    case CellType::Boss:         tk = 'Z'; break;
                    case CellType::Particle:     tk = 'X'; break;
                    default:                     tk = ' '; break;
                }
            }
            if (tk != prev_type) {
                if (!run.empty()) {
                    Color c = CellColor(prev_type == 'P' ? CellType::Player :
                                        prev_type == 'E' ? CellType::Enemy :
                                        prev_type == 'p' ? CellType::PlayerBullet :
                                        prev_type == 'e' ? CellType::EnemyBullet :
                                        prev_type == 'B' ? CellType::Border :
                                        prev_type == 'O' ? CellType::Obstacle :
                                        prev_type == 'U' ? CellType::PowerUp :
                                        prev_type == 'Z' ? CellType::Boss : prev_type == 'X' ? CellType::Particle : CellType::Empty);
                    segments.push_back(text(run) | color(c));
                    run.clear();
                }
                prev_type = tk;
            }
            run += ch;
            ++x;
        }
        if (!run.empty()) {
            Color c = CellColor(prev_type == 'P' ? CellType::Player :
                                prev_type == 'E' ? CellType::Enemy :
                                prev_type == 'p' ? CellType::PlayerBullet :
                                prev_type == 'e' ? CellType::EnemyBullet :
                                prev_type == 'B' ? CellType::Border :
                                prev_type == 'O' ? CellType::Obstacle :
                                prev_type == 'U' ? CellType::PowerUp :
                                prev_type == 'Z' ? CellType::Boss : prev_type == 'X' ? CellType::Particle : CellType::Empty);
            segments.push_back(text(run) | color(c));
        }
        lines.push_back(hbox(segments));
        ++y;
    }
    return vbox(lines);
}

// ===================================================================
// 侧边栏
// ===================================================================
Element BuildSidePanel() {
    std::vector<Element> rows;

    auto header = [&](const std::string& title, Color c) {
        return hbox({ text("  "), text(title) | bold | color(c) });
    };
    auto row = [&](const std::string& key, const std::string& val, Color vc = Color::White) {
        return hbox({
            text("  ") | size(WIDTH, EQUAL, 2),
            text(key) | color(Color::Grey50) | size(WIDTH, EQUAL, 12),
            text(val) | color(vc) | bold,
        });
    };
    auto sep_line = [] { return text("  ──────────────────") | color(Color::Grey30); };

    rows.push_back(text(""));
    rows.push_back(header("STATS", Color::Cyan));
    rows.push_back(sep_line());

    {
        std::ostringstream lv; lv << g_game.level();
        rows.push_back(row("Level:", lv.str(), Color::Yellow));
    }
    {
        std::ostringstream sc; sc << g_game.score();
        rows.push_back(row("Score:", sc.str(), Color::Yellow));
    }
    {
        std::ostringstream hp;
        for (int i = 0; i < g_game.lives(); ++i) hp << "\xE2\x99\xA5 ";
        for (int i = g_game.lives(); i < kMaxLives; ++i) hp << "\xE2\x97\x8B ";
        rows.push_back(row("Lives:", hp.str(), Color::Red));
    }
    {
        std::ostringstream kl;
        kl << g_game.kills() << " / " << kKillsPerLevel;
        rows.push_back(row("Kills:", kl.str(), Color::White));
    }

    // 道具效果
    bool has_fx = (g_game.fire_boost() > 0 || g_game.dual_shot() > 0);
    if (has_fx) {
        rows.push_back(text(""));
        rows.push_back(header("EFFECTS", Color::Green));
        rows.push_back(sep_line());

        if (g_game.fire_boost() > 0) {
            int sec = g_game.fire_boost() / 30;
            std::ostringstream ss; ss << sec << "s";
            rows.push_back(row("~ Fire Rate:", ss.str(), Color::GreenLight));
        }
        if (g_game.dual_shot() > 0) {
            int sec = g_game.dual_shot() / 30;
            std::ostringstream ss; ss << sec << "s";
            rows.push_back(row("= Dual Shot:", ss.str(), Color::GreenLight));
        }
    }

    // Boss 血条
    if (g_game.boss_alive()) {
        rows.push_back(text(""));
        rows.push_back(header("BOSS", Color::Orange1));
        rows.push_back(sep_line());

        int hp = g_game.boss_hp(), mx = g_game.boss_max_hp();
        int w = 14, bars = mx > 0 ? hp * w / mx : 0;
        std::string bar;
        for (int i = 0; i < w; ++i) bar += (i < bars) ? '\xDB' : '\xB0';
        std::ostringstream hpstr;
        hpstr << hp << "/" << mx;
        rows.push_back(row("HP:", bar + " " + hpstr.str(), Color::Orange1));
    }

    // 道具图例
    rows.push_back(text(""));
    rows.push_back(header("ITEMS", Color::Grey50));
    rows.push_back(sep_line());
    rows.push_back(row("+", "Health  +1", Color::Red));
    rows.push_back(row("~", "Fire Rate", Color::GreenLight));
    rows.push_back(row("=", "Dual Shot", Color::Yellow));

    return vbox(rows) | size(WIDTH, GREATER_THAN, 24);
}

// ===================================================================
Element BuildMenuScreen() {
    auto logo = vbox({
        text(R"(   ____  _               __      __        ___  )") | color(Color::Cyan) | bold,
        text(R"(  / __ \/ /___ _____     / /___ _/ /_____  / _ | )") | color(Color::Cyan) | bold,
        text(R"( / /_/ / / __ `/ __ \   / / __ `/ __/ __ \/ __ | )") | color(Color::Cyan),
        text(R"( / ____/ / /_/ / / / /  / / /_/ / /_/ /_/ / /_/ | )") | color(Color::Cyan),
        text(R"(/_/   /_/\__,_/_/ /_/  /_/\__,_/\__/\____/____/  )") | color(Color::Cyan),
    }) | center;

    auto title = text("  >>  PLANE WAR  <<  ") | bold | color(Color::Yellow) | center;
    auto hint1 = text("  Press ENTER to start  ") | dim | center;
    auto hint2 = text("  Press Q to quit  ") | dim | center;
    auto sep   = text("  ───────────────────────────────────────  ") | color(Color::Blue) | center;
    auto ctrl  = text("  WASD / Arrow : Move    Space : Shoot  ") | center;
    auto ctrl2 = text("  P : Pause    R : Restart  ") | center;
    auto items = text("  Collect  +Health  ~FireRate  =DualShot  ") | center;

    auto credit = text("MADE BY 沈皓然") | color(Color::Grey50) | dim;
    auto credit_row = hbox({ filler(), credit });

    return vbox({ text(""), logo, text(""), title, text(""),
                  sep, text(""), hint1, hint2, text(""), ctrl, ctrl2, text(""), items,
                  text(""), credit_row }) | center;
}

Element BuildPlayingScreen() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    std::ostringstream info;
    info << "  Level " << g_game.level()
         << "  |  Score " << g_game.score()
         << "  |  Kills " << g_game.kills() << "/" << kKillsPerLevel;

    auto info_bar = text(info.str()) | bold | color(Color::Yellow) | center;

    // Boss 血条（游戏区上方）
    Element boss_bar = text("");
    if (g_game.boss_alive()) {
        int hp = g_game.boss_hp(), mx = g_game.boss_max_hp();
        int w = 28, bars = mx > 0 ? hp * w / mx : 0;
        std::string bar;
        for (int i = 0; i < w; ++i) bar += (i < bars) ? '\xDB' : '\xB0';
        std::ostringstream ss;
        ss << "  BOSS  [" << bar << "]  " << hp << "/" << mx;
        boss_bar = text(ss.str()) | color(Color::Orange1) | bold | center;
    }

    auto field    = BuildColoredField();
    auto side     = BuildSidePanel();

    auto main = vbox({ info_bar, text(""), boss_bar, text(""), field }) | center;
    return hbox({ main, separator() | color(Color::Blue), side }) | center;
}

Element BuildPausedScreen() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    std::ostringstream info;
    info << "  Level " << g_game.level() << "  |  Score " << g_game.score();

    auto info_bar = text(info.str()) | bold | color(Color::Yellow) | center;
    auto field    = BuildColoredField();
    auto paused   = text("  ══  PAUSED  ══  ") | bold | color(Color::Yellow) | center | border;
    auto hint     = text("  P : Resume    Q : Quit  ") | dim | center;

    auto main = vbox({ info_bar, text(""), field, text(""), paused, hint });
    auto side = BuildSidePanel();
    return hbox({ main | center, separator() | color(Color::Blue), side }) | center;
}

Element BuildGameOverScreen() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    // 爆炸艺术字
    auto boom = vbox({
        text(R"(    *  .  *    .   *  .  )") | color(Color::Red) | bold,
        text(R"(  .   *    *   .  *    .)") | color(Color::Orange1),
        text(R"(    *  .   *    .  *  . )") | color(Color::Yellow),
    }) | center;

    auto over1 = text("  GAME OVER  ") | bold | color(Color::Red) | center;
    auto over2 = text("  MISSION FAILED  ") | color(Color::RedLight) | center;

    std::ostringstream ss;
    ss << "  Score: " << g_game.score() << "    Level: " << g_game.level()
       << "    Kills: " << g_game.kills();
    auto stats = text(ss.str()) | color(Color::Yellow) | bold | center;

    auto sep   = text("  ───────────────────────────────────────  ") | color(Color::Grey30) | center;
    auto hint  = text("  [R] Retry    [Q] Quit  ") | color(Color::White) | bold | center;

    return vbox({ text(""), boom, text(""), over1, over2, text(""),
                  stats, text(""), sep, text(""), hint }) | center;
}

Element BuildLevelClearScreen() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    auto stars = vbox({
        text(R"(   *   .   *   .   *  )") | color(Color::Yellow),
        text(R"( .   *   .   *   .   *)") | color(Color::GreenLight),
        text(R"(   *   .   *   .   *  )") | color(Color::Yellow),
    }) | center;

    std::ostringstream ss;
    ss << "  LEVEL " << g_game.level() << " CLEAR!  ";
    auto title = text(ss.str()) | bold | color(Color::Green) | center | borderDouble;

    std::ostringstream ss2;
    ss2 << "  Score: " << g_game.score() << "    Boss Defeated!";
    auto score_line = text(ss2.str()) | color(Color::Yellow) | bold | center;

    auto bonus = text("  +" + std::to_string(kBossScoreBonus * g_game.level()) + " BONUS") | color(Color::Orange1) | center;

    auto hint = text("  [ENTER] Next Level    [R] Restart  ") | bold | center;

    return vbox({ text(""), stars, text(""), title, text(""),
                  score_line, text(""), bonus, text(""), hint }) | center;
}

Element BuildUI() {
    GameState st;
    {
        std::lock_guard<std::recursive_mutex> lock(g_mutex);
        st = g_game.state();
    }
    switch (st) {
        case GameState::kMenu:       return BuildMenuScreen();
        case GameState::kPlaying:    return BuildPlayingScreen();
        case GameState::kPaused:     return BuildPausedScreen();
        case GameState::kGameOver:   return BuildGameOverScreen();
        case GameState::kLevelClear: return BuildLevelClearScreen();
    }
    return text("Unknown state");
}

// ===================================================================
int main() {
    auto screen = ScreenInteractive::Fullscreen();

    auto renderer  = Renderer([&] { return BuildUI(); });
    auto component = CatchEvent(renderer, [&](Event event) -> bool {
        GameState st;
        {
            std::lock_guard<std::recursive_mutex> lock(g_mutex);
            st = g_game.state();
        }

        if (event == Event::Character('q') || event == Event::Character('Q')) {
            g_quit = true; screen.ExitLoopClosure()(); return true;
        }
        if (st == GameState::kMenu) {
            if (event == Event::Return) {
                { std::lock_guard<std::recursive_mutex> lk(g_mutex); g_game.StartGame(); }
                g_cur_dir = Dir::kNone; g_shooting = false; return true;
            }
            return false;
        }
        if (st == GameState::kGameOver || st == GameState::kLevelClear) {
            if (event == Event::Character('r') || event == Event::Character('R')) {
                { std::lock_guard<std::recursive_mutex> lk(g_mutex); g_game.StartGame(); }
                g_cur_dir = Dir::kNone; g_shooting = false; return true;
            }
            if (event == Event::Return && st == GameState::kLevelClear) {
                { std::lock_guard<std::recursive_mutex> lk(g_mutex); g_game.NextLevel(); }
                g_cur_dir = Dir::kNone; g_shooting = false; return true;
            }
            return false;
        }
        if (event == Event::Character('p') || event == Event::Character('P')) {
            std::lock_guard<std::recursive_mutex> lk(g_mutex); g_game.TogglePause(); return true;
        }
        if (st != GameState::kPlaying) return false;
        if (event == Event::Custom) return false;

        if (event == Event::Character('w') || event == Event::Character('W') || event == Event::ArrowUp)
            g_cur_dir = Dir::kUp;
        else if (event == Event::Character('s') || event == Event::Character('S') || event == Event::ArrowDown)
            g_cur_dir = Dir::kDown;
        else if (event == Event::Character('a') || event == Event::Character('A') || event == Event::ArrowLeft)
            g_cur_dir = Dir::kLeft;
        else if (event == Event::Character('d') || event == Event::Character('D') || event == Event::ArrowRight)
            g_cur_dir = Dir::kRight;

        if (event == Event::Character(' ')) g_shooting = true;
        else if (event.is_character()) g_shooting = false;
        return true;
    });

    std::thread game_thread([&] {
        using namespace std::chrono;
        constexpr int kFPS = 30;
        constexpr auto kFrame = milliseconds(1000 / kFPS);
        while (!g_quit.load()) {
            auto t0 = steady_clock::now();
            {
                std::lock_guard<std::recursive_mutex> lk(g_mutex);
                g_game.Update(g_cur_dir, g_shooting);
            }
            screen.Post(Event::Custom);
            auto elapsed = steady_clock::now() - t0;
            if (elapsed < kFrame) std::this_thread::sleep_for(kFrame - elapsed);
        }
    });

    screen.Loop(component);
    g_quit = true;
    if (game_thread.joinable()) game_thread.join();
    return 0;
}
