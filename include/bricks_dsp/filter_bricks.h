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
    float out = in * coeff.b0 + reg.z1;
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

/* Standard Biquad with coefficent smoothing
 * This one should be deprecated, biquads should only really be used for fixed filters
 * as they generally don't take modulations well. */
class BiquadFilterBrick : public DspBrickImpl<3, 0, 1, 1>
{
public:
    enum class Mode
    {
        LOWPASS,
        BANDPASS,
        HIGHPASS,
        ALLPASS,
        PEAKING,
        LOW_SHELF,
        HIGH_SHELF
    };
    enum ControlInput
    {
        CUTOFF = 0,
        RESONANCE,
        GAIN
    };

    enum AudioOutput
    {
        FILTER_OUT = 0
    };

    BiquadFilterBrick(const float* cutoff, const float* resonance, const float* gain, const AudioBuffer* audio_in)
    {
        set_control_input(ControlInput::CUTOFF, cutoff);
        set_control_input(ControlInput::RESONANCE, resonance);
        set_control_input(ControlInput::GAIN, gain);
        set_audio_input(0, audio_in);
    }

    void render() override;

    void set_mode(Mode mode)
    {
        _mode = mode;
    }

    void reset() override
    {
        _reg = {0, 0};
        _coeff.fill(ControlSmootherLinear());
    }

private:
    Mode                                 _mode{Mode::LOWPASS};
    std::array<ControlSmootherLinear, 5> _coeff;
    Registers                            _reg{0,0};
};

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

    void render() override;

    void set_lowpass(float freq, float q = DEFAULT_Q, bool clear = true);
    void set_highpass(float freq, float q = DEFAULT_Q, bool clear = true);
    void set_bandpass(float freq, float q = DEFAULT_Q, bool clear = true);
    void set_peaking(float freq, float gain, float q = DEFAULT_Q, bool clear = true);
    void set_allpass(float freq, float q = DEFAULT_Q, bool clear = true);
    void set_lowshelf(float freq, float gain, float slope = DEFAULT_Q, bool clear = true);
    void set_highshelf(float freq, float gain, float slope = DEFAULT_Q, bool clear = true);

    void reset() override
    {
        _reg = {0, 0};
    }

private:
    Coefficients    _coeff{0,0,0,0,0};
    Registers       _reg{0,0};
};

/* Fixed Biquad with non-modulated filter parameters and templaated number of stages */
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
        if constexpr (stages == 1)
        {
            render_df2_biquad<FloatType, PROC_BLOCK_SIZE>(audio_in, audio_out, _coeff[0], regs[0]);
        }
        else if constexpr (stages % 2 == 0)
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
    static_assert(stages >= 2, "Need at least 2 stages to be useful");
    std::array<BiquadCoefficients<FloatType>, stages>   _coeff;
    std::array<BiquadRegisters<FloatType>, stages>      _reg;
    std::array<FloatType, stages>                       _pipeline;
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

    void reset() override
    {
        _g_lag.reset();
        _reg = {0, 0};
    }

    void render() override;

private:
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

    void reset() override
    {
        _freq_lag.reset();
        _states.fill(0);
    }

private:
    ControlSmootherLinear  _freq_lag;
    double                 _zi;
    std::array<double, 4>  _states{0, 0, 0, 0};
};

}// namespace bricks

#endif //BRICKS_DSP_FILTER_BRICKS_H
