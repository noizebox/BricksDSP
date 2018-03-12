#ifndef BRICKS_DSP_OSCILLATORS_H
#define BRICKS_DSP_OSCILLATORS_H

#include <cassert>

#include "dsp_brick.h"

namespace bricks {

/* Naive saw/sq/tri oscillator with control rate pitch input */
class OscillatorBrick : public DspBrick
{
public:
    enum class Waveform
    {
        SAW,
        PULSE,
        TRIANGLE,
        SINE
    };

    enum AudioOutputs
    {
        OSC_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    OscillatorBrick(ControlPort pitch) : _pitch_port(pitch) {}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _buffer;
    }

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void render() override;

private:
    ControlPort         _pitch_port;
    float               _phase{0};
    Waveform            _waveform{Waveform::SAW};
    int                 _tri_dir{1};
    ControlSmootherLag  _pitch_lag;
    AudioBuffer         _buffer;
};

/* Naive saw/sq/tri oscillator with audio rate linear fm */
class FmOscillatorBrick : public DspBrick
{
public:
    enum class Waveform
    {
        SAW,
        PULSE,
        TRIANGLE,
        SINE
    };

    enum AudioOutputs
    {
        OSC_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    FmOscillatorBrick(ControlPort pitch, AudioPort lin_fm) : _pitch_port(pitch),
                                                             _lin_fm_port(lin_fm){}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _buffer;
    }

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void render() override;

private:
    ControlPort         _pitch_port;
    AudioPort           _lin_fm_port;
    float               _phase{0};
    Waveform            _waveform{Waveform::SAW};
    int                 _tri_dir{1};
    ControlSmootherLag  _pitch_lag;
    AudioBuffer         _buffer;
};


/* Wavetable based saw/sq/tri oscillator with control rate pitch input */
class WtOscillatorBrick : public DspBrick
{
public:
    enum class Waveform
    {
        SAW,
        PULSE,
        TRIANGLE,
        SINE
    };

    enum AudioOutputs
    {
        OSC_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    WtOscillatorBrick(ControlPort pitch) : _pitch_port(pitch) {}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _buffer;
    }

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void render() override;

private:
    ControlPort         _pitch_port;
    float               _phase{0};
    Waveform            _waveform{Waveform::SAW};
    AudioBuffer         _buffer;
};

}// namespace bricks

#endif //BRICKS_DSP_OSCILLATORS_H
