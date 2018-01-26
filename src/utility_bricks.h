#ifndef BRICKS_DSP_UTILITY_BRICKS_H
#define BRICKS_DSP_UTILITY_BRICKS_H

#include "dsp_brick.h"
namespace bricks {

/* Generic gain control of audio signal */
class VcaBrick : public DspBrick
{
public:
    enum AudioOutputs
    {
        VCA_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    VcaBrick(ControlPort gain, AudioPort audio_in) : _gain_port(gain),
                                                     _audio_in(audio_in) {}

    AudioPort audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return &_audio_buffer;
    }

    void render() override
    {
        float gain = *_gain_port;
        for (unsigned s = 0; s < _audio_buffer.size(); ++s)
        {
            _audio_buffer[s] = (*_audio_in)[s] * gain;
        }
    }

private:
    ControlPort _gain_port;
    AudioPort   _audio_in;
    AudioBuffer _audio_buffer;
};

/* General n to 1 audio mixer with individual gain controls for each input
 * Instantiation example:
 * AudioMixerBrick<2> mixer({gain1_, gain_2}, {audio_in_1, audio_in_2}); */

template <size_t channel_count>
class AudioMixerBrick : public DspBrick
{
public:
    enum AudioOutputs
    {
        MIX_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    AudioMixerBrick(std::array<ControlPort, channel_count> gains,
                    std::array<AudioPort, channel_count> audio_ins) : _gains(gains),
                                                                      _audio_ins(audio_ins) {}

    AudioPort audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return &_audio_buffer;
    }

    void render() override
    {
        for (auto& s : _audio_buffer)
        {
            s = 0.0f;
        }
        for (unsigned i = 0; i < channel_count; ++i)
        {
            float gain = *_gains[i];
            for (unsigned s = 0; s < _audio_buffer.size(); ++s)
            {
                _audio_buffer[s] += (*_audio_ins[i])[s] * gain;
            }
        }
    }

private:
    std::array<ControlPort, channel_count> _gains;
    std::array<AudioPort, channel_count> _audio_ins;
    AudioBuffer _audio_buffer;
};


}// namespace bricks

#endif //BRICKS_DSP_UTILITY_BRICKS_H
