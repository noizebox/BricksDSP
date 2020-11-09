#ifndef BRICKS_DSP_FILTER_BRICKS_H
#define BRICKS_DSP_FILTER_BRICKS_H

#include "dsp_brick.h"
namespace bricks {

constexpr float DEFAULT_Q = 1 / 1.42f; //sqrtf(2.0f);

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
        _reg.fill(0.0f);
    }

private:
    Mode                                 _mode{Mode::LOWPASS};
    float                                _samplerate{DEFAULT_SAMPLERATE};
    std::array<ControlSmootherLinear, 5> _coeff;
    std::array<float ,2>                 _reg{0,0};
};

/* State variable filter with multiple outs from Andrew Simper, Cytomic,
 * adapted from https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf */
class SVFFilterBrick : public DspBrickImpl<2, 0, 1, 3>
{
public:
    enum ControlInputs
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

    SVFFilterBrick(const float* cutoff, float* resonance, const AudioBuffer* audio_in)
    {
        set_control_input(ControlInputs::CUTOFF, cutoff);
        set_control_input(ControlInputs::RESONANCE, resonance);
        set_audio_input(0, audio_in);
    }

    void reset()
    {
        // TODO - reset() on control smoothers.
        _reg.fill(0);
    }

    void render() override;

private:
    std::array<float ,2> _reg{0,0};
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

    void reset()
    {
        _reg = {0, 0};
    }

private:
    std::array<float, 5> _coeff{0,0,0,0,0};
    std::array<float, 2> _reg{0,0};
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

    MystransLadderFilter(float* cutoff, float* resonance, const AudioBuffer* audio_in)
    {
        set_control_input(ControlInput::CUTOFF, cutoff);
        set_control_input(ControlInput::RESONANCE, resonance);
        set_audio_input(0, audio_in);
    }

    void render() override;

    void reset()
    {
        // TODO - reset() on control smoothers.
        _states.fill(0);
    }

private:
    ControlSmootherLinear  _freq_lag;
    double                 _zi;
    std::array<double, 4>  _states{0, 0, 0, 0};
};

}// namespace bricks

#endif //BRICKS_DSP_FILTER_BRICKS_H
