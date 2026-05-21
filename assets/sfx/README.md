# 音效资源

程序化生成的 WAV 音效文件，由 `src/audio.cpp` 在首次运行时自动生成。

| 文件 | 触发场景 | 生成方式 |
|------|---------|---------|
| `sfx_shoot.wav` | 玩家发射子弹 | 800Hz 正弦波, 0.08s |
| `sfx_explosion.wav` | 敌机/Boss 爆炸、炸弹全屏清敌 | 白噪声, 0.3s |
| `sfx_enemy_die.wav` | 敌机被击毁 | 300Hz 正弦波, 0.1s |
| `sfx_hit.wav` | 玩家被击中 | 120Hz 正弦波, 0.2s |
| `sfx_powerup.wav` | 拾取道具 | 400→1200Hz 扫频, 0.2s |
| `sfx_boss.wav` | Boss 登场 | 100→60Hz 低频扫频, 0.6s |
| `sfx_ultimate.wav` | 释放终极技能 | 200→1600Hz 扫频, 0.5s |

- 采样率：22050Hz, 单声道, 16-bit PCM
- 首次运行时自动写入，之后直接从磁盘加载，不重复生成
