#include <vector>
#include <iostream>
#include <chrono>
#include <algorithm>

#include <sndfile.h>

#define DSP_BRICKS_BLOCK_SIZE 32

#include "bricks_dsp/bricks.h"

constexpr int EXAMPLE_SAMPLERATE = 44100;
constexpr char EXAMPLE_FILE_NAME[] = "./output.wav";
constexpr int SECONDS_TO_RENDER = 10;
constexpr int OVERSAMPLE = 4;

using namespace bricks;

constexpr std::array<bricks::BiquadCoefficients<float>, 4>  AA_4X_OVERSAMPLE_COEFF_44K =
{bricks::BiquadCoefficients<float>{-1.67276810e+00,  7.25848529e-01,  1.22968334e-03,  1.07652579e-03,  1.22968334e-03,},
 bricks::BiquadCoefficients<float>{ -1.61048977e+00,  8.24893014e-01, 1.00000000e+00, -9.05136372e-01,  1.00000000e+00},
 bricks::BiquadCoefficients<float>{ -1.55698028e+00,  9.19905292e-01, 1.00000000e+00, -1.30406849e+00,  1.00000000e+00},
 bricks::BiquadCoefficients<float>{ -1.54300544e+00,  9.78180180e-01, 1.00000000e+00, -1.40540566e+00,  1.00000000e+00,}};


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

    float gain = 0.7f;
    float clip_level = 0.7f;
    WtOscillatorBrick<OVERSAMPLE>                 osc;
    SaturationBrick<ClipType::SOFT, OVERSAMPLE>   clipper;
    MultiStageFilterBrick<4, OVERSAMPLE>          aa_filter;
    DownSampleBrick<OVERSAMPLE, 1>                downsampler;
    VcaBrick<Response::LINEAR>                    vca;

    osc.set_samplerate(EXAMPLE_SAMPLERATE);
    clipper.set_samplerate(EXAMPLE_SAMPLERATE);
    aa_filter.set_samplerate(EXAMPLE_SAMPLERATE);
    vca.set_samplerate(EXAMPLE_SAMPLERATE);

    aa_filter.set_coeffs(AA_4X_OVERSAMPLE_COEFF_44K);

    AudioOsBuffer<OVERSAMPLE> buffer_1;
    AudioOsBuffer<OVERSAMPLE> buffer_2;
    AudioOsBuffer<1>          buffer_3;
    AudioOsBuffer<1>          buffer_4;

    osc.set_waveform(Waveform::SAW);
    int samplecount = 0;
    int max_samples = SECONDS_TO_RENDER * EXAMPLE_SAMPLERATE;
    while (samplecount < max_samples)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        float pitch = static_cast<float_t>(samplecount) / max_samples;

        osc.render(pitch, buffer_1);;
        clipper.render(clip_level, buffer_1, buffer_2);
        aa_filter.render(buffer_2, buffer_1);
        downsampler.render(buffer_1, buffer_3);
        vca.render(gain, buffer_3, buffer_4);
        auto stop_time = std::chrono::high_resolution_clock::now();
        timing_records.push_back(stop_time - start_time);
        sf_writef_float(output_file, buffer_4.data(), buffer_4.size());
        samplecount += PROC_BLOCK_SIZE;
    }

    print_timings(timing_records);
    sf_close(output_file);
    return 0;
}