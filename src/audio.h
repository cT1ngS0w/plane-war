#pragma once

enum class Sfx { Shoot, Explosion, PowerUp, BossAlert, Ultimate, EnemyDie, PlayerHit, Count };

class AudioManager {
public:
    AudioManager();
    ~AudioManager();
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    void play(Sfx s);
    void set_muted(bool m) { muted_ = m; }
    bool muted() const { return muted_; }

private:
    void* engine_ = nullptr;  // ma_engine*
    bool muted_ = false;
    bool initialized_ = false;
};

extern AudioManager g_audio;
