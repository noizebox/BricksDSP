#ifndef BRICKS_DSP_FILTER_BRICKS_H
#define BRICKS_DSP_FILTER_BRICKS_H

#include "dsp_brick.h"
namespace bricks {

#ifdef BRICKS_DSP_CONSTEXPR_MATH
constexpr float DEFAULT_Q = 1 / sqrtf(2.0f);
#else
constexpr float DEFAULT_Q = 1 / 1.42f;
#endif

template<typename FloatType>
struct BiquadCoefficients
{
    FloatType a1;
    FloatType a2;
    FloatType b0;
    FloatType b1;
    FloatType b2;
};

template<typename FloatType>
struct BiquadRegisters
{
    FloatType z1;
    FloatType z2;
};

/* Direct form 2 transposed biquad calculation */
template <typename FloatType>
inline float render_biquad_sample(float in,
                                  const BiquadCoefficients<FloatType>& coeff,
                                  BiquadRegisters<FloatType>& reg)
{
    FloatType out = in * coeff.b0 + reg.z1;
    reg.z1 = in * coeff.b1 + reg.z2 - coeff.a1 * out;
    reg.z2 = in * coeff.b2 - coeff.a2 * out;
    return out;
}

template <typename FloatType, int BlockSize>
void render_df2_biquad(const AlignedArray<float, BlockSize>& in,
                       AlignedArray<float, BlockSize>& out,
                       const BiquadCoefficients<FloatType>& coeff,
                       BiquadRegisters<FloatType>& registers)
{
    auto reg = registers;
    for (int i = 0; i < in.size(); ++i)
    {
        out[i] = render_biquad_sample(in[i], coeff, reg);
    }
    registers = reg;
}

using Coefficients = BiquadCoefficients<float>;
using Registers = BiquadRegisters<float>;

/* Standard Biquad with non-modulated filter parameters */
class FixedFilterBrick : public DspBrickImpl<0, 0, 1, 1>
{
public:
    enum AudioOutput
    {
        FILTER_OUT = 0
    };

    FixedFilterBrick(const AudioBuffer* audio_in)
    {
        set_audio_input(0, audio_in);
    }

    void set_coeffs(const Coefficients& coeffs)
    {
        _coeff = coeffs;
    }

    void render() override;

    void reset() override
    {
        _reg = {0, 0};
    }

private:
    Coefficients    _coeff{0,0,0,0,0};
    Registers       _reg{0,0};
};

/* Fixed Biquad with non-modulated filter parameters and templated number of stages */
template<int stages, typename FloatType = float>
class MultiStageFilterBrick : public DspBrickImpl<0, 0, 1, 1>
{
public:
    enum AudioOutput
    {
        FILTER_OUT = 0
    };

    MultiStageFilterBrick() = default;

    MultiStageFilterBrick(const AudioBuffer* audio_in)
    {
        set_audio_input(0, audio_in);
    }

    void set_coeffs(const std::array<BiquadCoefficients<FloatType>, stages>& coeffs)
    {
        _coeff = coeffs;
    }

    void render() override
    {
        const AudioBuffer& audio_in = _input_buffer(0);
        AudioBuffer& audio_out = _output_buffer(AudioOutput::FILTER_OUT);
        auto regs = _reg;
        /* Can't partially specialise a member function, this is a workaround */
        if constexpr (stages % 2 == 0)
        {
            /* With an even number of stages, first render to the buffer, then ping pong
             * between audio_out and buffer to avoid an extra copy in the end */
            AudioBuffer buffer;
            render_df2_biquad<FloatType, PROC_BLOCK_SIZE>(audio_in, buffer, _coeff[0], regs[0]);
            for (int i = 1; i < stages; ++i)
            {
                const AudioBuffer& in = i % 2 ? buffer : audio_out;
                AudioBuffer& out = i % 2 ? audio_out : buffer;
                render_df2_biquad<FloatType, PROC_BLOCK_SIZE>(in, out, _coeff[i], regs[i]);
            }
        }
        else
        {
            /* With an odd number of stages, first render to audio_out, then ping-pong
             * between buffer and audio out */
            AudioBuffer buffer;
            render_df2_biquad<FloatType, PROC_BLOCK_SIZE>(audio_in, audio_out, _coeff[0], regs[0]);
            for (int i = 1; i < stages; ++i)
            {
                const AudioBuffer& in = i % 2 ? audio_out: buffer;
                AudioBuffer& out = i % 2 ? buffer : audio_out;
                render_df2_biquad<FloatType, PROC_BLOCK_SIZE>(in, out, _coeff[i], regs[i]);
            }
        }
        _reg = regs;
    }

