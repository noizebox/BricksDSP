#ifndef BRICKS_DSP_MODULATOR_BRICKS_H
#define BRICKS_DSP_MODULATOR_BRICKS_H

#include "dsp_brick.h"
namespace bricks {

/* Standard Biquad */
class SaturationBrick : public DspBrick
{
public:
    enum ControlInputs
    {
        CLIP_LEVEL = 0,
        MAX_CONTROL_INPUTS,
    };

    enum AudioOutputs
    {
        CLIP_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    SaturationBrick(ControlPort clip_level, AudioPort audio_in) : _clip_ctrl(clip_level),
                                                                  _audio_in(audio_in) {}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_out;
    }

    void render() override;

protected:
    ControlPort     _clip_ctrl;
    AudioPort       _audio_in;
    AudioBuffer     _audio_out;
};

} // end namespace bricks
#endif //BRICKS_DSP_MODULATOR_BRICKS_H
