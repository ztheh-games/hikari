#include "hikari/client/audio/GMESoundStream.hpp"
#include "hikari/core/util/FileSystem.hpp"
#include "hikari/core/util/Log.hpp"
#include <Music_Emu.h>
#include <gme.h>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace hikari {
namespace {
constexpr long sampleRate = 44100;
void gmeCheck(const char * error) { if (error) throw std::runtime_error(error); }
std::unique_ptr<Music_Emu> loadEmulator(const std::string & filename) {
    const auto type = gme_identify_extension(filename.c_str());
    if (!type) throw std::runtime_error("Unsupported music format: " + filename);
    auto input = FileSystem::openFileRead(filename);
    const std::vector<char> bytes{std::istreambuf_iterator<char>(*input),std::istreambuf_iterator<char>()};
    if (bytes.empty() || bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::runtime_error("Invalid NSF data size: " + filename);
    std::unique_ptr<Music_Emu> emu(type->new_emu());
    if (!emu) throw std::runtime_error("Cannot create NSF emulator: " + filename);
    gmeCheck(emu->set_sample_rate(sampleRate));
    gmeCheck(gme_load_data(emu.get(),bytes.data(),static_cast<long>(bytes.size())));
    gmeCheck(emu->start_track(0));
    return emu;
}
}
GMESoundStream::GMESoundStream(std::size_t bufferSize, std::shared_ptr<audio::Device> device)
    : audio::Stream(std::move(device),bufferSize), samples(bufferSize) {}
GMESoundStream::~GMESoundStream() { close(); }
bool GMESoundStream::open(const std::string & filename) {
    stop();
    std::lock_guard<std::mutex> lock(mutex);
    try { emu = loadEmulator(filename); }
    catch(const std::runtime_error & failure) { HIKARI_LOG(hikari::error) << "NSF load failed: " << failure.what(); return false; }
    return true;
}
bool GMESoundStream::validateFile(const std::string & filename) {
    try { auto validator = loadEmulator(filename); }
    catch(const std::runtime_error & failure) { HIKARI_LOG(hikari::error) << "NSF validation failed: " << failure.what(); return false; }
    return true;
}
void GMESoundStream::onSeek(platform::Time timeOffset) {
    const bool playing = isPlaying();
    stop();
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!emu) throw std::logic_error("Seek on unopened NSF");
        gmeCheck(emu->seek(static_cast<long>(timeOffset.asMilliseconds())));
    }
    if (playing) play();
}
std::size_t GMESoundStream::generate(short * output, std::size_t count) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!emu) throw std::logic_error("Playback on unopened NSF");
    if (emu->track_ended()) return 0;
    gmeCheck(emu->play(static_cast<long>(count),output));
    return count;
}
long GMESoundStream::getSampleRate() const { return sampleRate; }
int GMESoundStream::getCurrentTrack() const {
    std::lock_guard<std::mutex> lock(mutex);
    return emu ? emu->current_track() : 0;
}
void GMESoundStream::setCurrentTrack(int track) {
    stop();
    std::lock_guard<std::mutex> lock(mutex);
    if (!emu || track < 0 || track >= emu->track_count()) throw std::out_of_range("NSF track outside library");
    gmeCheck(emu->start_track(track));
}
int GMESoundStream::getTrackCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return emu ? emu->track_count() : 0;
}
const std::string GMESoundStream::getTrackName() {
    std::lock_guard<std::mutex> lock(mutex);
    if (!emu) throw std::logic_error("Metadata on unopened NSF");
    track_info_t info;
    gmeCheck(emu->track_info(&info));
    return info.song;
}
int GMESoundStream::getVoiceCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return emu ? emu->voice_count() : 0;
}
std::vector<std::string> GMESoundStream::getVoiceNames() const {
    std::lock_guard<std::mutex> lock(mutex);
    if (!emu) return {};
    return {emu->voice_names(),emu->voice_names()+emu->voice_count()};
}
std::unique_ptr<audio::PcmBuffer> GMESoundStream::renderTrackToBuffer(int track) {
    stop();
    std::lock_guard<std::mutex> lock(mutex);
    if (!emu || track < 0 || track >= emu->track_count()) throw std::out_of_range("Sample track outside NSF");
    gmeCheck(emu->start_track(track));
    auto result = std::make_unique<audio::PcmBuffer>();
    constexpr std::size_t chunk = 512, maximum = 4 * sampleRate;
    if (samples.size() < chunk) samples.resize(chunk);
    while (!emu->track_ended() && result->samples.size() < maximum) {
        gmeCheck(emu->play(chunk,samples.data()));
        result->samples.insert(result->samples.end(),samples.begin(),samples.begin()+chunk);
    }
    return result;
}
}