    void reset() override
    {
        _reg.fill({0, 0});
    }

private:
    std::array<BiquadCoefficients<FloatType>, stages>   _coeff;
    std::array<BiquadRegisters<FloatType>, stages>      _reg;
};

/* Fixed filter with non-modulated filter parameters and stages calculated
 * in parallel, adds 1 sample delay per stage but allows for much more
 * cpu-efficient processing */
template<int stages, typename FloatType = float>
class PipelinedFilterBrick : public DspBrickImpl<0, 0, 1, 1>
{
public:
    enum AudioOutput
    {
        FILTER_OUT = 0
    };

    PipelinedFilterBrick() = default;

    PipelinedFilterBrick(const AudioBuffer* audio_in)
    {
        set_audio_input(0, audio_in);
    }

    void set_coeffs(const std::array<BiquadCoefficients<FloatType>, stages>& coeffs)
    {
        _coeff = coeffs;
    }

    void render() override
    {
        const AudioBuffer& audio_in = _input_buffer(0);
        AudioBuffer& audio_out = _output_buffer(AudioOutput::FILTER_OUT);
        auto pipeline = _pipeline;
        auto regs = _reg;

        for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
        {
            pipeline[0] = audio_in[i];
            for (int s = 0; s < stages; ++s)
            {
                pipeline[s] = render_biquad_sample<FloatType>(pipeline[s], _coeff[s], regs[s]);
            }
            audio_out[i] = pipeline[stages - 1];
            // advance buffer 1 sample
            for (int s = stages - 1; s > 0 ; --s)
            {
                pipeline[s] = pipeline[s - 1];
            }
        }
        _pipeline = pipeline;
        _reg = regs;
    }

    void reset() override
    {
        _reg.fill({0, 0});
        _pipeline.fill(0);
    }

private:
    static_assert(stages >= 2, "Needs at least 2 stages to be useful");
    std::array<BiquadCoefficients<FloatType>, stages>   _coeff;
    std::array<BiquadRegisters<FloatType>, stages>      _reg;
    std::array<FloatType, stages>                       _pipeline;
};


/* Fixed filter with templated number of parallel paths.
 * More efficient for multiple voices/channels */
template<int channel_count, typename FloatType = float>
class ParallelFilterBrick : public DspBrickImpl<0, 0, channel_count, channel_count>
{
    using this_template = DspBrickImpl<0, 0, channel_count, channel_count>;

public:
    ParallelFilterBrick() = default;

    template <class ...T>
    explicit ParallelFilterBrick(T... inputs)
    {
        static_assert(sizeof...(inputs) == channel_count);
        std::array<const AudioBuffer*, channel_count> audio_ins = {{inputs...}};
        for (int i = 0; i < channel_count; ++i)
        {
            this_template::set_audio_input(i, audio_ins[i]);
        }
    }

    void set_coeffs(BiquadCoefficients<FloatType>& coeffs)
    {
        _coeff = coeffs;
    }

    void render() override
    {
        std::array<const AudioBuffer*, channel_count>  inputs;
        std::array<AudioBuffer*, channel_count>        outputs;

        for (int i = 0; i < channel_count; ++i)
        {
            inputs[i] = &this_template::_input_buffer(i);
            outputs[i] = &this_template::_output_buffer(i);
        }

        auto regs = _reg;

        for (int s = 0; s < PROC_BLOCK_SIZE; ++s)
        {
            for (int c = 0; c < channel_count; ++c)
            {
                outputs[c]->data()[s] = render_biquad_sample<FloatType>(inputs[c]->data()[s], _coeff, regs[c]);
            }
        }
        _reg = regs;
    }

