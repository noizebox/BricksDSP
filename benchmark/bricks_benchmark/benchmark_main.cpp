#include <benchmark/benchmark.h>

#include "fixture.h"

using namespace bricks_bench;

static auto silence_audio = gen_silence_data();
static auto sine_audio = gen_sine_data();
static auto noise_audio = gen_noise_data();

bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* bricks_bench::SILENCE_AUDIO = &silence_audio;
bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* bricks_bench::SINE_AUDIO = &sine_audio;
bricks::AlignedArray<float, TEST_AUDIO_DATA_SIZE>* bricks_bench::NOISE_AUDIO = &noise_audio;

constexpr bool FIXED_CTRL_DATA = false;
constexpr bool MODULATE_CTRL_DATA = true;

/* Baseline (1 audio input)*/
BENCHMARK_TEMPLATE(BrickBM, bricks_bench::BaselineBrick, 2, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(BrickBM, bricks_bench::BaselineBrickCtrlOnly, 2, 0, AudioType::SILENCE);

/* Up/downsample functions */
SAMPLE_FUNCTOR(SkipDownsampleFunc, bricks::skip_downsample, 4, 1);
BENCHMARK_TEMPLATE(FunBM, SkipDownsampleFunc ,4, 1);
SAMPLE_FUNCTOR(ZeroStuffUpsampleFunc, bricks::zero_stuff_upsample, 1, 4);
BENCHMARK_TEMPLATE(FunBM, ZeroStuffUpsampleFunc, 1, 4);
SAMPLE_FUNCTOR_MEM(LinearUpsampleFunc, bricks::linear_upsample, 1, 4, float);
BENCHMARK_TEMPLATE(FunBM, LinearUpsampleFunc, 1, 4, true);
SAMPLE_FUNCTOR_MEM(LinearUpsampleCleverFunc, bricks::linear_upsample_clever, 1, 4, float);
BENCHMARK_TEMPLATE(FunBM, LinearUpsampleCleverFunc, 1, 4, true);
using CubicMem = AlignedArray<float, 4>;
SAMPLE_FUNCTOR_MEM(CubicUpsampleFunc, bricks::cubic_upsample, 1, 4, CubicMem);
BENCHMARK_TEMPLATE(FunBM, CubicUpsampleFunc, 1, 4, true, AlignedArray<float,4>);

/* Envelope bricks */
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioRateADSRBrick<>, 4, 0, 0, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::LinearADSREnvelopeBrick, 4, 1, 0, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioADSREnvelopeBrick, 4, 1, 0, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::LfoBrick, 1, 1, 0, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::SineLfoBrick, 1, 1, 0, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::RandLfoBrick, 0, 1, 0, 0, AudioType::SILENCE);

/* Filter bricks */
BENCHMARK_TEMPLATE(Brick2BM, bricks::FixedFilterBrick<>,0, 0, 1, 1,  AudioType::SILENCE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::FixedFilterBrick<>,0, 0, 1, 1,  AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::FixedFilterBrick<>,0, 0, 1, 1,  AudioType::SINE);

BENCHMARK_TEMPLATE(Brick2BM, bricks::MultiStageFilterBrick<1, 1, float>,0, 0, 1, 1,  AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MultiStageFilterBrick<2, 1, float>,0, 0, 1, 1,  AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MultiStageFilterBrick<4, 1, float>,0, 0, 1, 1,  AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MultiStageFilterBrick<8, 1, float>,0, 0, 1, 1,  AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MultiStageFilterBrick<1, 1, double>,0, 0, 1, 1,  AudioType::NOISE);

