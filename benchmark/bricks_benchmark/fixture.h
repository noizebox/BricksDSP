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

/* Template on the type of brick to use, and the number of inputs */
template<typename T, int ctrl_inputs, int audio_inputs, AudioType audio_type>
static void BasicBrickBM(benchmark::State& state)
{
    std::array<float, 3> ctrl_signals;
    ctrl_signals.fill(0.0);
    bricks::AudioBuffer audio_buffer;
    auto audio_data = get_audio_data(audio_type);

    std::unique_ptr<T> test_module;
    if constexpr (audio_inputs == 0)
    {
        if constexpr (ctrl_inputs == 0)
        {
            test_module = std::make_unique<T>();
        }
        else if constexpr (ctrl_inputs == 1)
        {
            test_module = std::make_unique<T>(ctrl_signals[0]);
        }
        else if constexpr (ctrl_inputs == 2)
        {
            test_module = std::make_unique<T>(ctrl_signals[0], ctrl_signals[1]);
        }
        else if constexpr (ctrl_inputs == 3)
        {
            test_module = std::make_unique<T>(ctrl_signals[0], ctrl_signals[1], ctrl_signals[2]);
        }
        else if constexpr (ctrl_inputs == 4)
        {
            test_module = std::make_unique<T>(ctrl_signals[0], ctrl_signals[1], ctrl_signals[2], ctrl_signals[3]);
        }
    }
    else if constexpr (audio_inputs == 1)
    {
        if constexpr (ctrl_inputs == 0)
        {
            test_module = std::make_unique<T>(audio_buffer);
        }
        else if constexpr (ctrl_inputs == 1)
        {
            test_module = std::make_unique<T>(ctrl_signals[0], audio_buffer);
        }
        else if constexpr (ctrl_inputs == 2)
        {
            test_module = std::make_unique<T>(ctrl_signals[0], ctrl_signals[1], audio_buffer);
        }
        else if constexpr (ctrl_inputs == 3)
        {
            test_module = std::make_unique<T>(ctrl_signals[0], ctrl_signals[1], ctrl_signals[2], audio_buffer);
        }
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
        std::copy(audio_data->data() + samples, audio_data->data() + samples + bricks::PROC_BLOCK_SIZE, audio_buffer.data());
        for (auto& i : ctrl_signals)
        {
            i = static_cast<float>(samples) / TEST_AUDIO_DATA_SIZE;
        }
        /* Do procesing.
         * The setting up and copying of audio data above will be included in the total
         * measured time, though in practice it adds very little overhead and is negligible
         * for all but the simplest bricks */
        test_module->render();
    }
};

/* Template on the type of brick to use, and the number of inputs */
template<typename T, int ctrl_inputs, int audio_inputs, AudioType audio_type>
static void VarBrickBM(benchmark::State& state)
{
    std::array<float, ctrl_inputs> ctrl_values;
    ctrl_values.fill(0.0);
    std::array<ControlPort, ctrl_inputs> ctrl_signals;
    for (int i = 0; i < ctrl_inputs; ++i)
    {
        ctrl_signals[i] = ControlPort(ctrl_values[i]);
    }
    bricks::AudioBuffer audio_buffer;
    std::array<bricks::AudioPort, audio_inputs> audio_signals;
    audio_signals.fill(AudioPort(audio_buffer));
    auto audio_data = get_audio_data(audio_type);

    std::unique_ptr<T> test_module;
    if constexpr (audio_inputs == 0)
    {
        test_module = std::make_unique<T>(ctrl_signals);
    }
    else if constexpr (ctrl_inputs == 0)
    {
        test_module = std::make_unique<T>(audio_signals);
    }
    else if constexpr (audio_inputs > 0)
    {
        test_module = std::make_unique<T>(ctrl_signals, audio_signals);
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
        std::copy(audio_data->data() + samples, audio_data->data() + samples + bricks::PROC_BLOCK_SIZE, audio_buffer.data());
        for (auto& i : ctrl_values)
        {
            i = static_cast<float>(samples) / TEST_AUDIO_DATA_SIZE;
        }
        /* Do procesing.
         * The setting up and copying of audio data above will be included in the total
         * measured time, though in practice it adds very little overhead and is negligible
         * for all but the simplest bricks */
        test_module->render();
    }
};


} // end bricks_bench
#endif //BRICKS_DSP_FIXTURE_H
