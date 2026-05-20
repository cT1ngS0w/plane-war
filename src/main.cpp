#include "game.h"

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
static std::atomic<int>        g_anim_frame{0};
static int                     g_select_idx = 0;

// ===================================================================
Color CellColor(CellType ct) {
    switch (ct) {
        case CellType::Player: {
            PlaneType pt;
            int inv;
            {
                std::lock_guard<std::recursive_mutex> lk(g_mutex);
                pt = g_game.plane_type();
                inv = g_game.invincible();
            }
            if (inv > 0 && ((g_anim_frame.load() / 4) & 1))
                return Color(Color::White);
            switch (pt) {
                case PlaneType::kFighter: return Color(Color::Cyan);
                case PlaneType::kBomber:  return Color(Color::Orange1);
                case PlaneType::kStealth: return Color(Color::Magenta);
            }
            break;
        }
        case CellType::Enemy:        return Color(Color::Red);
        case CellType::PlayerBullet: return Color(Color::Yellow);
        case CellType::EnemyBullet:  return Color(Color::RedLight);
        case CellType::Border:       return Color(Color::Blue);
        case CellType::Obstacle:     return Color(Color::Grey50);
        case CellType::PowerUp:      return Color(Color::GreenLight);
        case CellType::Boss:         return Color(Color::Orange1);
        case CellType::Particle:     return Color(Color::Yellow);
        case CellType::Wingman:      return Color(Color::MagentaLight);
        default:                     return Color(Color::Default);
    }
    return Color(Color::Default);
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
                    case CellType::Wingman:      tk = 'W'; break;
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
                                        prev_type == 'Z' ? CellType::Boss : prev_type == 'X' ? CellType::Particle : prev_type == 'W' ? CellType::Wingman : CellType::Empty);
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
                                prev_type == 'Z' ? CellType::Boss : prev_type == 'X' ? CellType::Particle : prev_type == 'W' ? CellType::Wingman : CellType::Empty);
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
        kl << g_game.kills() << " / " << g_game.kills_needed();
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
    if (g_game.boss_alive() || g_game.boss_dying()) {
        rows.push_back(text(""));
        rows.push_back(header("BOSS", Color::Orange1));
        rows.push_back(sep_line());

        if (g_game.boss_alive()) {
            int hp = g_game.boss_hp(), mx = g_game.boss_max_hp();
            int w = 14, bars = mx > 0 ? hp * w / mx : 0;
            std::string bar;
            for (int i = 0; i < w; ++i) bar += (i < bars) ? '\xDB' : '\xB0';
            std::ostringstream hpstr;
            hpstr << hp << "/" << mx;
            rows.push_back(row("HP:", bar + " " + hpstr.str(), Color::Orange1));
        } else {
            rows.push_back(row("", "DEFEATED!", Color::Yellow));
        }
    }

    // 终极技能
    {
        float charge = g_game.ultimate_charge();
        int ult_dur = g_game.ultimate_duration();
        int inv = g_game.invincible();
        rows.push_back(text(""));
        rows.push_back(header("ULTIMATE", Color::Magenta));
        rows.push_back(sep_line());

        const char* skill_name = "???";
        switch (g_game.plane_type()) {
            case PlaneType::kFighter: skill_name = "Bullet Storm"; break;
            case PlaneType::kBomber:  skill_name = "Iron Shield"; break;
            case PlaneType::kStealth: skill_name = "Wingmen"; break;
        }
        rows.push_back(row("Skill:", skill_name, Color::MagentaLight));

        if (ult_dur > 0 || inv > 0) {
            int remaining = ult_dur > 0 ? ult_dur : inv;
            int sec = remaining / 30;
            std::ostringstream ss; ss << sec << "s";
            rows.push_back(row("Active:", ss.str(), Color::Yellow));
            if (g_game.wingmen_active() > 0) {
                std::ostringstream wss; wss << "x" << g_game.wingmen_active();
                rows.push_back(row("Wingmen:", wss.str(), Color::MagentaLight));
            }
        } else if (charge >= 1.0f) {
            rows.push_back(row("", "[E] ACTIVATE!", Color::Yellow));
        } else {
            int w = 14;
            int bars = static_cast<int>(charge * w);
            std::string bar;
            for (int i = 0; i < w; ++i) bar += (i < bars) ? '\xDB' : '\xB0';
            std::ostringstream ss;
            ss << bar << " " << static_cast<int>(charge * 100) << "%";
            rows.push_back(row("Charge:", ss.str(), Color::Magenta));
        }
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
    static const std::vector<Color> kWavePalette = {
        Color::Blue,       Color::Cyan,        Color::CyanLight,
        Color::GreenLight, Color::Yellow,      Color::Orange1,
        Color::RedLight,   Color::Magenta,     Color::MagentaLight,
        Color::BlueLight,  Color::Green,       Color::YellowLight,
    };
    int frame = g_anim_frame.load(std::memory_order_relaxed);
    int speed = frame / 12;

    auto logo_line = [&](int idx, const std::string& s, bool bld) {
        int ci = (idx * 3 + speed) % static_cast<int>(kWavePalette.size());
        Element e = text(s) | color(kWavePalette[ci]);
        if (bld) e = e | bold;
        return e;
    };

    auto logo = vbox({
        logo_line(0, R"(   ____  _               __      __        ___  )", true),
        logo_line(1, R"(  / __ \/ /___ _____     / /___ _/ /_____  / _ | )", true),
        logo_line(2, R"( / /_/ / / __ `/ __ \   / / __ `/ __/ __ \/ __ | )", false),
        logo_line(3, R"( / ____/ / /_/ / / / /  / / /_/ / /_/ /_/ / /_/ | )", false),
        logo_line(4, R"(/_/   /_/\__,_/_/ /_/  /_/\__,_/\__/\____/____/  )", false),
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

Element BuildSelectScreen() {
    Color themes[3] = {Color::Cyan, Color::Orange1, Color::Magenta};

    auto arrow_r = [](bool sel) { return std::string(sel ? " \xE2\x96\xB6" : "  "); };
    auto arrow_l = [](bool sel) { return std::string(sel ? "\xE2\x97\x80 " : "  "); };

    std::vector<Element> rows;
    rows.push_back(text(""));
    rows.push_back(text("  >>  SELECT YOUR PLANE  <<") | bold | color(Color::Yellow));
    rows.push_back(text(""));
    rows.push_back(text("  W / S : switch    ENTER : confirm    Q : back") | dim);
    rows.push_back(text(""));
    rows.push_back(text(""));

    for (int i = 0; i < 3; ++i) {
        const char* sprites_top[3] = {"  ^  ", "[#B#]", "  V  "};
        const char* sprites_bot[3] = {" /F\\ ", "/===\\", " <S> "};
        const char* names[3] = {"Valkyrie", "Fortress", "Phantom"};
        const char* cnames[3] = {"\xE8\xBF\x85\xE9\xA3\x8E\xE6\x88\x98\xE6\x9C\xBA",
                                  "\xE5\xA0\xA1\xE5\x9E\x92\xE9\x87\x8D\xE8\x88\xB0",
                                  "\xE6\x9A\x97\xE5\xBD\xB1\xE6\x88\x98\xE6\x9C\xBA"};
        const char* descs[3] = {
            "\xE2\x80\xA2 Agile \xE2\x80\xA2 Speedy  \xE2\x9A\xA1 Bullet Storm",
            "\xE2\x80\xA2 Armored \xE2\x80\xA2 Tank   \xE2\x9A\xA1 Iron Shield",
            "\xE2\x80\xA2 Tactical \xE2\x80\xA2 Drones \xE2\x9A\xA1 Wingmen",
        };

        bool sel = (i == g_select_idx);
        std::string line1 = arrow_r(sel) + sprites_top[i] + arrow_l(sel)
                          + "  " + names[i] + "  " + cnames[i] + "  " + descs[i];
        std::string line2 = arrow_r(sel) + sprites_bot[i] + arrow_l(sel);

        Element e1 = text(line1) | color(sel ? themes[i] : Color::Grey50);
        Element e2 = text(line2) | color(sel ? themes[i] : Color::Grey50);
        if (sel) { e1 = e1 | bold; e2 = e2 | bold; }
        else     { e1 = e1 | dim;  e2 = e2 | dim;  }

        rows.push_back(e1);
        rows.push_back(e2);
        rows.push_back(text(""));
    }

    return vbox(std::move(rows));
}

Element BuildPlayingScreen() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    constexpr int kMainW = kFieldWidth + 2;  // 36
    auto blank = [&]{ return text(std::string(kMainW, ' ')); };

    std::ostringstream info;
    info << "Lv" << g_game.level()
         << " | " << g_game.score()
         << " | " << g_game.kills() << "/" << g_game.kills_needed();
    auto info_bar = text(info.str()) | bold | color(Color::Yellow);

    // Boss 血条
    Element boss_bar = blank();
    if (g_game.boss_alive()) {
        int hp = g_game.boss_hp(), mx = g_game.boss_max_hp();
        int w = 16, bars = mx > 0 ? hp * w / mx : 0;
        std::string bar;
        for (int i = 0; i < w; ++i) bar += (i < bars) ? '\xDB' : '\xB0';
        std::ostringstream ss;
        ss << " BOSS [" << bar << "] " << hp << "/" << mx;
        boss_bar = text(ss.str()) | color(Color::Orange1) | bold;
    } else if (g_game.boss_dying()) {
        boss_bar = text(" *** BOSS DEFEATED! *** ") | bold | color(Color::Yellow) | blink;
    }

    // 终极技能条
    Element ult_bar = blank();
    {
        int ult_dur = g_game.ultimate_duration();
        int inv = g_game.invincible();
        float charge = g_game.ultimate_charge();
        if (ult_dur > 0 || inv > 0) {
            int remaining = ult_dur > 0 ? ult_dur : inv;
            int sec = remaining / 30;
            std::ostringstream ss;
            ss << " \xE2\x9A\xA1 ULT " << sec << "s";
            ult_bar = text(ss.str()) | bold | color(Color::Yellow);
        } else if (charge >= 1.0f) {
            ult_bar = text(" [E] ULTIMATE READY!") | bold | color(Color::Yellow);
        } else {
            int w = 14;
            int bars = static_cast<int>(charge * w);
            std::string bar;
            for (int i = 0; i < w; ++i) bar += (i < bars) ? '\xDB' : '\xB0';
            std::ostringstream ss;
            ss << " ULT [" << bar << "] " << static_cast<int>(charge * 100) << "%";
            ult_bar = text(ss.str()) | color(Color::Magenta);
        }
    }

    auto field = BuildColoredField();
    auto side  = BuildSidePanel();
    auto main  = vbox({ info_bar, blank(), boss_bar, blank(), ult_bar, blank(), field })
               | size(WIDTH, EQUAL, kMainW);
    return hbox({ filler(), side, separator() | color(Color::Blue), main, filler() });
}

Element BuildPausedScreen() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    std::ostringstream info;
    info << "Lv" << g_game.level() << " | " << g_game.score();

    auto info_bar = text(info.str()) | bold | color(Color::Yellow);
    auto field    = BuildColoredField();
    auto paused   = text("  ══  PAUSED  ══  ") | bold | color(Color::Yellow) | border;
    auto hint     = text("  P : Resume    Q : Quit  ") | dim;

    constexpr int kMainW = kFieldWidth + 2;
    auto blank = [&]{ return text(std::string(kMainW, ' ')); };
    auto main = vbox({ info_bar, blank(), field, blank(), paused, hint }) | size(WIDTH, EQUAL, kMainW);
    auto side = BuildSidePanel();
    return hbox({ filler(), side, separator() | color(Color::Blue), main, filler() });
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
    auto hint  = text("  [R] Retry    [M] Menu    [Q] Quit  ") | color(Color::White) | bold | center;

    return vbox({ text(""), boom, text(""), over1, over2, text(""),
                  stats, text(""), sep, text(""), hint }) | center;
}

Element BuildLevelClearScreen() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    int frame = g_anim_frame.load(std::memory_order_relaxed);
    int wave = (frame / 8) % 3;
    Color star_colors[3] = {Color::Yellow, Color::GreenLight, Color::CyanLight};

    auto trophy = vbox({
        text(R"(  *  *  *  *  *  *  *  *  *  *  *  )") | color(star_colors[(wave+0)%3]) | bold,
        text(R"(   *   *   *   *   *   *   *   *   )") | color(star_colors[(wave+1)%3]) | bold,
        text(R"(  *   V I C T O R Y !   *  *  *  )") | color(Color::Yellow) | bold,
        text(R"(   *   *   *   *   *   *   *   *   )") | color(star_colors[(wave+2)%3]) | bold,
        text(R"(  *  *  *  *  *  *  *  *  *  *  *  )") | color(star_colors[(wave+0)%3]) | bold,
    }) | center;

    auto title = text("  LEVEL " + std::to_string(g_game.level()) + " CLEAR!  ") | bold | color(Color::Green) | center | borderDouble;

    std::ostringstream sc;
    sc << "  Score: " << g_game.score()
       << "    Kills: " << g_game.kills()
       << "    Lives: " << g_game.lives();
    auto stats = text(sc.str()) | color(Color::White) | bold | center;

    std::ostringstream bn;
    bn << "  +" << (kBossScoreBonus * g_game.level()) << " BOSS BONUS  ";
    auto bonus = text(bn.str()) | color(Color::Orange1) | bold | center;

    auto sep   = text("  ───────────────────────────────────────  ") | color(Color::Grey30) | center;
    auto hint  = text("  [ENTER] Next Level    [R] Retry    [M] Menu  ") | bold | center;

    return vbox({ text(""), trophy, text(""), title, text(""),
                  stats, text(""), bonus,
                  text(""), sep, text(""), hint }) | center;
}