    void reset() override
    {
        _reg.fill({0, 0});
    }

private:
    BiquadCoefficients<FloatType>   _coeff;
    std::array<BiquadRegisters<FloatType>, channel_count>    _reg;
};


/* State variable filter with multiple outs from Andrew Simper, Cytomic,
 * adapted from https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf */
class SVFFilterBrick : public DspBrickImpl<2, 0, 1, 3>
{
public:
    enum ControlInput
    {
        CUTOFF = 0,
        RESONANCE
    };

    enum AudioOutput
    {
        LOWPASS = 0,
        BANDPASS,
        HIGHPASS
    };

    SVFFilterBrick(const float* cutoff, const float* resonance, const AudioBuffer* audio_in)
    {
        set_control_input(ControlInput::CUTOFF, cutoff);
        set_control_input(ControlInput::RESONANCE, resonance);
        set_audio_input(0, audio_in);
    }

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override
    {
        _g_lag.reset();
        _reg = {0, 0};
    }

    void render() override;

private:
    float                 _samplerate_inv {1.0f / DEFAULT_SAMPLERATE};
    std::array<float, 2>  _reg{0,0};
    ControlSmootherLinear _g_lag;
};

/* Topology-preserving (zero delay) ladder with non-linearities
 * From https://www.kvraudio.com/forum/viewtopic.php?t=349859
 * and Copyright 2012 Teemu Voipio (mystran @ kvr)  */
class MystransLadderFilter : public DspBrickImpl<2, 0, 1, 1>
{
   public:
    enum ControlInput
    {
        CUTOFF = 0,
        RESONANCE,
    };

    enum AudioOutput
    {
        FILTER_OUT = 0,
    };

    MystransLadderFilter(const float* cutoff, const float* resonance, const AudioBuffer* audio_in)
    {
        set_control_input(ControlInput::CUTOFF, cutoff);
        set_control_input(ControlInput::RESONANCE, resonance);
        set_audio_input(0, audio_in);
    }

