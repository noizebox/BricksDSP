#ifndef BRICKS_DSP_MODULATOR_BRICKS_H
#define BRICKS_DSP_MODULATOR_BRICKS_H

#include <chrono>
#include <type_traits>
#include <memory>

#include "dsp_brick.h"

namespace bricks {

enum class ClipType
{
    SOFT,
    HARD
};

enum class InterpolationType
{
    NONE,
    LIN,
    CUBIC,
    CR_CUB
};

/* Simple and cheap sample-by sample clipper with a choice of tanh saturation or brickwall clipping */
template <ClipType type>
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

    SaturationBrick(ControlPort gain, AudioPort audio_in) : _gain(gain),
                                                            _audio_in(audio_in) {}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_out;
    }

    void render() override;

private:
    ControlPort     _gain;
    AudioPort       _audio_in;
    AudioBuffer     _audio_out;
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

private:
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

private:
    const AudioBuffer*  _audio_in;
    AudioBuffer         _unit_delay;
    AudioBuffer         _audio_out;
};


/* Provides basic functionality common to all delays */
class BasicDelay : public DspBrick
{
public:
    enum AudioOutputs
    {
        DELAY_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    BasicDelay(AudioPort audio_in, std::chrono::microseconds max_delay) : _audio_in(audio_in),
                                                                          _max_delay_time(max_delay)
    {
        set_max_delay_time(max_delay);
    }

    ~BasicDelay() override = default;

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_out;
    }

    void set_samplerate(float samplerate) override
    {
        DspBrick::set_samplerate(samplerate);
        set_max_delay_time(_max_delay_time);
    }

    void reset()
    {
        std::fill(_buffer, _buffer + _max_samples + PROC_BLOCK_SIZE, 0.0f);
    }

    void set_max_delay_time(std::chrono::duration<float> delay)
    {
        int samples = _samplerate * delay.count();
        /* Samples is rounded up to nearest multiple of PROC_BLOCK_SIZE + 1 more */
        _max_samples = (samples / PROC_BLOCK_SIZE + 2) * PROC_BLOCK_SIZE;
        _max_delay = samples;

        /* The actual allocation is PROC_BLOCK_SIZE larger to enable readout without
         * checking for wraparound every sample + a margin for interpolation */
        auto data_size = _max_samples + PROC_BLOCK_SIZE + 2 * INTERPOLATION_MARGIN;

        _buffer_data = std::make_unique<float[]>(data_size);
        std::fill(_buffer_data.get(), _buffer_data.get() + data_size, 0.0f);
        _buffer = _buffer_data.get() + INTERPOLATION_MARGIN;
    }

protected:
    void _copy_audio_in(const AudioBuffer& in)
    {
        std::copy(in.begin(), in.end(), &_buffer[_write_index]);
        _write_index += PROC_BLOCK_SIZE;
        if (_write_index > _max_samples)
        {
            assert(_write_index < _max_samples + PROC_BLOCK_SIZE + INTERPOLATION_MARGIN);
            /* The last block is repeated at the start again */
            std::copy(in.begin(), in.end(), _buffer);
            _write_index = PROC_BLOCK_SIZE;
        }
    }

    AudioPort                   _audio_in;
    AudioBuffer                 _audio_out;

    int                         _max_delay{0};
    int                         _max_samples{0};
    int                         _write_index{PROC_BLOCK_SIZE};
    float*                      _buffer{nullptr};

private:
    static constexpr int        INTERPOLATION_MARGIN = 2;
    std::unique_ptr<float[]>    _buffer_data{nullptr};
    std::chrono::microseconds   _max_delay_time;
};


/* Fixed delay line with selectable interpolation. For efficient memory usage,
 * do not use with a max delay much longer than the actual delay used.
 * Changing the delay line while running is safe but will cause artifacts */
template <InterpolationType type>
class FixedDelayBrick : public BasicDelay
{
public:
    FixedDelayBrick(AudioPort audio_in, std::chrono::microseconds max_delay) : BasicDelay(audio_in, max_delay)
    {
        set_delay_time(max_delay);
    }

    void set_delay_time(std::chrono::duration<float> delay)
    {
        set_delay_samples(_samplerate * delay.count());
    }

    /* Note that this can still be a fraction if interpolation is not NONE */
    void set_delay_samples(float samples)
    {
        _delay = clamp(samples, 0, _max_delay);
    }

