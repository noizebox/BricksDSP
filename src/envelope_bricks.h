#ifndef BRICKS_DSP_ENVELOPE_BRICKS_H
#define BRICKS_DSP_ENVELOPE_BRICKS_H

#include "dsp_brick.h"
namespace bricks {

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

    enum AudioOutputs
    {
        ENV_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    ADSREnvelopeBrick(ControlPort attack, ControlPort decay,
                      ControlPort sustain, ControlPort release) : _controls{attack, decay, sustain, release} {}

    AudioPort audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return &_envelope;
    }

    /* Not part of the general interface. Analogous to the gate signal on an analog
     * envelope. Setting gate to true will start the envelope in the attack phase
     * and setting it to false will start the release phase. */
    void gate(bool gate);

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
    EnvelopeState _state;
    float _level;
};


}// namespace bricks

#endif //BRICKS_DSP_ENVELOPE_BRICKS_H