    void render() override;

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override
    {
        _freq_lag.reset();
        _states.fill(0);
    }

private:
    ControlSmootherLinear  _freq_lag;
    double                 _zi;
    float                  _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    std::array<double, 4>  _states{0, 0, 0, 0};
};

/* Coefficient generation from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt */
template <typename  FloatType = float>
BiquadCoefficients<FloatType> calc_lowpass(float freq, float q, float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    FloatType w0 = 2.0f * static_cast<FloatType>(M_PI) * freq / samplerate;
    FloatType w0_cos = std::cos(w0);
    FloatType w0_sin = std::sin(w0);
    FloatType alpha = w0_sin / q;
    FloatType norm = 1.0f / (1.0f + alpha);
    FloatType b0 = (1.0f - w0_cos) / 2.0f * norm;

    coeff.a1 = -2.0f * w0_cos * norm;
    coeff.a2 = (1 - alpha) * norm;
    coeff.b0 = b0;
    coeff.b1 = (1 - w0_cos) * norm;
    coeff.b2 = b0;
    return coeff;
};

template <typename  FloatType = float>
BiquadCoefficients<FloatType> calc_highpass(float freq, float q, float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    FloatType w0 = 2.0f * static_cast<FloatType>(M_PI) * freq / samplerate;
    FloatType w0_cos = std::cos(w0);
    FloatType w0_sin = std::sin(w0);
    FloatType alpha = w0_sin / q;
    FloatType norm = 1.0f / (1.0f + alpha);
    FloatType b0 = (1.0f + w0_cos) / 2.0f * norm;

    coeff.a1 = -2.0f * w0_cos * norm;
    coeff.a2 = (1 - alpha) * norm;
    coeff.b0 = b0;
    coeff.b1 = -(1 + w0_cos) * norm;
    coeff.b2 = b0;
    return coeff;
};

template <typename  FloatType = float>
inline BiquadCoefficients<FloatType> calc_bandpass(float freq, float q, float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / q;
    float norm = 1.0f / (1.0f + alpha);
    float b0 = alpha * norm;

    coeff.a1 = -2.0f * w0_cos * norm;
    coeff.a2 = (1 - alpha) * norm;
    coeff.b0 = b0;
    coeff.b1 = 0.0f;
    coeff.b2 = -b0;
    return coeff;
};

template <typename  FloatType = float>
inline BiquadCoefficients<FloatType> calc_allpass(float freq, float q, float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / q;
    float norm = 1.0f / (1.0f + alpha);
    float a1 =  -2.0f * w0_cos * norm;
    float b0 = (1.0f - alpha) * norm;

    coeff.a1 = a1;
    coeff.a2 = b0;
    coeff.b0 = b0;
    coeff.b1 = a1;
    coeff.b2 = 1.0f;
    return coeff;
};

/* freq in Hz, gain in dB */
template <typename  FloatType = float>
inline BiquadCoefficients<FloatType> calc_peaking(float freq, float gain, float q,  float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    float A = powf(10.0f, gain / 40.0f) ;
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / q;
    float norm = 1.0f / (1.0f + alpha / A);
    float b0 = (1.0f + alpha * A) * norm;

    coeff.a1 = -2.0f * w0_cos * norm;
    coeff.a2 = (1 - alpha / A) * norm;
    coeff.b0 = b0;
    coeff.b1 = -2.0f * w0_cos * norm;
    coeff.b2 = (1.0f - alpha * A) * norm;
    return coeff;
};

template <typename  FloatType = float>
inline BiquadCoefficients<FloatType> calc_lowshelf(float freq, float gain, float slope, float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    float A = powf(10.0f, gain / 40.0f);
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float A2_sqrt_alpha = 2.0f * w0_sin * std::sqrt((A * A + 1.0f) * (1.0f / slope - 1.0f) + 2.0f * A);
    float A_inc_cos_w0 = (A + 1.0f) * w0_cos;
    float A_dec_cos_w0 = (A - 1.0f) * w0_cos;
    float norm = 1.0f / ((A + 1.0f) + A_dec_cos_w0 + A2_sqrt_alpha);

    coeff.a1 = -2.0f * (A - 1.0f + A_inc_cos_w0) * norm;
    coeff.a2 = (A + 1.0f + A_dec_cos_w0 - A2_sqrt_alpha) * norm;
    coeff.b0 = A * (A + 1.0f - A_dec_cos_w0 + A2_sqrt_alpha) * norm;
    coeff.b1 = 2.0f * A * (A - 1.0f - A_inc_cos_w0) * norm;
    coeff.b2 = A * (A + 1.0f - A_dec_cos_w0 - A2_sqrt_alpha) * norm;
    return coeff;
};

template <typename  FloatType = float>
inline BiquadCoefficients<FloatType> calc_highshelf(float freq, float gain, float slope, float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    float A = powf(10.0f, gain / 40.0f);
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float A2_sqrt_alpha = 2.0f * w0_sin * std::sqrt((A * A + 1.0f) * (1.0f / slope - 1.0f) + 2.0f * A);
    float A_inc_cos_w0 = (A + 1.0f) * w0_cos;
    float A_dec_cos_w0 = (A - 1.0f) * w0_cos;
    float norm = 1.0f / ((A + 1.0f) - A_dec_cos_w0 + A2_sqrt_alpha);

    coeff.a1 = 2.0f * (A - 1.0f - A_inc_cos_w0) * norm;
    coeff.a2 = (A + 1.0f - A_dec_cos_w0 - A2_sqrt_alpha) * norm;
    coeff.b0 = A * (A + 1.0f + A_dec_cos_w0 + A2_sqrt_alpha) * norm;
    coeff.b1 = -2.0f * A * (A - 1.0f + A_inc_cos_w0) * norm;
    coeff.b2 = A * (A + 1.0f + A_dec_cos_w0 - A2_sqrt_alpha) * norm;
    return coeff;
};


}// namespace bricks

#endif //BRICKS_DSP_FILTER_BRICKS_H
