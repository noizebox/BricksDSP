#ifndef BRICKS_DSP_FIXTURE_H
#define BRICKS_DSP_FIXTURE_H

#include <cmath>

#include <benchmark/benchmark.h>

#include "bricks_dsp/bricks.h"

namespace bricks_bench {
using namespace bricks;

constexpr int TEST_AUDIO_DATA_SIZE = 44100;
constexpr float TEST_SAMPLE_RATE = 44100;

extern bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* SILENCE_AUDIO;
extern bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* SINE_AUDIO;
extern bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* NOISE_AUDIO;

enum class AudioType
{
    SILENCE,
    SINE,
    NOISE
};

static bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* get_audio_data(AudioType type)
{
    switch (type)
    {
        case AudioType::SILENCE: return SILENCE_AUDIO;
        case AudioType::SINE: return SINE_AUDIO;
        case AudioType::NOISE: return NOISE_AUDIO;
        default: return SILENCE_AUDIO;
    }
}

static bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE> gen_silence_data()
{
    bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE> data;
    data.fill(0.0);
    return data;
}

static bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE> gen_sine_data()
{
    bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE> data;
    int phase = 0;
    for (auto& i : data)
    {
        i = std::sin(phase);
        phase += 0.1;
    }
    return data;
}

static bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE> gen_noise_data()
{
    bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE> data;
    for (auto& i : data)
    {
        i = static_cast<float>(std::rand()) / (2.0 * RAND_MAX) - 1.0;
    }
    return data;
}

/* No-op bricks to measure the benchmarking overhead, essentially subtract the
 * measured time from this from all brick benchmarks to measure the overhead */
class BaselineBrick : public bricks::DspBrick
{
public:
    BaselineBrick(ControlPort ctrl_1, ControlPort ctrl_2, AudioPort in) : _ctrl_1(ctrl_1),_ctrl_2(ctrl_2), _in(in) {}

    void render() override {};

private:
    ControlPort _ctrl_1;
    ControlPort _ctrl_2;
    AudioPort   _in;
};

class BaselineBrickCtrlOnly : public bricks::DspBrick
{
public:
    BaselineBrickCtrlOnly(ControlPort ctrl_1, ControlPort ctrl_2) : _ctrl_1(ctrl_1), _ctrl_2(ctrl_2) {}

    void render() override {};

private:
    ControlPort _ctrl_1;
    ControlPort _ctrl_2;
};

/* Expands arrays into variadic packs for passing to constructors */
template<typename T, int ctrl_inputs, int audio_inputs, size_t... Is, size_t... Isa>
std::unique_ptr<T> make_brick(const std::array<float, ctrl_inputs>& ctrl_args,
                              const std::array<AudioBuffer, audio_inputs>& audio_args,
                              std::index_sequence<Is...>, std::index_sequence<Isa...>)
{
    return std::make_unique<T>(std::get<Is>(ctrl_args) ..., std::get<Isa>(audio_args) ...);
}

/* Expands arrays into arrays of ControlPorts and AudioPorts for passing to constructors */
template<typename T, int ctrl_inputs, int audio_inputs, size_t... Is, size_t... Isa>
std::unique_ptr<T> make_brick_array_args(const std::array<float, ctrl_inputs>& ctrl_args,
                                         const std::array<AudioBuffer, audio_inputs>& audio_args,
                                         std::index_sequence<Is...>, std::index_sequence<Isa...>)
{
    auto c_arg = std::array<ControlPort, ctrl_inputs>{std::get<Is>(ctrl_args) ...};
    auto a_arg = std::array<AudioPort, audio_inputs>{std::get<Isa>(audio_args) ...};

    return std::make_unique<T>(c_arg, a_arg);
}

/* Generic test fixture that passes a given number of control and audio inputs to the
 * DspBrick to benchmark
 *
 * T            - DspBrick type to benchmark
 * ctrl_inputs  - Number of control inputs to the brick
 * audio_inputs - Number of audio inputs to the brick
 * audio_type   - The type of audio to pass through - if performance depends on branch pred. or similar
 * array_args   - if true, format the constructor arguments into arrays of ControlPorts and AudioPorts,
 *                Else, use variadic expansion for passing arguments */
template<typename T, int ctrl_inputs, int audio_inputs, AudioType audio_type, bool array_args = false>
static void BrickBM(benchmark::State& state)
{
    std::array<float, ctrl_inputs> ctrl_signals;
    std::array<bricks::AudioBuffer, audio_inputs> audio_signals;
    auto audio_data = get_audio_data(audio_type);

    ctrl_signals.fill(0.0);

    std::unique_ptr<DspBrick> test_module;
    if constexpr (array_args)
    {
        test_module = make_brick_array_args<T, ctrl_inputs, audio_inputs>(ctrl_signals, audio_signals,
                                                                          std::make_index_sequence<ctrl_inputs>{},
                                                                          std::make_index_sequence<audio_inputs>{});
    }
    else
    {
        test_module = make_brick<T, ctrl_inputs, audio_inputs>(ctrl_signals, audio_signals,
                                                               std::make_index_sequence<ctrl_inputs>{},
                                                               std::make_index_sequence<audio_inputs>{});
    }
    assert(test_module);
    test_module->set_samplerate(TEST_SAMPLE_RATE);

    int samples = 0;
    for (auto _ : state)
    {
        if (samples >= TEST_AUDIO_DATA_SIZE - bricks::PROC_BLOCK_SIZE)
        {
            samples = 0;
        }
        for (auto& i : audio_signals)
        {
            std::copy(audio_data->data() + samples, audio_data->data() + samples + bricks::PROC_BLOCK_SIZE, i.data());
        }
        for (auto& i : ctrl_signals)
        {
            i = static_cast<float>(samples) / TEST_AUDIO_DATA_SIZE;
        }
        /* Do processing.
         * The changing of control values and copying of audio data above will be included
         * in the total measured time, though in practice it adds very little overhead and
         * is negligible for all but the simplest bricks.
         * See timings for the BaselineBrick for an estimation of the overhead */
        test_module->render();
    }
};

} // end bricks_bench
#endif //BRICKS_DSP_FIXTURE_H
