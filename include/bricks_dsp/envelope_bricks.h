#ifndef BRICKS_DSP_ENVELOPE_BRICKS_H
#define BRICKS_DSP_ENVELOPE_BRICKS_H

#include "dsp_brick.h"
#include "random_device.h"

namespace bricks {

/* Linear slope ADSR envelope generated at audio rate */
class AudioRateADSRBrick : public DspBrickImpl <4, 0, 0, 1>
{
public:
    enum ControlInput
    {
        ATTACK = 0,
        DECAY,
        SUSTAIN,
        RELEASE
    };

    enum AudioOutput
    {
        ENV_OUT = 0
    };

    AudioRateADSRBrick() = default;

    AudioRateADSRBrick(const float* attack, const float* decay, const float* sustain, const float* release)
    {
        set_control_input(ControlInput::ATTACK, attack);
        set_control_input(ControlInput::DECAY, decay);
        set_control_input(ControlInput::SUSTAIN, sustain);
        set_control_input(ControlInput::RELEASE, release);
    }

    /* Not part of the general interface. Analogous to the gate signal on an analog
     * envelope. Setting gate to true will start the envelope in the attack phase
     * and setting it to false will start the release phase. */
    void gate(bool gate);

    bool finished() {return _state == EnvelopeState::OFF;}

    void set_samplerate(float samplerate) override
    {
        _samplerate = samplerate;
    }

    void reset() override
    {
        _state = EnvelopeState::OFF;
        _level = 0.0f;
    }

    void render() override;

private:
    enum class EnvelopeState
    {
        OFF,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE,
    };

    EnvelopeState _state{EnvelopeState::OFF};
    float         _level{0};
    float         _samplerate{DEFAULT_SAMPLERATE};
};

/* Control rate linear ADSR envelope with linear slopes */
class LinearADSREnvelopeBrick : public DspBrickImpl<4, 1, 0, 0>
{
public:
    enum ControlInput
    {
        ATTACK = 0,
        DECAY,
        SUSTAIN,
        RELEASE
    };

    enum ControlOutput
    {
        ENV_OUT = 0
    };

    LinearADSREnvelopeBrick() = default;

    LinearADSREnvelopeBrick(const float* attack, const float* decay, const float* sustain, const float* release)
    {
        set_control_input(ControlInput::ATTACK, attack);
        set_control_input(ControlInput::DECAY, decay);
        set_control_input(ControlInput::SUSTAIN, sustain);
        set_control_input(ControlInput::RELEASE, release);
    }

    /* Not part of the general interface. Analogous to the gate signal on an analog
     * envelope. Setting gate to true will start the envelope in the attack phase
     * and setting it to false will start the release phase. */
    void gate(bool gate);

    bool finished() {return _state == EnvelopeState::OFF;}

    void set_samplerate(float samplerate) override
    {
        _samplerate = samplerate;
    }

    void reset() override
    {
        _state = EnvelopeState::OFF;
        _level = 0.0f;
        _set_ctrl_value(ControlOutput::ENV_OUT, 0.0f);
    }

    void render() override;

private:
    enum class EnvelopeState
    {
        OFF,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE,
    };

    EnvelopeState _state{EnvelopeState::OFF};
    float         _level{0};
    float         _samplerate{DEFAULT_SAMPLERATE};
};

/* ADSR Envelope intended for audio control with linear attack phase and
 * exponentially falling decay and release phases */
class AudioADSREnvelopeBrick : public DspBrickImpl<4, 1, 0, 0>
{
public:
    enum ControlInput
    {
        ATTACK = 0,
        DECAY,
        SUSTAIN,
        RELEASE
    };

    enum ControlOutput
    {
        ENV_OUT = 0
    };

    AudioADSREnvelopeBrick() = default;

    AudioADSREnvelopeBrick(const float* attack, const float* decay, const float* sustain, const float* release)
    {
        set_control_input(ControlInput::ATTACK, attack);
        set_control_input(ControlInput::DECAY, decay);
        set_control_input(ControlInput::SUSTAIN, sustain);
        set_control_input(ControlInput::RELEASE, release);
    }

    /* Not part of the general interface. Analogous to the gate signal on an analog
     * envelope. Setting gate to true will start the envelope in the attack phase
     * and setting it to false will start the release phase. */
    void gate(bool gate);

    bool finished() {return _state == EnvelopeState::OFF;}

    void set_samplerate(float samplerate) override
    {
        _samplerate = samplerate;
    }

    void reset() override
    {
        _state = EnvelopeState::OFF;
        _level = 0.0f;
        _set_ctrl_value(ControlOutput::ENV_OUT, 0.0f);
    }

    void render() override;

private:
    enum class EnvelopeState
    {
        OFF,
        ATTACK,
        DECAY,
        RELEASE,
    };

    EnvelopeState _state{EnvelopeState::OFF};
    float         _level{0};
    float         _samplerate{DEFAULT_SAMPLERATE};
};


/* Control rate LFO  */
class LfoBrick : public DspBrickImpl<1, 1, 0, 0>
{
public:
    enum class Waveform
    {
        SAW,
        PULSE,
        TRIANGLE,
        SINE,
        SAMPLE_HOLD, // Sampled noise
        NOISE,       // White noise
        RANDOM,      // lp filtered noise
    };

    enum ControlInput
    {
        RATE = 0
    };

    enum ControlOutput
    {
        LFO_OUT = 0
    };

    LfoBrick() = default;

    LfoBrick(const float* rate)
    {
        set_control_input(ControlInput::RATE, rate);
    }

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override
    {
        _phase = 0;
        _level = 0;
        _set_ctrl_value(ControlOutput::LFO_OUT, 0);
    }

    void render() override;

private:
    float          _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float          _phase{0};
    float          _level{0};
    Waveform       _waveform{Waveform::TRIANGLE};
    int            _tri_dir{1};
    RandomDevice   _rand_device;
};

/* Optimised LFO with only sine as waveform */
class SineLfoBrick : public DspBrickImpl<1, 1, 0, 0>
{
public:
    enum ControlInput
    {
        RATE = 0
    };

    enum ControlOutput
    {
        LFO_OUT = 0
    };

    SineLfoBrick() = default;

    SineLfoBrick(const float* rate)
    {
        set_control_input(ControlInput::RATE, rate);
    }

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override {_phase = 0;}

    void render() override;

private:
    float _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float _phase{0};
};

/* Optimised low frequency random generation */
class RandLfoBrick : public DspBrickImpl<1, 1, 0, 0>
{
public:
    enum ControlInput
    {
        RATE = 0
    };

    enum ControlOutput
    {
        RAND_OUT = 0
    };

    RandLfoBrick() = default;

    RandLfoBrick(const float* rate)
    {
        set_control_input(ControlInput::RATE, rate);
    }

    void set_samplerate(float samplerate) override;

    void render() override
    {
        float out = _rand_device.get_norm() * GAIN_COMP;
        float level = (1.0f - _coeff_a0) * out + _coeff_a0 * _level;
        _set_ctrl_value(ControlOutput::RAND_OUT, level);
        _level = level;
    }

    static constexpr float LP_CUTOFF = 0.05f;
    static constexpr float GAIN_COMP = 100.0f;

private:
    float        _coeff_a0{0};
    float        _level{0};
    RandomDevice _rand_device;
};
}// namespace bricks

#endif //BRICKS_DSP_ENVELOPE_BRICKS_H