    void render() override
    {
        _copy_audio_in(_audio_in.buffer());
        auto read_index = _get_read_index();
        for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
        {
            assert(read_index <= _max_samples + PROC_BLOCK_SIZE);
            assert(read_index >= 0);

            if constexpr (type == InterpolationType::NONE)
                _audio_out[i] = _buffer[read_index++];

            else if constexpr (type == InterpolationType::LIN)
                _audio_out[i] = linear_int(read_index++, _buffer);

            else if constexpr (type == InterpolationType::CUBIC)
                _audio_out[i] = cubic_int(read_index++, _buffer);

            else if constexpr (type == InterpolationType::CR_CUB)
                _audio_out[i] = catmull_rom_cubic_int(read_index++, _buffer);
        }
    }

private:
    using DelayIndex = std::conditional_t<type == InterpolationType::NONE, int, float>;

    DelayIndex _get_read_index()
    {
        auto pos = _write_index - _delay - PROC_BLOCK_SIZE;
        return pos >= 0? pos : _max_samples + pos;
    }

    DelayIndex  _delay{0};
};


/* Modulated delay line with selectable interpolation. Intended for small
 * and slow modulations. Think chorus/flanger/reverb as this modulates
 * the delay offset directly.
 * For heavy, tape-like modulation. Use the delay line below instead */
template <InterpolationType type>
class ModDelayBrick : public BasicDelay
{
public:
    ModDelayBrick(ControlPort delay_mod_ctrl, AudioPort audio_in,
                  std::chrono::microseconds max_delay) : BasicDelay(audio_in, max_delay),
                                                         _delay_mod_ctrl(delay_mod_ctrl)
    {
        set_delay_time(max_delay / 2);
    }

    void set_delay_time(std::chrono::duration<float> delay)
    {
        _delay = clamp(_samplerate * delay.count(), 0, _max_delay);
    }

    void render() override
    {
        _copy_audio_in(_audio_in.buffer());
        _mod_lag.set(clamp(_delay_mod_ctrl.value(), 0.0f, 1.0f));

        for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
        {
            float delay = clamp(_delay * (1.0f - _mod_lag.get()) - i, 0, _max_delay);
            auto read_index = _get_read_index(delay);
            assert(read_index < _max_samples + PROC_BLOCK_SIZE);

            if constexpr (type == InterpolationType::LIN)
                _audio_out[i] = linear_int(read_index, _buffer);

            else if constexpr (type == InterpolationType::CUBIC)
                _audio_out[i] = cubic_int(read_index, _buffer);

            else if constexpr (type == InterpolationType::CR_CUB)
                _audio_out[i] = catmull_rom_cubic_int(read_index, _buffer);
        }
    }

private:
    float _get_read_index(float delay)
    {
        auto pos = _write_index - delay- PROC_BLOCK_SIZE;
        return pos >= 0? pos : _max_samples + pos;
    }

    ControlPort           _delay_mod_ctrl;
    ControlSmootherLinear _mod_lag;
    float                 _delay{0};
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

    ~ModulatedDelayBrick() override
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

private:
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

/* Reduce the bit depth continuously from 24 to 1 */
class BitRateReducerBrick : public DspBrick
{
public:
    enum ControlInputs
    {
        BIT_DEPTH = 0,
        MAX_CONTROL_INPUTS,
    };

    enum AudioOutputs
    {
        OUT = 0,
        MAX_AUDIO_OUTS,
    };

    BitRateReducerBrick(ControlPort bit_depth, AudioPort audio_in) : _bit_depth(bit_depth),
                                                                     _audio_in(audio_in) {}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_out;
    }

    void render() override;

private:
    ControlPort     _bit_depth;
    AudioPort       _audio_in;
    AudioBuffer     _audio_out;
};

/* Reduce the sample rate continuously from 44100Hz to 20 Hz
 * Linear interpolation in both up and down sampling */
class SampleRateReducerBrick : public DspBrick
{
public:
    enum ControlInputs
    {
        SAMPLE_RATE = 0,
        MAX_CONTROL_INPUTS,
    };

    enum AudioOutputs
    {
        OUT = 0,
        MAX_AUDIO_OUTS,
    };

    SampleRateReducerBrick(ControlPort rate, AudioPort audio_in) : _rate(rate),
                                                                   _audio_in(audio_in)
    {
        _delay_buffer.fill(0.0f);
        _downsampled_buffer.fill(0.0f);
    }

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_out;
    }

    void render() override;

private:
    static constexpr int SAMPLE_DELAY = 2;

    ControlPort     _rate;
    AudioPort       _audio_in;
    AudioBuffer     _audio_out;

    float           _down_phase{0.0f};
    float           _up_phase{0.0f};

    AlignedArray<float, PROC_BLOCK_SIZE + SAMPLE_DELAY * 2> _delay_buffer;
    AlignedArray<float, PROC_BLOCK_SIZE + SAMPLE_DELAY * 2> _downsampled_buffer;
};

} // end namespace bricks
#endif //BRICKS_DSP_MODULATOR_BRICKS_H
