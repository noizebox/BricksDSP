#ifndef BRICKS_DSP_FILTER_BRICKS_H
#define BRICKS_DSP_FILTER_BRICKS_H

#include "dsp_brick.h"
namespace bricks {

/* Standard Biquad */
class BiquadFilterBrick : public DspBrick
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

    BiquadFilterBrick(ControlPort cutoff, ControlPort resonance, AudioPort audio_in) :
            _cutoff_ctrl(cutoff), _res_ctrl(resonance), _audio_in(audio_in) {}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_out;
    }

    void render() override;

protected:
    virtual void _calc_coefficients();

    ControlPort     _cutoff_ctrl;
    ControlPort     _res_ctrl;
    AudioPort       _audio_in;
    std::array<ControlSmootherLinear, 5> _coeff;
    std::array<float, 2> _in{0,0};
    std::array<float, 2> _out{0,0};

    AudioBuffer     _audio_out;
};

}// namespace bricks

#endif //BRICKS_DSP_FILTER_BRICKS_H
