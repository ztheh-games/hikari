#ifndef HIKARI_GME_SOUND_STREAM
#define HIKARI_GME_SOUND_STREAM

#include "hikari/client/audio/Playback.hpp"
#include "hikari/client/platform/Events.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct Music_Emu;

namespace hikari {
class GMESoundStream : public audio::Stream {
    std::unique_ptr<Music_Emu> emu;
    mutable std::mutex mutex;
    std::vector<short> samples;
    std::size_t generate(short * output, std::size_t count) override;
public:
    GMESoundStream(std::size_t bufferSize, std::shared_ptr<audio::Device> device);
    ~GMESoundStream() override;
    bool open(const std::string & filename);
    static bool validateFile(const std::string & filename);
    void onSeek(platform::Time timeOffset);
    long getSampleRate() const;
    int getCurrentTrack() const;
    void setCurrentTrack(int track);
    int getTrackCount() const;
    const std::string getTrackName();
    int getVoiceCount() const;
    std::vector<std::string> getVoiceNames() const;
    std::unique_ptr<audio::PcmBuffer> renderTrackToBuffer(int track);
};
}
#endif
