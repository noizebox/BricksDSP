#include <benchmark/benchmark.h>

#include "fixture.h"

using namespace bricks_bench;

static auto silence_audio = gen_silence_data();
static auto sine_audio = gen_sine_data();
static auto noise_audio = gen_noise_data();

bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* bricks_bench::SILENCE_AUDIO = &silence_audio;
bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* bricks_bench::SINE_AUDIO = &sine_audio;
bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* bricks_bench::NOISE_AUDIO = &noise_audio;

/* Envelope bricks */
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::AudioRateADSRBrick, 4, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::LinearADSREnvelopeBrick, 4, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::AudioADSREnvelopeBrick, 4, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::LfoBrick, 1, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::SineLfoBrick, 1, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::RandLfoBrick, 1, 0, AudioType::SILENCE);

/* Filter bricks */
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::FixedFilterBrick, 0, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::FixedFilterBrick, 0, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::FixedFilterBrick, 0, 1, AudioType::SINE);

BENCHMARK_TEMPLATE(BasicBrickBM, bricks::BiquadFilterBrick, 3, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::BiquadFilterBrick, 3, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::BiquadFilterBrick, 3, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(BasicBrickBM, bricks::SVFFilterBrick, 2, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::SVFFilterBrick, 2, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::SVFFilterBrick, 2, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(BasicBrickBM, bricks::MystransLadderFilter, 2, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::MystransLadderFilter, 2, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::MystransLadderFilter, 2, 1, AudioType::NOISE);

/* Modulator bricks */
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::SaturationBrick<ClipType::HARD>, 1, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::SaturationBrick<ClipType::SOFT>, 1, 1, AudioType::SINE);

BENCHMARK_TEMPLATE(BasicBrickBM, bricks::AASaturationBrick<ClipType::HARD>, 1, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::AASaturationBrick<ClipType::HARD>, 1, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::AASaturationBrick<ClipType::HARD>, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::AASaturationBrick<ClipType::SOFT>, 1, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::AASaturationBrick<ClipType::SOFT>, 1, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::AASaturationBrick<ClipType::SOFT>, 1, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(BasicBrickBM, bricks::FixedDelayBrick<InterpolationType::NONE>, 0, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::FixedDelayBrick<InterpolationType::LIN>, 0, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::FixedDelayBrick<InterpolationType::CUBIC>, 0, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::FixedDelayBrick<InterpolationType::CR_CUB>, 0, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(BasicBrickBM, bricks::ModDelayBrick<InterpolationType::NONE>, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::ModDelayBrick<InterpolationType::LIN>, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::ModDelayBrick<InterpolationType::CUBIC>, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::ModDelayBrick<InterpolationType::CR_CUB>, 1, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(BasicBrickBM, bricks::ModulatedDelayBrick, 1, 1, AudioType::NOISE);

/* Oscillator bricks */
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::OscillatorBrick, 1, 0, AudioType::NOISE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::FmOscillatorBrick, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::WtOscillatorBrick, 1, 0, AudioType::NOISE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::NoiseGeneratorBrick, 0, 0, AudioType::NOISE);

/* Utility bricks */
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::VcaBrick<Response::LINEAR>, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(BasicBrickBM, bricks::VcaBrick<Response::LOG>, 1, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(VarBrickBM, bricks::AudioMixerBrick<4, Response::LINEAR>, 4, 4, AudioType::NOISE);
BENCHMARK_TEMPLATE(VarBrickBM, bricks::AudioMixerBrick<4, Response::LOG>, 4, 4, AudioType::NOISE);
BENCHMARK_TEMPLATE(VarBrickBM, bricks::AudioMixerBrick<8, Response::LINEAR>, 8, 8, AudioType::NOISE);
BENCHMARK_TEMPLATE(VarBrickBM, bricks::AudioMixerBrick<8, Response::LOG>, 8, 8, AudioType::NOISE);
BENCHMARK_TEMPLATE(VarBrickBM, bricks::AudioMixerBrick<16, Response::LINEAR>, 16, 16, AudioType::NOISE);
BENCHMARK_TEMPLATE(VarBrickBM, bricks::AudioMixerBrick<16, Response::LOG>, 16, 16, AudioType::NOISE);

//BENCHMARK_TEMPLATE(VarBrickBM, bricks::AudioSummerBrick<4>, 0, 4, AudioType::NOISE);
//BENCHMARK_TEMPLATE(VarBrickBM, bricks::AudioSummerBrick<8>, 0, 8, AudioType::NOISE);
//BENCHMARK_TEMPLATE(VarBrickBM, bricks::AudioSummerBrick<16>, 0, 16, AudioType::NOISE);


BENCHMARK_MAIN();