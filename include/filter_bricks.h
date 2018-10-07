#ifndef BRICKS_DSP_FILTER_BRICKS_H
#define BRICKS_DSP_FILTER_BRICKS_H

#include "dsp_brick.h"
namespace bricks {

constexpr float DEFAULT_Q = 1 / sqrtf(2.0f);

/* Standard Biquad */
class BiquadFilterBrick : public DspBrick
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
    enum ControlInputs
    {
        CUTOFF = 0,
        RESONANCE,
        MAX_CONTROL_INPUTS,
    };

    enum AudioOutputs
    {
        FILTER_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    BiquadFilterBrick(ControlPort cutoff, ControlPort resonance, ControlPort gain, AudioPort audio_in) :
            _cutoff_ctrl(cutoff), _res_q_ctrl(resonance), _gain_ctrl(gain), _audio_in(audio_in) {}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_out;
    }

    void render() override;

    void set_mode(Mode mode) {_mode = mode;};

    void reset() {_reg = {0, 0};}

protected:
    ControlPort     _cutoff_ctrl;
    ControlPort     _res_q_ctrl;
    ControlPort     _gain_ctrl;
    AudioPort       _audio_in;
    Mode            _mode{Mode::LOWPASS};
    std::array<ControlSmootherLinear, 5> _coeff;
    std::array<float ,2> _reg{0,0};
    AudioBuffer     _audio_out;
};


/* State variable lowpass */
class SVFFilterBrick : public DspBrick
{
public:
    enum ControlInputs
    {
        CUTOFF = 0,
        RESONANCE,
        MAX_CONTROL_INPUTS,
    };

    enum AudioOutputs
    {
        FILTER_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    SVFFilterBrick(ControlPort cutoff, ControlPort resonance, AudioPort audio_in) :
            _cutoff_ctrl(cutoff), _res_ctrl(resonance), _audio_in(audio_in) {}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_out;
    }

    void render() override;

protected:
    ControlPort     _cutoff_ctrl;
    ControlPort     _res_ctrl;
    AudioPort       _audio_in;
    AudioBuffer     _audio_out;
    std::array<float ,2> _reg{0,0};
    ControlSmootherLinear _g_lag;
};

/* Standard Biquad with non-modulated filter parameters */
class FixedFilterBrick : public DspBrick
{
public:
    enum ControlInputs
    {
        MAX_CONTROL_INPUTS = 0,
    };

    enum AudioOutputs
    {
        FILTER_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    FixedFilterBrick(AudioPort audio_in) : _audio_in(audio_in) {}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_out;
    }

    void render() override;

    void set_lowpass(float freq, float q = DEFAULT_Q, bool clear = true);
    void set_highpass(float freq, float q = DEFAULT_Q, bool clear = true);
    void set_bandpass(float freq, float q = DEFAULT_Q, bool clear = true);
    void set_peaking(float freq, float gain, float q = DEFAULT_Q, bool clear = true);
    void set_allpass(float freq, float q = DEFAULT_Q, bool clear = true);
    void set_lowshelf(float freq, float gain, float slope = DEFAULT_Q, bool clear = true);
    void set_highshelf(float freq, float gain, float slope = DEFAULT_Q, bool clear = true);
    void reset() {_reg = {0, 0}; }

protected:
    AudioPort            _audio_in;
    std::array<float, 5> _coeff{0,0,0,0,0};
    std::array<float, 2> _reg{0,0};
    AudioBuffer          _audio_out;
};


}// namespace bricks

#endif //BRICKS_DSP_FILTER_BRICKS_H
