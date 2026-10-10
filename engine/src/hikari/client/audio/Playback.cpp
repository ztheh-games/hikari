#include "hikari/client/audio/Playback.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <string>

namespace hikari::audio {
namespace {
void require(bool success, const char * operation) {
    if (!success) throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}
}
Device::Device() {
    require(SDL_InitSubSystem(SDL_INIT_AUDIO), "Initialize SDL audio");
    id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,nullptr);
    if (!id) {
        const std::string message = std::string("Open audio device: ") + SDL_GetError();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        throw std::runtime_error(message);
    }
}
Device::~Device() { if (id) SDL_CloseAudioDevice(id); SDL_QuitSubSystem(SDL_INIT_AUDIO); }
Stream::Stream(std::shared_ptr<Device> device, std::size_t bufferSize)
    : device(std::move(device)), buffer(bufferSize) {
    if (!this->device || bufferSize < 2 || bufferSize % 2)
        throw std::invalid_argument("Audio requires a device and an even stereo sample buffer");
    const SDL_AudioSpec input{SDL_AUDIO_S16,2,44100};
    stream = SDL_CreateAudioStream(&input,nullptr);
    require(stream != nullptr,"Create audio stream");
    if (!SDL_SetAudioStreamGetCallback(stream,&Stream::callback,this) ||
        !SDL_BindAudioStream(this->device->getId(),stream)) {
        const std::string message = std::string("Bind audio stream: ") + SDL_GetError();
        SDL_DestroyAudioStream(stream); stream = nullptr;
        throw std::runtime_error(message);
    }
}
Stream::~Stream() { close(); }
void Stream::close() {
    if (stream) {
        SDL_DestroyAudioStream(stream); stream = nullptr;
    }
}
void Stream::play() {
    checkError();
    require(SDL_LockAudioStream(stream),"Lock playback stream");
    producing = true;
    SDL_UnlockAudioStream(stream);
}
void Stream::stop() {
    if (!stream) return;
    require(SDL_LockAudioStream(stream),"Lock stopped stream");
    producing = false;
    const bool cleared = SDL_ClearAudioStream(stream);
    SDL_UnlockAudioStream(stream);
    require(cleared,"Clear stopped audio");
}
bool Stream::isPlaying() const {
    require(SDL_LockAudioStream(stream),"Lock playback status");
    const int queued = SDL_GetAudioStreamQueued(stream);
    const int available = SDL_GetAudioStreamAvailable(stream);
    const bool result = producing || queued > 0 || available > 0;
    SDL_UnlockAudioStream(stream);
    require(queued >= 0 && available >= 0,"Read playback status");
    return result;
}
void Stream::setVolume(float volume) {
    if (!std::isfinite(volume)) throw std::invalid_argument("Non-finite playback volume");
    require(SDL_SetAudioStreamGain(stream,std::clamp(volume,0.0f,100.0f)/100.0f),"Set playback gain");
}
void Stream::checkError() {
    require(SDL_LockAudioStream(stream),"Lock callback diagnostics");
    const bool hasError = failed.exchange(false);
    const auto diagnostic = error;
    SDL_UnlockAudioStream(stream);
    if (hasError) throw std::runtime_error(std::string("Audio callback failed: ") + diagnostic.data());
}
void Stream::callback(void * userdata, SDL_AudioStream * stream, int additional, int) {
    auto & self = *static_cast<Stream *>(userdata);
    if (!self.producing || additional <= 0) return;
    try {
        std::size_t remaining = (static_cast<std::size_t>(additional) + sizeof(short)*2-1) / (sizeof(short)*2) * 2;
        while (remaining && self.producing) {
            const auto requested = std::min(remaining,self.buffer.size());
            const auto produced = self.generate(self.buffer.data(),requested);
            if (produced > requested || produced % 2) throw std::runtime_error("Invalid stereo PCM chunk");
            if (produced) require(SDL_PutAudioStreamData(stream,self.buffer.data(),static_cast<int>(produced*sizeof(short))),"Queue PCM");
            if (produced < requested) {
                self.producing = false;
                require(SDL_FlushAudioStream(stream),"Flush finite audio");
                break;
            }
            remaining -= produced;
        }
    } catch (const std::exception & ex) {
        self.producing = false;
        std::snprintf(self.error.data(),self.error.size(),"%s",ex.what());
        self.failed.store(true);
    }
}
SampleVoice::SampleVoice(std::shared_ptr<Device> device, std::shared_ptr<const PcmBuffer> pcm)
    : Stream(std::move(device)), pcm(std::move(pcm)) {
    if (!this->pcm || this->pcm->channels != 2 || this->pcm->sampleRate != 44100 || this->pcm->samples.size() % 2)
        throw std::invalid_argument("Invalid GME sample format");
}
SampleVoice::~SampleVoice() { close(); }
void SampleVoice::play() {
    stop(); position = 0;
    Stream::play();
}
std::size_t SampleVoice::generate(short * output, std::size_t count) {
    const auto available = std::min(count,pcm->samples.size()-position);
    std::copy_n(pcm->samples.data()+position,available,output);
    position += available;
    return available;
}
}