Element BuildUI() {
    GameState st;
    {
        std::lock_guard<std::recursive_mutex> lock(g_mutex);
        st = g_game.state();
    }
    switch (st) {
        case GameState::kMenu:       return BuildMenuScreen();
        case GameState::kSelect:     return BuildSelectScreen();
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
                { std::lock_guard<std::recursive_mutex> lk(g_mutex); g_game.EnterSelect(); }
                return true;
            }
            return false;
        }
        if (st == GameState::kSelect) {
            if (event == Event::ArrowUp || event == Event::Character('w') || event == Event::Character('W')) {
                g_select_idx = (g_select_idx + 2) % 3; return true;
            }
            if (event == Event::ArrowDown || event == Event::Character('s') || event == Event::Character('S')) {
                g_select_idx = (g_select_idx + 1) % 3; return true;
            }
            if (event == Event::Return) {
                { std::lock_guard<std::recursive_mutex> lk(g_mutex); g_game.SelectPlane(g_select_idx); g_game.StartGame(); }
                g_cur_dir = Dir::kNone; g_shooting = false; return true;
            }
            if (event == Event::Character('q') || event == Event::Character('Q')) {
                { std::lock_guard<std::recursive_mutex> lk(g_mutex); g_game.BackToMenu(); }
                return true;
            }
            return false;
        }
        if (st == GameState::kGameOver || st == GameState::kLevelClear) {
            if (event == Event::Character('r') || event == Event::Character('R')) {
                { std::lock_guard<std::recursive_mutex> lk(g_mutex); g_game.StartGame(); }
                g_cur_dir = Dir::kNone; g_shooting = false; return true;
            }
            if (event == Event::Character('m') || event == Event::Character('M')) {
                { std::lock_guard<std::recursive_mutex> lk(g_mutex); g_game.BackToMenu(); }
                return true;
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
        if (st == GameState::kPlaying) {
            if (event == Event::Character('e') || event == Event::Character('E')) {
                std::lock_guard<std::recursive_mutex> lk(g_mutex); g_game.ActivateUltimate(); return true;
            }
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
            g_anim_frame.fetch_add(1, std::memory_order_relaxed);
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
