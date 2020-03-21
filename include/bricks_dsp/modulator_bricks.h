#ifndef BRICKS_DSP_MODULATOR_BRICKS_H
#define BRICKS_DSP_MODULATOR_BRICKS_H

#include "dsp_brick.h"
namespace bricks {

/* Tanh clipping */
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

enum class ClipType
{
    SOFT,
    HARD
};

/* Sigm or hard clipping with infinite linear oversampling according to :
 * https://www.researchgate.net/publication/308020367_Reducing_The_Aliasing_Of_Nonlinear_Waveshaping_Using_Continuous-Time_Convolution
 */
template <ClipType type = ClipType::HARD>
class AASaturationBrick : public DspBrick
{
public:
    enum ControlInputs
    {
        GAIN = 0,
        MAX_CONTROL_INPUTS,
    };

    enum AudioOutputs
    {
        CLIP_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    AASaturationBrick(ControlPort gain, AudioPort audio_in) : _gain(gain),
                                                              _audio_in(audio_in)
    {
        _audio_out.fill(0.0f);
    }

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_out;
    }

    void render() override;

protected:
    float           _prev_F1{0.0f};
    float           _prev_x{0};
    ControlPort     _gain;
    AudioPort       _audio_in;
    AudioBuffer     _audio_out;
};

/* Delay audio one process block, used to break circular dependencies from feedback
 * loops. Input must must be set with set_input before calling render() */
class UnitDelayBrick : public DspBrick
{
public:
    enum AudioOutputs
    {
        MAIN_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    UnitDelayBrick()
    {
        _unit_delay.fill(0.0f);
    }

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_out;
    }

    void set_input(const AudioBuffer& audio_in)
    {
        _audio_in = &audio_in;
    }

    void render() override;

protected:
    const AudioBuffer*  _audio_in;
    AudioBuffer         _unit_delay;
    AudioBuffer         _audio_out;
};

/* Delayline with controllable delay time and tape-like behaviour when modulated */
class ModulatedDelayBrick : public DspBrick
{
public:
    enum AudioOutputs
    {
        DELAY_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    ModulatedDelayBrick(ControlPort delay_ctrl, AudioPort audio_in, float max_delay_seconds) : _delay_ctrl(delay_ctrl),
                                                                                               _audio_in(audio_in),
                                                                                               _max_seconds(max_delay_seconds)

    {
        set_max_delay_time(_max_seconds);
        _delay_time_lag.set(1.0f),
        _delay_time_lag.get_all();
    }

    ~ModulatedDelayBrick()
    {
        delete[] _buffer;
        delete[] _rec_times;
    }

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_out;
    }

    void set_samplerate(float samplerate) override
    {
        DspBrick::set_samplerate(samplerate);
        set_max_delay_time(_max_seconds);
    }

    void set_max_delay_time(float max_delay_seconds);

    void render() override;

protected:
    ControlPort         _delay_ctrl;
    AudioPort           _audio_in;
    AudioBuffer         _audio_out;
    ControlSmootherLinear _delay_time_lag;
    float               _max_seconds;
    int                 _max_samples;
    int                 _rec_head;
    int                 _rec_speed_index;
    int                 _rec_wraparound;
    float               _play_head{PROC_BLOCK_SIZE};
    float               _play_wraparound;
    float*              _buffer{nullptr};
    float*              _rec_times{nullptr};

};

} // end namespace bricks
#endif //BRICKS_DSP_MODULATOR_BRICKS_H
