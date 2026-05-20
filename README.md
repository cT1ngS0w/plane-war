# Plane War

基于 C++ 和 FTXUI 库开发的终端彩色飞机大战游戏，支持关卡推进、Boss 战、道具系统和粒子特效。

## 游戏特性

- **彩色 TUI 界面** — 基于 FTXUI 的终端渲染，支持真彩色和 Unicode 字符
- **关卡递进** — 每关击落 10 架敌机后遭遇关底 Boss，击败后进入更高难度关卡
- **Boss 战** — Boss 拥有多阶段攻击模式：圆形弹幕（全方向 16 发）和扇形追瞄弹幕
- **道具系统** — 三种可拾取道具：回血（+）、射速提升（~）、双发子弹（=）
- **障碍物** — 从顶部掉落的障碍物会阻挡玩家移动和子弹
- **粒子爆炸特效** — 击毁敌机和 Boss 时产生爆炸粒子动画
- **侧边信息面板** — 实时显示关卡、分数、生命、击杀数、道具效果倒计时和 Boss 血条
- **难度递增** — 每关提升敌机速度、射击频率，并提高敌机血量和 Boss 血量

## 游戏截图

```
  Level 3  |  Score 5300  |  Kills 7/10

  BOSS  [████████░░░░░░]  14/20

+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
:                                    :
:              O                     :
:         \M/                        :
:          v                         :
:                                    :
:        \   /                       :
:        | B |          |            :
:        /_ _\          |            :
:                       |            :
:         O     O       |            :
:                                    :
:          O                        :
:         ~                         :
:                                    :
:         ^                          :
:        /H\                         :
:                                    :
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+

  STATS
  ──────────────────
  Level:      3
  Score:      5300
  Lives:      ♥ ♥ ◯ ◯ ◯
  Kills:      7 / 10

  EFFECTS
  ──────────────────
  ~ Fire Rate:  18s

  BOSS
  ──────────────────
  HP:       ████████░░░░░░  14/20

  ITEMS
  ──────────────────
  +         Health  +1
  ~         Fire Rate
  =         Dual Shot
```

## 操作方式

| 按键 | 功能 |
|------|------|
| `W` / `↑` | 向上移动 |
| `S` / `↓` | 向下移动 |
| `A` / `←` | 向左移动 |
| `D` / `→` | 向右移动 |
| `Space` | 发射子弹 |
| `P` | 暂停 / 继续 |
| `R` | 重新开始（游戏结束或过关后） |
| `Enter` | 开始游戏 / 进入下一关 |
| `Q` | 退出游戏 |

## 游戏规则

- 操控战斗机 `^` 在区域内自由移动，发射子弹 `|` 击毁敌机
- 躲避敌方子弹 `O` 和敌机撞击，每被击中一次扣一条命
- 敌机逃出屏幕底部也会扣命
- 击毁 10 架敌机后，Boss 登场，击败 Boss 进入下一关
- 拾取 `+` 回血、`~` 提升射速、`=` 双发子弹
- `#` 障碍物会阻挡移动和子弹，可推动其移动
- 共 5 条命，耗尽游戏结束
- 关卡越高，敌机越快、越多、越耐打

## 构建与运行

### 依赖

- C++17 编译器（GCC、Clang 或 MSVC）
- CMake >= 3.14
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) 库

### 安装 FTXUI

**Ubuntu / Debian:**
```bash
sudo apt install cmake g++
git clone https://github.com/ArthurSonzogni/FTXUI.git
cd FTXUI
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

**macOS:**
```bash
brew install ftxui
```

**Windows (vcpkg):**
```bash
vcpkg install ftxui
```

### 构建项目

```bash
git clone https://github.com/cT1ngS0w/plane-war.git
cd plane-war
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 运行

```bash
./plane_war
```

## 项目结构

```
plane-war/
├── CMakeLists.txt      # CMake 构建配置
├── README.md           # 项目说明
└── src/
    ├── main.cpp        # 入口：FTXUI 渲染、事件处理、游戏循环
    ├── game.h          # 常量定义、实体结构体、Game 类声明
    └── game.cpp        # 游戏逻辑：移动、碰撞、Boss AI、粒子系统
```

## 开源协议

本项目仅用于课程学习目的。
