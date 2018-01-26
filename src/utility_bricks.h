#ifndef BRICKS_DSP_UTILITY_BRICKS_H
#define BRICKS_DSP_UTILITY_BRICKS_H

#include "dsp_brick.h"
namespace bricks {

class VcaBrick : public DspBrick
{
public:
    enum ControlInputs
    {
        GAIN = 0,
        MAX_CONTROL_INPUTS,
    };

    VcaBrick(const ControlPort gain, const AudioPort audio_in) : _gain_port(gain),
                                                     _audio_in(audio_in) {}
    ControlPort control_output(int /*n*/) override
    {
        assert(false);
    }
    AudioPort audio_output(int n) override
    {
        assert(n < MAX_CONTROL_INPUTS);
        return &_audio_buffer;
    }

    void render() override
    {
        float gain = *_gain_port;
        for (unsigned s = 0; s < _audio_buffer.size(); ++s)
        {
            _audio_buffer[s] = (*_audio_in)[s] * gain;
        }
        (*_audio_in)[0] = 1.0f;
    }

private:
    ControlPort _gain_port;
    AudioPort   _audio_in;
    AudioBuffer _audio_buffer;
};





}// namespace bricks

#endif //BRICKS_DSP_UTILITY_BRICKS_H
