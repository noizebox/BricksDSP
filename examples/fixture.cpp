#include <vector>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <thread>

#include <sndfile.h>

#define DSP_BRICKS_BLOCK_SIZE 32

#include "bricks_dsp/bricks.h"

constexpr int EXAMPLE_SAMPLERATE = 44100;
constexpr char EXAMPLE_FILE_NAME[] = "./output.wav";
constexpr int SECONDS_TO_RENDER = 10;

using namespace bricks;

void print_timings(const std::vector<std::chrono::nanoseconds>& timings)
{
    int64_t mean = 0;
    for (auto i : timings)
    {
        mean += i.count();
    }
    mean = mean / timings.size();
    auto max = *std::max_element(timings.begin(), timings.end());
    std::cout << "Mean time: " << mean << " ns, max time: " << max.count() <<std::endl;


}


/* Test fixture for generating audio to disk from brick */
int main ()
{
    SF_INFO file_info;
    file_info.channels = 1;
    file_info.samplerate = EXAMPLE_SAMPLERATE;
    file_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    SNDFILE* output_file = sf_open(EXAMPLE_FILE_NAME, SFM_WRITE, &file_info);
    if (output_file == nullptr)
    {
        std::cout << "Couldn't open file for writing: " << sf_strerror(nullptr) << std::endl;
        return -1;
    }

    /* Timing utilities */
    std::vector<std::chrono::nanoseconds> timing_records;


    /*float pitch = 0.25f;
    float gain = 0.0f;
    //float pwm = 0.0;
    WtOscillatorBrick test_brick{pitch};
    VcaBrick gain_brick{gain, test_brick.audio_output(0)};
    test_brick.set_samplerate(EXAMPLE_SAMPLERATE);
    test_brick.set_waveform(WtOscillatorBrick::Waveform::PULSE   );*/



    float _dummy = 1.0f;
    float _pitch = 0.2;
    float _pitch2 = 0.1;
    float _pitch_fact = 1;
    float _fm_mod = 0;
    float _pwm = 0.5;
    float _gain = 0.7f;
    float _clip_level = 3.2f;
    //float pwm = 0.0;
    WtOscillatorBrick           _osc{_pitch};
    SaturationBrick   _clipper{_clip_level, _osc.audio_output(0)};
    VcaBrick<Response::LINEAR>  _vca{_gain, _clipper.audio_output(0)};
    const AudioBuffer& out =    _vca.audio_output(0);

    _osc.set_waveform(WtOscillatorBrick::Waveform::SINE);
    int samplecount = 0;
    int max_samples = SECONDS_TO_RENDER * EXAMPLE_SAMPLERATE;
    while (samplecount < max_samples)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        _pitch = static_cast<float_t>(samplecount) / max_samples;

        _osc.render();
        _clipper.render();
        _vca.render();

        auto stop_time = std::chrono::high_resolution_clock::now();
        timing_records.push_back(stop_time - start_time);
        sf_writef_float(output_file, out.data(), PROC_BLOCK_SIZE);
        samplecount += PROC_BLOCK_SIZE;
        //std::this_thread::sleep_for(std::chrono::microseconds(500));
    }

    print_timings(timing_records);
    sf_close(output_file);
    return 0;
}