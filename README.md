# Plane War

基于 C++ 和 FTXUI 库开发的终端飞机大战游戏。

## 游戏简介

玩家操控一架战机在终端中与敌机作战。躲避敌方子弹，击落敌机，逐关推进，获取高分。

## 游戏截图

```
+------------------------------------+
|                                    |
|              V                     |
|              |                     |
|         .                         |
|                         V         |
|                                    |
|                                    |
|              .                    |
|                                    |
|                                    |
|                   V               |
|                                    |
|   V              .                |
|                                    |
|              A                     |
|                                    |
+------------------------------------+

  Level: 1  |  Score: 300  |  Lives: ♥ ♥ ♥  |  Kills: 3/10
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
| `R` | 重新开始（游戏结束后） |
| `Enter` | 开始游戏 / 进入下一关 |
| `Q` | 退出游戏 |

## 游戏规则

- 操控战机 `A` 在游戏区域内自由移动
- 发射子弹 `|` 击毁敌机 `V`
- 躲避敌方子弹 `.` 和敌机撞击
- 每击毁 10 架敌机进入下一关
- 玩家有 3 条生命，被击中或让敌机逃出屏幕底部会扣命
- 生命耗尽则游戏结束
- 关卡递增，敌机速度和射击频率越来越快

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
git clone https://github.com/<your-username>/plane-war.git
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
├── README.md           # 项目说明文档
└── src/
    ├── main.cpp        # 入口：FTXUI 渲染、事件处理、游戏循环
    ├── game.h          # 游戏常量、实体结构、Game 类声明
    └── game.cpp        # 游戏逻辑实现
```

## 技术要点

- **游戏循环**：独立线程以 30 FPS 运行游戏逻辑更新
- **渲染**：FTXUI `ScreenInteractive` + 自定义 `Renderer` 组件
- **事件驱动**：通过 `CatchEvent` 捕获键盘输入，支持方向键和 WASD
- **碰撞检测**：逐帧检测玩家子弹与敌机、敌方子弹与玩家的碰撞
- **关卡系统**：基于击杀数的关卡推进，难度递增

## 贡献指南

1. Fork 本项目
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

## 开源协议

本项目仅用于课程学习目的。