BENCHMARK_TEMPLATE(Brick2BM, bricks::PipelinedFilterBrick<2>, 0, 0, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::PipelinedFilterBrick<4>, 0, 0, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::PipelinedFilterBrick<8>, 0, 0, 1, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(Brick2BM, bricks::ParallelFilterBrick<1>, 0, 0, 1, 1, AudioType::NOISE, CallType::AUDIO_INPUTS_OUTPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ParallelFilterBrick<2>, 0, 0, 2, 2, AudioType::NOISE, CallType::AUDIO_INPUTS_OUTPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ParallelFilterBrick<4>, 0, 0, 4, 4, AudioType::NOISE, CallType::AUDIO_INPUTS_OUTPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ParallelFilterBrick<8>, 0, 0, 8, 8, AudioType::NOISE, CallType::AUDIO_INPUTS_OUTPUTS_ARE_ARRAYS);

BENCHMARK_TEMPLATE(Brick2BM, bricks::SVFFilterBrick<>, 2, 0, 1, 3, AudioType::SILENCE, CallType::ALL_SINGLE_ARGS, MODULATE_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::SVFFilterBrick<>, 2, 0, 1, 3, AudioType::SILENCE, CallType::ALL_SINGLE_ARGS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::SVFFilterBrick<>, 2, 0, 1, 3, AudioType::SINE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::SVFFilterBrick<>, 2, 0, 1, 3, AudioType::NOISE);

BENCHMARK_TEMPLATE(Brick2BM, bricks::MystransLadderFilter<>, 2, 0, 1, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MystransLadderFilter<>, 2, 0, 1, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MystransLadderFilter<>, 2, 0, 1, 1, AudioType::NOISE);

/* Modulator bricks */
BENCHMARK_TEMPLATE(Brick2BM, bricks::SaturationBrick<ClipType::HARD>, 1, 0, 1, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::SaturationBrick<ClipType::SOFT>, 1, 0, 1, 1, AudioType::SINE);

BENCHMARK_TEMPLATE(Brick2BM, bricks::AASaturationBrick<ClipType::HARD>, 1, 0, 1, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AASaturationBrick<ClipType::HARD>, 1, 0, 1, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AASaturationBrick<ClipType::HARD>, 1, 0, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AASaturationBrick<ClipType::SOFT>, 1, 0, 1, 1, AudioType::SILENCE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AASaturationBrick<ClipType::SOFT>, 1, 0, 1, 1, AudioType::SINE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AASaturationBrick<ClipType::SOFT>, 1, 0, 1, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(Brick2BM, bricks::SustainerBrick<>, 4, 0, 2, 2, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::SustainerBrick<>, 4, 0, 2, 2, AudioType::SINE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::SustainerBrick<>, 4, 0, 2, 2, AudioType::SILENCE);

BENCHMARK_TEMPLATE(Brick2BM, bricks::FixedDelayBrick<ZerothInterpolation<float>>, 1, 0, 1, 1,  AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::FixedDelayBrick<LinearInterpolation<float>>, 1, 0, 1, 1,  AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::FixedDelayBrick<CubicInterpolation<float>>, 1, 0, 1, 1,  AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::FixedDelayBrick<CRCubicInterpolation<float>>, 1, 0, 1, 1,  AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::FixedDelayBrick<AllpassInterpolation<float>>, 1, 0, 1, 1,  AudioType::NOISE);

BENCHMARK_TEMPLATE(Brick2BM, bricks::ModDelayBrick<ZerothInterpolation<float>>, 1, 0, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ModDelayBrick<LinearInterpolation<float>>, 1, 0, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ModDelayBrick<CubicInterpolation<float>>, 1, 0, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ModDelayBrick<CRCubicInterpolation<float>>, 1, 0, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ModDelayBrick<AllpassInterpolation<float>>, 1, 0, 1, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(Brick2BM, bricks::ModulatedDelayBrick<>, 1, 0, 1, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(Brick2BM, bricks::AllpassDelayBrick<500>, 2, 0, 1, 1, AudioType::NOISE);

BENCHMARK_TEMPLATE(Brick2BM, bricks::BitRateReducerBrick<>, 1, 0, 1, 1, AudioType::NOISE, CallType::ALL_SINGLE_ARGS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::BitRateReducerBrick<>, 1, 0, 1, 1, AudioType::NOISE, CallType::ALL_SINGLE_ARGS, MODULATE_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::SampleRateReducerBrick<>, 1, 0, 1, 1, AudioType::NOISE, CallType::ALL_SINGLE_ARGS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::SampleRateReducerBrick<>, 1, 0, 1, 1, AudioType::NOISE, CallType::ALL_SINGLE_ARGS, MODULATE_CTRL_DATA);

/* Oscillator bricks */
BENCHMARK_TEMPLATE(Brick2BM, bricks::OscillatorBrick<>, 1, 0, 0, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::FmOscillatorBrick<>, 1, 0, 1, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::WtOscillatorBrick<>, 1, 0, 0, 1, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::NoiseGeneratorBrick<>, 0, 0, 0, 1, AudioType::NOISE);

/* Analysis bricks */
BENCHMARK_TEMPLATE(Brick2BM, bricks::MeterBrick<1>, 0, 2, 1, 0, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MeterBrick<1>, 0, 2, 1, 0, AudioType::SINE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MeterBrick<1>, 0, 2, 1, 0, AudioType::SILENCE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MeterBrick<2>, 0, 2, 1, 0, AudioType::NOISE);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MeterBrick<4>, 0, 2, 1, 0, AudioType::NOISE);

/* Utility bricks */
BENCHMARK_TEMPLATE(Brick2BM, bricks::VcaBrick<Response::LINEAR>, 1, 0, 1, 1, AudioType::NOISE, CallType::ALL_SINGLE_ARGS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::VcaBrick<Response::LINEAR>, 1, 0, 1, 1, AudioType::NOISE, CallType::ALL_SINGLE_ARGS, MODULATE_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::VcaBrick<Response::LOG>, 1, 0, 1, 1, AudioType::NOISE, CallType::ALL_SINGLE_ARGS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::VcaBrick<Response::LOG>, 1, 0, 1, 1, AudioType::NOISE, CallType::ALL_SINGLE_ARGS, MODULATE_CTRL_DATA);

BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMixerBrick<4, Response::LOG>,     4, 0, 4, 1, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, MODULATE_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMixerBrick<4, Response::LINEAR>,  4, 0, 4, 1, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, MODULATE_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMixerBrick<4, Response::LOG>,     4, 0, 4, 1, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMixerBrick<4, Response::LINEAR>,  4, 0, 4, 1, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMixerBrick<8, Response::LOG>,     8, 0, 8, 1, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, MODULATE_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMixerBrick<8, Response::LINEAR>,  8, 0, 8, 1, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, MODULATE_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMixerBrick<8, Response::LOG>,     8, 0, 8, 1, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMixerBrick<8, Response::LINEAR>,  8, 0, 8, 1, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMixerBrick<16, Response::LOG>,   16, 0,16, 1, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, MODULATE_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMixerBrick<16, Response::LINEAR>,16, 0,16, 1, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, MODULATE_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMixerBrick<16, Response::LOG>,   16, 0,16, 1, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMixerBrick<16, Response::LINEAR>,16, 0,16, 1, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, FIXED_CTRL_DATA);

BENCHMARK_TEMPLATE(Brick2BM, bricks::StereoMixerBrick<4, Response::LINEAR>, 8, 0, 4, 2, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::StereoMixerBrick<4, Response::LOG>, 8, 0, 4, 2, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::StereoMixerBrick<4, Response::LOG>, 8, 0, 4, 2,AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::StereoMixerBrick<8, Response::LOG>, 16, 0, 8, 2, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::StereoMixerBrick<8, Response::LOG>, 16, 0, 8, 2, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, FIXED_CTRL_DATA);
BENCHMARK_TEMPLATE(Brick2BM, bricks::StereoMixerBrick<16, Response::LOG>, 32, 0, 16, 2, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::StereoMixerBrick<16, Response::LOG>, 32, 0, 16, 2, AudioType::NOISE, CallType::CTRL_INPUTS_AUDIO_INPUTS_ARE_ARRAYS, FIXED_CTRL_DATA);

BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioSummerBrick<4>,  0, 0, 4, 1, AudioType::NOISE, CallType::AUDIO_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioSummerBrick<8>,  0, 0, 8, 1, AudioType::NOISE, CallType::AUDIO_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioSummerBrick<16>, 0, 0, 16, 1,AudioType::NOISE, CallType::AUDIO_INPUTS_ARE_ARRAYS);

BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMultiplierBrick<2>,  0, 0, 2, 1, AudioType::NOISE, CallType::AUDIO_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMultiplierBrick<4>,  0, 0, 4, 1, AudioType::NOISE, CallType::AUDIO_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMultiplierBrick<8>,  0, 0, 8, 1, AudioType::NOISE, CallType::AUDIO_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::AudioMultiplierBrick<16>, 0, 0, 16, 1,AudioType::NOISE, CallType::AUDIO_INPUTS_ARE_ARRAYS);

