#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "audio.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static ma_engine* g_engine = nullptr;
static ma_sound*  g_sounds = nullptr;
static bool       g_loaded = false;

AudioManager g_audio;

namespace {

std::vector<uint8_t> make_wav(const std::vector<float>& samples, int sample_rate) {
    int data_size = static_cast<int>(samples.size() * sizeof(int16_t));
    int file_size = 44 + data_size;
    std::vector<uint8_t> wav(file_size);

    auto w4 = [&](int pos, uint32_t v) {
        wav[pos]=v&0xFF; wav[pos+1]=(v>>8)&0xFF; wav[pos+2]=(v>>16)&0xFF; wav[pos+3]=(v>>24)&0xFF;
    };
    auto w2 = [&](int pos, uint16_t v) {
        wav[pos]=v&0xFF; wav[pos+1]=(v>>8)&0xFF;
    };

    // RIFF header
    w4(0, 0x46464952);   // "RIFF"
    w4(4, file_size - 8);
    w4(8, 0x45564157);   // "WAVE"
    // fmt chunk
    w4(12, 0x20746d66);  // "fmt "
    w4(16, 16);
    w2(20, 1);           // PCM
    w2(22, 1);           // mono
    w4(24, sample_rate);
    w4(28, sample_rate * 2);
    w2(32, 2);
    w2(34, 16);
    // data chunk
    w4(36, 0x61746164);  // "data"
    w4(40, data_size);
    for (size_t i = 0; i < samples.size(); ++i) {
        float v = samples[i];
        if (v > 1.0f) v = 1.0f;
        if (v < -1.0f) v = -1.0f;
        int16_t s = static_cast<int16_t>(v * 32767.0f);
        wav[44 + i * 2] = s & 0xFF;
        wav[44 + i * 2 + 1] = (s >> 8) & 0xFF;
    }
    return wav;
}

void write_temp(const char* name, const std::vector<uint8_t>& data) {
    FILE* f = fopen(name, "wb");
    if (f) { fwrite(data.data(), 1, data.size(), f); fclose(f); }
}

std::vector<float> gen_tone(float freq, float duration, float sample_rate) {
    int n = static_cast<int>(sample_rate * duration);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        float env = 1.0f - (t / duration);
        s[i] = std::sin(2.0f * static_cast<float>(M_PI) * freq * t) * env * 0.4f;
    }
    return s;
}

std::vector<float> gen_noise(float duration, float sample_rate) {
    int n = static_cast<int>(sample_rate * duration);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        float env = 1.0f - (t / duration);
        s[i] = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * env * 0.3f;
    }
    return s;
}

std::vector<float> gen_sweep(float f0, float f1, float duration, float sample_rate) {
    int n = static_cast<int>(sample_rate * duration);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        float freq = f0 + (f1 - f0) * (t / duration);
        float env = 1.0f - (t / duration);
        s[i] = std::sin(2.0f * static_cast<float>(M_PI) * freq * t) * env * 0.5f;
    }
    return s;
}

} // namespace

AudioManager::AudioManager() {}

void AudioManager::init() {
    if (initialized_) return;
    constexpr int SR = 22050;

    struct SfxDef { Sfx id; const char* name; std::vector<float> (*gen)(); };
    SfxDef defs[] = {
        {Sfx::Shoot,      "sfx_shoot.wav",      []{ return gen_tone(800, 0.08f, SR); }},
        {Sfx::Explosion,  "sfx_explosion.wav",  []{ return gen_noise(0.3f, SR); }},
        {Sfx::PowerUp,    "sfx_powerup.wav",    []{ return gen_sweep(400, 1200, 0.2f, SR); }},
        {Sfx::BossAlert,  "sfx_boss.wav",       []{ return gen_sweep(100, 60, 0.6f, SR); }},
        {Sfx::Ultimate,   "sfx_ultimate.wav",   []{ return gen_sweep(200, 1600, 0.5f, SR); }},
        {Sfx::EnemyDie,   "sfx_enemy_die.wav",  []{ return gen_tone(300, 0.1f, SR); }},
        {Sfx::PlayerHit,  "sfx_hit.wav",        []{ return gen_tone(120, 0.2f, SR); }},
    };

    g_engine = new ma_engine{};
    g_sounds = new ma_sound[static_cast<int>(Sfx::Count)]{};

    ma_result mr = ma_engine_init(NULL, g_engine);
    if (mr != MA_SUCCESS) return;

    for (auto& d : defs) {
        auto wav = make_wav(d.gen(), SR);
        write_temp(d.name, wav);
        mr = ma_sound_init_from_file(g_engine, d.name, 0, NULL, NULL, &g_sounds[static_cast<int>(d.id)]);
        if (mr != MA_SUCCESS) continue;
    }

    g_loaded = true;
    initialized_ = true;
}

AudioManager::~AudioManager() {
    if (!initialized_) return;
    for (int i = 0; i < static_cast<int>(Sfx::Count); ++i)
        ma_sound_uninit(&g_sounds[i]);
    ma_engine_uninit(g_engine);
    delete[] g_sounds;
    delete g_engine;
}

void AudioManager::play(Sfx s) {
    if (!g_loaded || muted_) return;
    int idx = static_cast<int>(s);
    ma_sound_seek_to_pcm_frame(&g_sounds[idx], 0);
    ma_sound_start(&g_sounds[idx]);
}
