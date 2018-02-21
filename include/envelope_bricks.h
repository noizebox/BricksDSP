#ifndef BRICKS_DSP_ENVELOPE_BRICKS_H
#define BRICKS_DSP_ENVELOPE_BRICKS_H

#include "dsp_brick.h"
namespace bricks {

/* Linear slope ADSR envelope generated at audio rate */
class AudioRateADSRBrick : public DspBrick
{
public:
    enum ControlInputs
    {
        ATTACK = 0,
        DECAY,
        SUSTAIN,
        RELEASE,
        MAX_CONTROL_INPUTS,
    };

    enum AudioOutputs
    {
        ENV_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    AudioRateADSRBrick(ControlPort attack, ControlPort decay,
                       ControlPort sustain, ControlPort release) : _controls{attack, decay, sustain, release} {}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _envelope;
    }

    /* Not part of the general interface. Analogous to the gate signal on an analog
     * envelope. Setting gate to true will start the envelope in the attack phase
     * and setting it to false will start the release phase. */
    void gate(bool gate);

    bool finished() {return _state == EnvelopeState::OFF;}

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

    std::array<ControlPort, MAX_CONTROL_INPUTS> _controls;
    AudioBuffer _envelope;
    EnvelopeState _state{EnvelopeState::OFF};
    float _level{0};
};

/* Control rate linear ADSR envelope */
class ADSREnvelopeBrick : public DspBrick
{
public:
    enum ControlInputs
    {
        ATTACK = 0,
        DECAY,
        SUSTAIN,
        RELEASE,
        MAX_CONTROL_INPUTS,
    };

    enum ControlOuts
    {
        ENV_OUT = 0,
        MAX_CONTROL_OUTS,
    };

    ADSREnvelopeBrick(ControlPort attack, ControlPort decay,
                      ControlPort sustain, ControlPort release) : _controls{attack, decay, sustain, release} {}

    const float& control_output(int n) override
    {
        assert(n < MAX_CONTROL_OUTS);
        return _level;
    }

    /* Not part of the general interface. Analogous to the gate signal on an analog
     * envelope. Setting gate to true will start the envelope in the attack phase
     * and setting it to false will start the release phase. */
    void gate(bool gate);

    bool finished() {return _state == EnvelopeState::OFF;}

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

    std::array<ControlPort, MAX_CONTROL_INPUTS> _controls;
    EnvelopeState _state{EnvelopeState::OFF};
    float _level{0.0f};
};


/* Control rate LFO  */
class LfoBrick : public DspBrick
{
public:
    enum class Waveform
    {
        SAW,
        PULSE,
        TRIANGLE,
        SINE
    };

    enum ControlOutputs
    {
        LFO_OUT = 0,
        MAX_CONTROL_OUTS,
    };

    LfoBrick(ControlPort rate) : _rate_port(rate) {}

    const float& control_output(int n) override
    {
        assert(n < MAX_CONTROL_OUTS);
        return _level;
    }

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void render() override;

private:
    ControlPort    _rate_port;
    float          _phase{0};
    float          _level{0};
    Waveform       _waveform{Waveform::TRIANGLE};
    int            _tri_dir{1};
};

}// namespace bricks

#endif //BRICKS_DSP_ENVELOPE_BRICKS_H
