# Plane War

基于 C++17 和 FTXUI 库开发的终端真彩色飞机大战游戏。支持三种战机选择、终极技能、Boss 战、六种道具系统、连击机制和程序化音效。

## 游戏特性

### 战机与终极技能

| 战机 | 类型 | 描述 | 终极技能 (E键) |
|------|------|------|---------------|
| Valkyrie 迅风战机 | 敏捷型 | 3x2 小巧机身，高机动性 | 弹幕风暴 3s — 射速拉满、双发、无敌 |
| Fortress 堡垒重舰 | 重装型 | 5x2 宽体机身，碰撞体积大 | 铁壁护盾 4s — 免疫伤害 |
| Phantom 暗影战机 | 刺客型 | 3x2 紧凑机身 | 僚机召唤 10s — 两架无人机辅助射击 |

终极技能通过被动时间积累和击杀敌人充能，蓄满后按 `E` 键释放。

### 关卡与 Boss 战

- 每关需击落一定数量敌机（随关卡递增），随后 Boss 登场
- Boss 拥有两种攻击模式：圆形弹幕（全方向 16 发）和扇形追瞄弹幕
- 击败 Boss 后进入下一关，难度递增（敌机速度、射击频率、血量提升）
- Boss 死亡时触发爆炸粒子动画

### 敌机类型

| 类型 | 外观 | 行为 |
|------|------|------|
| Normal 普通型 | `\M/` + `v` | 左右微摆移动，可被左右移动干扰 |
| Diver 俯冲型 | `V` + `/D\` | 过半场后加速俯冲，速度提升 3 倍 |

### 六种道具

| 符号 | 道具 | 效果 |
|------|------|------|
| `+` | 回血 | 回复 1 条命（上限 5） |
| `~` | 射速提升 | 20 秒内射击冷却减半 |
| `=` | 双发子弹 | 20 秒内双管齐下 |
| `O` | 护盾 | 抵挡一次伤害 |
| `!` | 炸弹 | 全屏清敌，产生爆炸粒子效果 |
| `M` | 磁铁 | 15 秒内自动吸取附近道具 |

### 其他特性

- **真彩色渲染** — FTXUI 真彩色支持，彩色精灵、渐变 Logo、粒子特效
- **连击系统** — 连续击杀触发 Combo，Combo > 2 时屏幕显示橙色 COMBO 提示
- **程序化音效** — 基于 miniaudio 库，程序生成射击、爆炸、受伤、Boss、道具等 7 种音效
- **最高分记录** — 自动保存最高分到 `highscore.txt`
- **粒子爆炸特效** — 敌机、Boss 和炸弹道具触发爆炸粒子动画
- **障碍物** — 从顶部掉落，可被推动，阻挡移动和子弹
- **侧边信息面板** — 实时显示关卡、分数、生命、击杀进度、道具倒计时、Boss 血条和终极技能蓄能

## 操作方式

| 按键 | 功能 |
|------|------|
| `W` / `↑` | 向上移动 |
| `S` / `↓` | 向下移动 |
| `A` / `←` | 向左移动 |
| `D` / `→` | 向右移动 |
| `Space` | 发射子弹 |
| `E` | 释放终极技能（蓄满时） |
| `P` | 暂停 / 继续 |
| `R` | 重新开始（游戏结束或过关后） |
| `Enter` | 开始游戏 / 进入下一关 / 确认选择 |
| `Q` | 返回上级菜单 / 退出游戏 |

## 游戏规则

- 操控战机在区域内自由移动，发射子弹 `|` 击毁敌机
- 躲避敌方子弹 `O` 和敌机撞击，每被击中一次扣一条命
- 敌机逃出屏幕底部也会扣命
- 护盾可抵挡一次伤害，护盾激活时战机上方显示 `.` 光点
- 击毁足够敌机后 Boss 登场，击败 Boss 进入下一关
- 拾取道具获得增益效果，同种道具效果不叠加而是刷新时间
- 共 5 条命，耗尽游戏结束
- 关卡越高，敌机越快、越多、越耐打

## 游戏截图

```
  Level 3  |  Score 5300  COMBO x5!  |  7/12

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
:        /F\                         :
:                                    :
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+

  STATS
  ──────────────────
  Level:      3
  Score:      5300
  Lives:      ♥ ♥ ◯ ◯ ◯
  Kills:      7 / 12

  EFFECTS
  ──────────────────
  ~ Fire Rate:  18s

  BOSS
  ──────────────────
  HP:       ████████░░░░░░  14/20

  ULT [████████░░] 67%
```

## 构建与运行

### 依赖

- C++17 编译器（GCC、Clang 或 MSVC）
- CMake >= 3.14
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) 库
- [miniaudio](https://github.com/mackron/miniaudio) 库（已通过 FetchContent 自动获取）

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
cmake --build . --config Release -j8
```

### 运行

```bash
./plane_war        # Linux / macOS
.\build\Release\plane_war.exe   # Windows
```

## 项目结构

```
plane-war/
├── CMakeLists.txt      # CMake 构建配置（FTXUI + miniaudio 依赖）
├── README.md           # 项目说明文档
├── highscore.txt       # 最高分记录
├── sfx_*.wav           # 程序化生成的音效文件（7 个）
└── src/
    ├── main.cpp        # 入口：FTXUI 渲染、事件处理、UI 构建、游戏循环
    ├── game.h          # 常量定义、精灵数据、实体结构体、Game 类声明
    ├── game.cpp        # 游戏逻辑：移动、碰撞、Boss AI、粒子系统、道具
    ├── audio.h         # 音频管理器声明（Sfx 枚举、AudioManager 类）
    └── audio.cpp       # 音频实现：miniaudio 引擎、WAV 生成、音效播放
```

## 开源协议

本项目仅用于课程学习目的。
