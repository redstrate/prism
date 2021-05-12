#include "audio.hpp"

#include <portaudio.h>
#include <opusfile.h>
#include <algorithm>
#include <cstring>

#include "log.hpp"
#include "file.hpp"
#include "platform.hpp"

constexpr int num_channels = 2;
constexpr int sample_rate = 48000;
constexpr int frame_size = 960;
constexpr int frames_per_buffer = 480;

struct AudioFile {
    float gain = 1.0f;
    bool finished = false;
    OggOpusFile* handle = nullptr;
};

std::vector<AudioFile> audio_files;

static int pa_callback(const void*, void* buffer,
                       unsigned long,
                       const PaStreamCallbackTimeInfo*,
                       PaStreamCallbackFlags,
                       void*) {
    auto out = static_cast<float*>(buffer);

    float final_values[frame_size] = {0.0f};

    for(auto& file : audio_files) {
        if(file.finished)
            continue;

        float values[frame_size] = {0.0f};

        int val = op_read_float_stereo(file.handle, values, frame_size);
        if(val == 0)
            file.finished = true;

        for(int i = 0; i < frame_size; i++) {
            final_values[i] += values[i] * file.gain;
        }
    }

    memcpy(out, final_values, frame_size * sizeof(float));

    audio_files.erase(std::remove_if(audio_files.begin(),
                                     audio_files.end(),
                                     [](AudioFile& x){return x.finished;}),
                      audio_files.end());

    return 0;
}

void audio::initialize() {
    platform::mute_output();

    Pa_Initialize();

    platform::unmute_output();

    PaStream* stream;

    Pa_OpenDefaultStream(&stream,
                         0,
                         num_channels,
                         paFloat32,
                         sample_rate,
                         frames_per_buffer,
                         pa_callback,
                         nullptr);

    Pa_StartStream(stream);
}

void audio::play_file(const prism::path path, const float gain) {
    auto audio_file = prism::open_file(path);
    if(audio_file == std::nullopt)
        return;

    audio_file->read_all();

    AudioFile file;
    file.gain = gain;
    file.handle = op_open_memory(audio_file->cast_data<unsigned char>(), audio_file->size(), nullptr);

    audio_files.push_back(file);
}
