#include <benchmark/benchmark.h>

#include "fixture.h"

using namespace bricks_bench;

static auto silence_audio = gen_silence_data();
static auto sine_audio = gen_sine_data();
static auto noise_audio = gen_noise_data();

bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* bricks_bench::SILENCE_AUDIO = &silence_audio;
bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* bricks_bench::SINE_AUDIO = &sine_audio;
bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* bricks_bench::NOISE_AUDIO = &noise_audio;

constexpr bool PASS_ARRAY_ARGS = true;

/* Baseline (1 audio input)*/
BENCHMARK_TEMPLATE(BrickBM, bricks_bench::BaselineBrick, 2, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks_bench::BaselineBrickCtrlOnly, 2, 0, AudioType::SILENCE);

/* Envelope bricks */
BENCHMARK_TEMPLATE(BrickBM, bricks::AudioRateADSRBrick, 4, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::LinearADSREnvelopeBrick, 4, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::AudioADSREnvelopeBrick, 4, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::LfoBrick, 1, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::SineLfoBrick, 1, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::RandLfoBrick, 1, 0, AudioType::SILENCE);

/* Filter bricks */
BENCHMARK_TEMPLATE(BrickBM, bricks::FixedFilterBrick, 0, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::FixedFilterBrick, 0, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::FixedFilterBrick, 0, 1, AudioType::SINE);

BENCHMARK_TEMPLATE(BrickBM, bricks::BiquadFilterBrick, 3, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::BiquadFilterBrick, 3, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BrickBM, bricks::BiquadFilterBrick, 3, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(BrickBM, bricks::SVFFilterBrick, 2, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::SVFFilterBrick, 2, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BrickBM, bricks::SVFFilterBrick, 2, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(BrickBM, bricks::MystransLadderFilter, 2, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::MystransLadderFilter, 2, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BrickBM, bricks::MystransLadderFilter, 2, 1, AudioType::NOISE);

/* Modulator bricks */
BENCHMARK_TEMPLATE(BrickBM, bricks::SaturationBrick<ClipType::HARD>, 1, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BrickBM, bricks::SaturationBrick<ClipType::SOFT>, 1, 1, AudioType::SINE);

BENCHMARK_TEMPLATE(BrickBM, bricks::AASaturationBrick<ClipType::HARD>, 1, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::AASaturationBrick<ClipType::HARD>, 1, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BrickBM, bricks::AASaturationBrick<ClipType::HARD>, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::AASaturationBrick<ClipType::SOFT>, 1, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::AASaturationBrick<ClipType::SOFT>, 1, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BrickBM, bricks::AASaturationBrick<ClipType::SOFT>, 1, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(BrickBM, bricks::SustainerBrick, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::SustainerBrick, 1, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BrickBM, bricks::SustainerBrick, 1, 1, AudioType::SILENCE);

BENCHMARK_TEMPLATE(BrickBM, bricks::FixedDelayBrick<InterpolationType::NONE>, 0, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::FixedDelayBrick<InterpolationType::LIN>, 0, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::FixedDelayBrick<InterpolationType::CUBIC>, 0, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::FixedDelayBrick<InterpolationType::CR_CUB>, 0, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(BrickBM, bricks::ModDelayBrick<InterpolationType::LIN>, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::ModDelayBrick<InterpolationType::CUBIC>, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::ModDelayBrick<InterpolationType::CR_CUB>, 1, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(BrickBM, bricks::ModulatedDelayBrick, 1, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(BrickBM, bricks::BitRateReducerBrick, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::SampleRateReducerBrick, 1, 1, AudioType::NOISE);

/* Oscillator bricks */
BENCHMARK_TEMPLATE(BrickBM, bricks::OscillatorBrick, 1, 0, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::FmOscillatorBrick, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::WtOscillatorBrick, 1, 0, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::NoiseGeneratorBrick, 0, 0, AudioType::NOISE);

/* Utility bricks */
BENCHMARK_TEMPLATE(BrickBM, bricks::VcaBrick<Response::LINEAR>, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::VcaBrick<Response::LOG>, 1, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(BrickBM, bricks::AudioMixerBrick<4, Response::LINEAR>, 4, 4, AudioType::NOISE, PASS_ARRAY_ARGS);
BENCHMARK_TEMPLATE(BrickBM, bricks::AudioMixerBrick<4, Response::LOG>, 4, 4, AudioType::NOISE, PASS_ARRAY_ARGS);
BENCHMARK_TEMPLATE(BrickBM, bricks::AudioMixerBrick<8, Response::LINEAR>, 8, 8, AudioType::NOISE, PASS_ARRAY_ARGS);
BENCHMARK_TEMPLATE(BrickBM, bricks::AudioMixerBrick<8, Response::LOG>, 8, 8, AudioType::NOISE, PASS_ARRAY_ARGS);
BENCHMARK_TEMPLATE(BrickBM, bricks::AudioMixerBrick<16, Response::LINEAR>, 16, 16, AudioType::NOISE, PASS_ARRAY_ARGS);
BENCHMARK_TEMPLATE(BrickBM, bricks::AudioMixerBrick<16, Response::LOG>, 16, 16, AudioType::NOISE, PASS_ARRAY_ARGS);

BENCHMARK_TEMPLATE(BrickBM, bricks::AudioSummerBrick<4>, 0, 4, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::AudioSummerBrick<8>, 0, 8, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::AudioSummerBrick<16>, 0, 16, AudioType::NOISE);

BENCHMARK_TEMPLATE(BrickBM, bricks::AudioMultiplierBrick<2>, 0, 2, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::AudioMultiplierBrick<4>, 0, 4, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::AudioMultiplierBrick<8>, 0, 8, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::AudioMultiplierBrick<16>, 0, 16, AudioType::NOISE);

BENCHMARK_TEMPLATE(BrickBM, bricks::ControlMixerBrick<4>, 8, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::ControlMixerBrick<8>, 16, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::ControlMixerBrick<16>, 32, 0, AudioType::SILENCE);

BENCHMARK_TEMPLATE(BrickBM, bricks::ControlSummerBrick<4>, 4, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::ControlSummerBrick<8>, 8, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::ControlSummerBrick<16>, 16, 0, AudioType::SILENCE);

BENCHMARK_TEMPLATE(BrickBM, bricks::ControlMultiplierBrick<2>, 2, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::ControlMultiplierBrick<4>, 4, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::ControlMultiplierBrick<8>, 8, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks::ControlMultiplierBrick<16>, 16, 0, AudioType::SILENCE);

BENCHMARK_TEMPLATE(BrickBM, bricks::MetaControlBrick<2, 4, true>, 2, 0, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::MetaControlBrick<2, 4, false>, 2, 0, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::MetaControlBrick<4, 8, true>, 4, 0, AudioType::NOISE);
BENCHMARK_TEMPLATE(BrickBM, bricks::MetaControlBrick<4, 8, false>, 4, 0, AudioType::NOISE);

BENCHMARK_MAIN();