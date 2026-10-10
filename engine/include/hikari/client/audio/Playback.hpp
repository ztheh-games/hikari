#ifndef HIKARI_AUDIO_PLAYBACK
#define HIKARI_AUDIO_PLAYBACK

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

struct SDL_AudioStream;

namespace hikari::audio {

class Device {
    std::uint32_t id = 0;
public:
    Device();
    ~Device();
    Device(const Device &) = delete;
    Device & operator=(const Device &) = delete;
    std::uint32_t getId() const { return id; }
};
struct PcmBuffer {
    std::vector<short> samples;
    unsigned int sampleRate = 44100;
    unsigned int channels = 2;
};

class Stream {
    std::shared_ptr<Device> device;
    SDL_AudioStream * stream = nullptr;
    std::vector<short> buffer;
    bool producing = false;
    std::atomic<bool> failed{false};
    std::array<char, 512> error{};
    static void callback(void * userdata, SDL_AudioStream * stream, int additional, int total);
protected:
    virtual std::size_t generate(short * output, std::size_t count) = 0;
    void close();
public:
    explicit Stream(std::shared_ptr<Device> device, std::size_t bufferSize = 4096);
    virtual ~Stream();
    Stream(const Stream &) = delete;
    Stream & operator=(const Stream &) = delete;
    void play();
    void stop();
    bool isPlaying() const;
    void setVolume(float volume);
    void checkError();
};

class SampleVoice : public Stream {
    std::shared_ptr<const PcmBuffer> pcm;
    std::size_t position = 0;
    std::size_t generate(short * output, std::size_t count) override;
public:
    SampleVoice(std::shared_ptr<Device> device, std::shared_ptr<const PcmBuffer> pcm);
    ~SampleVoice() override;
    void play();
};
}
#endif
