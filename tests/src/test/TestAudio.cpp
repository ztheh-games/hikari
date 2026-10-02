#include <catch.hpp>
#include "hikari/client/audio/Playback.hpp"
#include "hikari/client/audio/GMESoundStream.hpp"
#include "hikari/core/util/FileSystemSession.hpp"
#include "hikari/core/util/PhysFS.hpp"
#include <SDL3/SDL.h>
#include <string>

TEST_CASE("SDL sample voices finish, restart, stop, and release callbacks", "[audio]") {
    auto device = std::make_shared<hikari::audio::Device>();
    auto pcm = std::make_shared<hikari::audio::PcmBuffer>();
    pcm->samples.resize(4410,1000);
    hikari::audio::SampleVoice voice(device,pcm);
    REQUIRE_FALSE(voice.isPlaying());
    voice.setVolume(50); voice.play();
    REQUIRE(voice.isPlaying());
    for (int i = 0; i < 200 && voice.isPlaying(); ++i) SDL_Delay(10);
    voice.checkError();
    REQUIRE_FALSE(voice.isPlaying());
    voice.play(); REQUIRE(voice.isPlaying());
    voice.stop(); REQUIRE_FALSE(voice.isPlaying());
    { hikari::audio::SampleVoice temporary(device,pcm); temporary.play(); }
}

TEST_CASE("SDL callback decoder failures surface on the main thread", "[audio]") {
    struct BrokenStream : hikari::audio::Stream {
        using Stream::Stream;
        ~BrokenStream() override { close(); }
        std::size_t generate(short *,std::size_t) override { throw std::runtime_error("fixture decoder error"); }
    };
    auto device = std::make_shared<hikari::audio::Device>();
    BrokenStream stream(device);
    stream.play();
    std::string failure;
    for (int i = 0; i < 200 && failure.empty(); ++i) {
        SDL_Delay(10);
        try { stream.checkError(); } catch(const std::runtime_error & error) { failure = error.what(); }
    }
    REQUIRE(failure == "Audio callback failed: fixture decoder error");
    stream.stop();
}

TEST_CASE("GME pre-rendering preserves stereo sample cap and block rounding", "[audio][gme]") {
    hikari::FileSystemSession filesystem("audio-tests");
    PhysFS::addToSearchPath(HIKARI_TEST_CONTENT_DIR);
    auto device = std::make_shared<hikari::audio::Device>();
    hikari::GMESoundStream stream(4096,device);
    REQUIRE(stream.open("assets/sound/mega-man-3-nes-[NSF-ID2016].nsf"));
    REQUIRE(stream.getSampleRate() == 44100);
    REQUIRE(stream.getTrackCount() > 0);
    const auto pcm = stream.renderTrackToBuffer(0);
    REQUIRE(pcm->channels == 2);
    REQUIRE(pcm->sampleRate == 44100);
    REQUIRE(pcm->samples.size() > 0);
    REQUIRE((pcm->samples.size() % 512) == 0);
    REQUIRE(pcm->samples.size() == 176640);
    REQUIRE_THROWS_AS(stream.setCurrentTrack(stream.getTrackCount()),std::out_of_range);
}
