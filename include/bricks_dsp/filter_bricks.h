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
struct GeneralBiquadCoeff
{
    FloatType a1;
    FloatType a2;
    FloatType b0;
    FloatType b1;
    FloatType b2;
};

template<typename FloatType>
struct GeneralBiquadReg
{
    FloatType z1;
    FloatType z2;
};

/* Direct form 2 transposed biquad */
template <typename FloatType, int BlockSize>
void do_df2_biquad(const AlignedArray<FloatType, BlockSize>& in,
                   AlignedArray<FloatType, BlockSize>& out,
                   const GeneralBiquadCoeff<FloatType>& coeff,
                   GeneralBiquadReg<FloatType>& registers)
{
    auto reg = registers;
    for (int i = 0; i < in.size(); ++i)
    {
        float out_val = in[i] * coeff.b0 + reg.z1;
        reg.z1 = in[i] * coeff.b1 + reg.z2 - coeff.a1 * out_val;
        reg.z2 = in[i] * coeff.b2 - coeff.a2 * out_val;
        out[i] = out_val;
    }
    registers = reg;
}

using Coefficients = GeneralBiquadCoeff<float>;
using BiquadRegisters = GeneralBiquadReg<float>;

/* Standard Biquad */
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
    BiquadRegisters                      _reg{0,0};
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
    BiquadRegisters _reg{0,0};
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
