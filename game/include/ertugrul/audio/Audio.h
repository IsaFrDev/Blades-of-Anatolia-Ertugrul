#pragma once
// waveOut asosidagi oddiy aralashtirgich (mixer). Tashqi kutubxona yo'q.
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace ert {

struct SoundData {
    std::vector<int16_t> samples;   // interleaved
    int channels = 1;
    int rate     = 44100;
    float durationSec() const { return channels && rate ? (float)(samples.size() / channels) / (float)rate : 0.0f; }
};
using SoundRef = std::shared_ptr<SoundData>;

enum Bus { BUS_MASTER = 0, BUS_MUSIC = 1, BUS_SFX = 2, BUS_VOICE = 3, BUS_AMBIENCE = 4, BUS_COUNT = 5 };

class Audio {
public:
    static Audio& get();

    bool init(int sampleRate = 44100);
    void shutdown();
    bool ready() const { return ready_; }
    // Har kadr chaqiriladi (bufferlarni to'ldiradi)
    void update();

    static SoundRef loadWav(const std::string& path);   // xatoda nullptr
    static SoundRef makeTone(float freqHz, float seconds, float amplitude = 0.3f, int rate = 44100);
    static SoundRef makeNoise(float seconds, float amplitude = 0.15f, int rate = 44100);

    // voiceId qaytaradi (0 = muvaffaqiyatsiz)
    int  play(SoundRef s, int bus = BUS_SFX, float volume = 1.0f, bool loop = false, float pitch = 1.0f);
    int  playFile(const std::string& wavPath, int bus = BUS_SFX, float volume = 1.0f, bool loop = false);
    void stop(int voiceId);
    void stopBus(int bus);
    void stopAll();
    bool isPlaying(int voiceId) const;

    void  setBusVolume(int bus, float v);
    float busVolume(int bus) const;

    // Musiqa uchun kesishuvchi o'tish
    void  crossfadeMusic(SoundRef s, float seconds = 1.5f, float volume = 0.6f);

private:
    Audio() = default;
    bool ready_ = false;
};

} // namespace ert