BENCHMARK_TEMPLATE(Brick2BM, bricks::ControlMixerBrick<4>,  8, 1, 0, 0, AudioType::SILENCE, CallType::CTRL_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ControlMixerBrick<8>, 16, 1, 0, 0, AudioType::SILENCE, CallType::CTRL_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ControlMixerBrick<16>,32, 1, 0, 0, AudioType::SILENCE, CallType::CTRL_INPUTS_ARE_ARRAYS);

BENCHMARK_TEMPLATE(Brick2BM, bricks::ControlSummerBrick<4>,  4, 1, 0, 0, AudioType::SILENCE, CallType::CTRL_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ControlSummerBrick<8>,  8, 1, 0, 0, AudioType::SILENCE, CallType::CTRL_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ControlSummerBrick<16>, 16, 1, 0, 0,AudioType::SILENCE, CallType::CTRL_INPUTS_ARE_ARRAYS);

BENCHMARK_TEMPLATE(Brick2BM, bricks::ControlMultiplierBrick<2>,  2, 1, 0, 0, AudioType::SILENCE, CallType::CTRL_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ControlMultiplierBrick<4>,  4, 1, 0, 0, AudioType::SILENCE, CallType::CTRL_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ControlMultiplierBrick<8>,  8, 1, 0, 0, AudioType::SILENCE, CallType::CTRL_INPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::ControlMultiplierBrick<16>,16, 1, 0, 0, AudioType::SILENCE, CallType::CTRL_INPUTS_ARE_ARRAYS);

BENCHMARK_TEMPLATE(Brick2BM, bricks::MetaControlBrick<2, 4, true>,  2, 4, 0, 0, AudioType::NOISE, CallType::CTRL_INPUTS_OUTPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MetaControlBrick<2, 4, false>, 2, 4, 0, 0, AudioType::NOISE, CallType::CTRL_INPUTS_OUTPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MetaControlBrick<4, 8, true>,  4, 8, 0, 0, AudioType::NOISE, CallType::CTRL_INPUTS_OUTPUTS_ARE_ARRAYS);
BENCHMARK_TEMPLATE(Brick2BM, bricks::MetaControlBrick<4, 8, false>, 4, 8, 0, 0, AudioType::NOISE, CallType::CTRL_INPUTS_OUTPUTS_ARE_ARRAYS);

BENCHMARK_MAIN();