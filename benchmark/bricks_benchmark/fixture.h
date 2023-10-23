#ifndef BRICKS_DSP_FIXTURE_H
#define BRICKS_DSP_FIXTURE_H

#include <cmath>

#include <benchmark/benchmark.h>

#ifdef __SSE__
#include <xmmintrin.h>
#define denormals_intrinsic() _mm_setcsr(0x9FC0)
#else
#define denormals_intrinsic()
#endif
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
class BaselineBrick : public bricks::DspBrickImpl<2, 0, 1, 1>
{
public:
    BaselineBrick(const float* ctrl_1, const float* ctrl_2, const AudioBuffer* in)
    {
        set_control_input(0, ctrl_1);
        set_control_input(1, ctrl_2);
        set_audio_input(0, in);
    }

    void render() override {};
};

class BaselineBrickCtrlOnly : public bricks::DspBrickImpl<2, 2, 0, 0>
{
public:
    BaselineBrickCtrlOnly(const float* ctrl_1, const float* ctrl_2)
    {
        set_control_input(0, ctrl_1);
        set_control_input(1, ctrl_2);
    }

    void render() override {};
};

/* Expands arrays into variadic packs for passing to constructors */
template<typename T, int ctrl_inputs, int audio_inputs, size_t... Is, size_t... Isa>
std::unique_ptr<T> make_brick(const std::array<float, ctrl_inputs>& ctrl_args,
                              const std::array<bricks::AudioBuffer, audio_inputs>& audio_args,
                              std::index_sequence<Is...>, std::index_sequence<Isa...>)
{
    return std::make_unique<T>(&std::get<Is>(ctrl_args) ..., &std::get<Isa>(audio_args) ...);
}

/* Expands arrays into arrays of pointers for passing to constructors */
template<typename T, int ctrl_inputs, int audio_inputs, size_t... Is, size_t... Isa>
std::unique_ptr<T> make_brick_array_args(const std::array<float, ctrl_inputs>& ctrl_args,
                                         const std::array<AudioBuffer, audio_inputs>& audio_args,
                                         std::index_sequence<Is...>, std::index_sequence<Isa...>)
{
    auto c_arg = std::array<const float*, ctrl_inputs>{&std::get<Is>(ctrl_args) ...};
    auto a_arg = std::array<const AudioBuffer*, audio_inputs>{&std::get<Isa>(audio_args) ...};

    return std::make_unique<T>(c_arg, a_arg);
}

/* Generic test fixture that passes a given number of control and audio inputs to the
 * DspBrick to benchmark
 *
 * T                  - DspBrick type to benchmark
 * ctrl_inputs        - Number of control inputs to the brick
 * audio_inputs       - Number of audio inputs to the brick
 * audio_type         - The type of audio to pass through - if performance depends on branch pred. or similar
 * array_args         - If true, format the constructor arguments into arrays of ControlPorts and AudioPorts,
 *                      Else, use variadic expansion for passing arguments
 * modulate_ctrl_data - If true, modulate control data between calls to render */
template<typename T, int ctrl_inputs, int audio_inputs, AudioType audio_type, bool array_args = false, bool modulate_ctrl_data = true>
static void BrickBM(benchmark::State& state)
{
    denormals_intrinsic();

    std::array<float, ctrl_inputs> ctrl_signals;
    std::array<bricks::AudioBuffer, audio_inputs> audio_signals;
    auto audio_data = get_audio_data(audio_type);

    ctrl_signals.fill(0.0);
    constexpr float CTRL_INC = 1.0f / TEST_AUDIO_DATA_SIZE;

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
        if (samples++ >= TEST_AUDIO_DATA_SIZE - bricks::PROC_BLOCK_SIZE)
        {
            samples = 0;
        }
        for (auto& i : audio_signals)
        {
            std::copy(audio_data->data() + samples, audio_data->data() + samples + bricks::PROC_BLOCK_SIZE, i.data());
        }
        if constexpr (modulate_ctrl_data)
        {
            for (auto& i: ctrl_signals)
            {
                i = static_cast<float>(samples) * CTRL_INC;
            }
        }
        /* Do processing.
         * The changing of control values and copying of audio data above will be included
         * in the total measured time, though in practice it adds very little overhead and
         * is negligible for all but the simplest bricks.
         * See timings for the BaselineBrick for an estimation of the overhead */
        test_module->render();
    }
};

/* Creates a functor object from free-standing function fun, for use in FunBM template */
#define SAMPLE_FUNCTOR(name, fun, from_size, to_size) \
struct name                             \
{                                                     \
    void operator () (const AlignedArray<float, from_size*bricks::PROC_BLOCK_SIZE>& audio_in, AlignedArray<float, to_size*bricks::PROC_BLOCK_SIZE>& audio_out) \
    {                                                 \
        fun<from_size*bricks::PROC_BLOCK_SIZE, to_size*bricks::PROC_BLOCK_SIZE>(audio_in, audio_out);                     \
    }                                                 \
};

#define SAMPLE_FUNCTOR_MEM(name, fun, from_size, to_size, mem_type) \
struct name                             \
{                                                     \
    void operator () (const AlignedArray<float, from_size*bricks::PROC_BLOCK_SIZE>& audio_in, AlignedArray<float, to_size*bricks::PROC_BLOCK_SIZE>& audio_out, mem_type& memory) \
    {                                                 \
        fun<from_size*bricks::PROC_BLOCK_SIZE, to_size*bricks::PROC_BLOCK_SIZE>(audio_in, audio_out, memory);                     \
    }                                                 \
};

/* Generic test fixture for free-standing functions that take sample arrays of different sizes
 *
 * SampleFun        - Functon created with SAMPLE_FUNCTOR or SAMPLE_FUNCTOR_MEM
 * from_size        - Size of input sample array in multiplies of bricks::PROC_BLOCK_SIZE
 * to_size          - Size of output sample array in multiplies of bricks::PROC_BLOCK_SIZE
 * memory_arg       - If true, uses a functor with extra memory argument
 * mem_type         - Type of the above memory argument */

template <typename SampleFun, int from_size, int to_size, bool memory_arg=false, typename mem_type=float>
static void FunBM(benchmark::State& state)
{
    denormals_intrinsic();

    bricks::AlignedArray<float, from_size * bricks::PROC_BLOCK_SIZE> in_audio;
    bricks::AlignedArray<float, to_size * bricks::PROC_BLOCK_SIZE> out_audio;
    mem_type memory;
    auto audio_data = get_audio_data(AudioType::NOISE);
    SampleFun func;

    int samples = 0;
    for (auto _ : state)
    {
        if (samples++ >= TEST_AUDIO_DATA_SIZE - bricks::PROC_BLOCK_SIZE)
        {
            samples = 0;
        }
        std::copy(audio_data->data() + samples, audio_data->data() + samples + in_audio.size(), in_audio.data());
        /* Do processing.
         * The copying of audio data above will be included
         * in the total measured time, though in practice it adds very little overhead and
         * is negligible for all but the simplest bricks.
         * See timings for the BaselineBrick for an estimation of the overhead */
        if constexpr (memory_arg)
        {
            func(in_audio, out_audio, memory);
        }
        else
        {
            func(in_audio, out_audio);
        }
        benchmark::DoNotOptimize(out_audio[2]++);
    }
};

} // end bricks_bench
#endif //BRICKS_DSP_FIXTURE_H
