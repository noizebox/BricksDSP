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
constexpr int OVERSAMPLE = 1;

using namespace bricks;

constexpr std::array<bricks::BiquadCoefficients<float>, 4>  AA_4X_OVERSAMPLE_COEFF_44K =
{bricks::BiquadCoefficients<float>{-1.67276810e+00,  7.25848529e-01,  1.22968334e-03,  1.07652579e-03,  1.22968334e-03,},
 bricks::BiquadCoefficients<float>{ -1.61048977e+00,  8.24893014e-01, 1.00000000e+00, -9.05136372e-01,  1.00000000e+00},
 bricks::BiquadCoefficients<float>{ -1.55698028e+00,  9.19905292e-01, 1.00000000e+00, -1.30406849e+00,  1.00000000e+00},
 bricks::BiquadCoefficients<float>{ -1.54300544e+00,  9.78180180e-01, 1.00000000e+00, -1.40540566e+00,  1.00000000e+00,}};

constexpr std::array<bricks::BiquadCoefficients<float>, 4>  AA_4X_OVERSAMPLE_COEFF_22K =
{bricks::BiquadCoefficients<float>{-1.6727681023e+00, 7.2584852947e-01, 1.2296833389e-03, 1.0765257937e-03, 1.2296833389e-03},
 bricks::BiquadCoefficients<float>{-1.6104897694e+00, 8.2489301402e-01, 1.0000000000e+00, -9.0513637204e-01, 1.0000000000e+00},
 bricks::BiquadCoefficients<float>{-1.5569802752e+00, 9.1990529158e-01, 1.0000000000e+00, -1.3040684925e+00, 1.0000000000e+00},
 bricks::BiquadCoefficients<float>{-1.5430054385e+00, 9.7818018028e-01, 1.0000000000e+00, -1.4054056592e+00, 1.0000000000e+00}};

constexpr std::array<bricks::BiquadCoefficients<float>, 4>  AA_4X_OVERSAMPLE_COEFF_88K =
{bricks::BiquadCoefficients<float>{-1.6564436043e+00, 6.9680497067e-01, 6.8730923389e-04, 2.4728734641e-04, 6.8730923389e-04},
 bricks::BiquadCoefficients<float>{-1.6887374695e+00, 7.9003842588e-01, 1.0000000000e+00, -1.3273377575e+00, 1.0000000000e+00},
 bricks::BiquadCoefficients<float>{-1.7284235935e+00, 8.9328801229e-01, 1.0000000000e+00, -1.6134634986e+00, 1.0000000000e+00},
 bricks::BiquadCoefficients<float>{-1.7684057286e+00, 9.6867459489e-01, 1.0000000000e+00, -1.6835489159e+00, 1.0000000000e+00}};

constexpr std::array<bricks::BiquadCoefficients<float>, 4>  AA_2X_OVERSAMPLE_COEFF_44K =
{bricks::BiquadCoefficients<float>{ -1.22411276, 0.47214926, 0.01021942,  0.01789365,  0.01021942},
 bricks::BiquadCoefficients<float>{-0.78314966,  0.69361109, 1.00000000e+00, 0.75768866,  1.00000000e+00},
 bricks::BiquadCoefficients<float>{-0.44616276, 0.87048105, 1.0000000000e+00, 0.21477974, 1.0000000000e+00},
 bricks::BiquadCoefficients<float>{-0.30950659, 0.96578163, 1.0000000000e+00, 0.02802799, 1.0000000000e+00}};

/*
array([[ 1.28228599e-03,  1.16931922e-03,  1.28228599e-03,
         1.00000000e+00, -1.66524310e+00,  7.20588237e-01],
       [ 1.00000000e+00, -8.68545349e-01,  1.00000000e+00,
         1.00000000e+00, -1.59827789e+00,  8.21664388e-01],
       [ 1.00000000e+00, -1.27749423e+00,  1.00000000e+00,
         1.00000000e+00, -1.54058822e+00,  9.18480120e-01],
       [ 1.00000000e+00, -1.38197717e+00,  1.00000000e+00,
         1.00000000e+00, -1.52475139e+00,  9.77795695e-01]])

[[ 0.01021942,  0.01789365,  0.01021942,  1.        , -1.22411276, 0.47214926],
       [ 1.        ,  0.75768866,  1.        ,  1.        , -0.78314966,  0.69361109],
       [ 1.        ,  0.21477974,  1.        ,  1.        , -0.44616276, 0.87048105],
       [ 1.        ,  0.02802799,  1.        ,  1.        , -0.30950659, 0.96578163]])

scipy.signal.ellip(8, 1, 70, 0.22, 'lowpass', False, 'sos')

*/


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

    float gain = 0.3f;
    float clip_level = 2.7f;
    //BlepOscillatorBrick<PolyBlepLagrange   , OVERSAMPLE>                 osc;
    WtOscillatorBrick<CubicInterpolation<float>, OVERSAMPLE>                 osc;
    SaturationBrick<ClipType::SOFT, OVERSAMPLE>   clipper;
    PipelinedFilterBrick<4, OVERSAMPLE>          aa_filter;
    DownSampleBrick<OVERSAMPLE, 1>                downsampler;
    VcaBrick<Response::LINEAR>                    vca;
    LfoBrick                                      lfo;

    auto now = std::chrono::high_resolution_clock::now();

    auto WT = create_wavetable(Waveform::SAW, 4096);
    auto wt_done = std::chrono::high_resolution_clock::now();
    std::cout << "Wavetable created: " << (wt_done - now).count() / 1000000.0 << "ms" << std::endl;

    osc.set_samplerate(EXAMPLE_SAMPLERATE);
    osc.set_wavetable(&WT);
    clipper.set_samplerate(EXAMPLE_SAMPLERATE);
    aa_filter.set_samplerate(EXAMPLE_SAMPLERATE);
    vca.set_samplerate(EXAMPLE_SAMPLERATE);

    aa_filter.set_coeffs(AA_2X_OVERSAMPLE_COEFF_44K);

    AudioOsBuffer<OVERSAMPLE> buffer_1;
    AudioOsBuffer<OVERSAMPLE> buffer_2;
    AudioOsBuffer<1>          buffer_3;
    AudioOsBuffer<1>          buffer_4;

    osc.set_waveform(Waveform::PULSE);
    int samplecount = 0;
    int max_samples = SECONDS_TO_RENDER * EXAMPLE_SAMPLERATE;
    while (samplecount < max_samples)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        float pitch = static_cast<float_t>(samplecount) / max_samples;
        float lfo_out;
        lfo.render(0.3, lfo_out);

        //osc.render(pitch, lfo_out * 0.8, buffer_1);
        osc.render(pitch, buffer_1);
        //clipper.render(clip_level, buffer_1, buffer_2);
        aa_filter.render(buffer_1, buffer_2);
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