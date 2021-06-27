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
class SaturationBrick : public DspBrickImpl<1, 0, 1, 1>
{
public:
    enum ControlInput
    {
        GAIN = 0
    };

    enum AudioOutput
    {
        CLIP_OUT = 0
    };

    SaturationBrick() = default;

    SaturationBrick(const float* gain, const AudioBuffer* audio_in)
    {
        set_control_input(ControlInput::GAIN, gain);
        set_audio_input(0, audio_in);
    }

    void render() override;
};

/* Sigm or hard clipping with infinite linear oversampling according to :
 * https://www.researchgate.net/publication/308020367_Reducing_The_Aliasing_Of_Nonlinear_Waveshaping_Using_Continuous-Time_Convolution
 */
template <ClipType type = ClipType::HARD>
class AASaturationBrick : public DspBrickImpl<1, 0, 1, 1>
{
public:
    enum ControlInput
    {
        GAIN = 0
    };

    enum AudioOutput
    {
        CLIP_OUT = 0
    };

    AASaturationBrick() = default;

    AASaturationBrick(const float* gain, const AudioBuffer* audio_in)
    {
        set_control_input(ControlInput::GAIN, gain);
        set_audio_input(0, audio_in);
    }

    void render() override;

private:
    float           _prev_F1{0.0f};
    float           _prev_x{0};
};

/* Clone of Soft Distortion Sustainer circuit from Roland AG5 pedal */
class SustainerBrick : public DspBrickImpl<5, 0, 1, 1>
{
public:
    enum ControlInput
    {
        GAIN = 0,
        ASYMMETRY,
        THIRD,
        FOURTH,
        FIFTH
    };

    enum AudioOutput
    {
        SUSTAIN_OUT = 0
    };

    SustainerBrick() = default;

    SustainerBrick(const float* gain, const AudioBuffer* audio_in)
    {
        set_control_input(ControlInput::GAIN, gain);
        set_audio_input(DEFAULT_INPUT, audio_in);
    }

    void render() override;

    void reset() override;

    void set_samplerate(float samplerate) override;

private:
    float           _op_gain{0.0f};
    float           _fet_gain{0.0f};
    bool            _plot;

    RCStage<double>  _op_hp;
    RCStage<double>  _op_lp;
    RCStage<double>  _env_hp;
    RCStage<double>  _env_lp;
};

/* Delay audio one process block, used to break circular dependencies from feedback
 * loops. Input must must be set with set_input before calling render() */
class UnitDelayBrick : public DspBrickImpl<0, 0, 1, 1>
{
public:
    enum AudioOutput
    {
        DELAY_OUT = 0
    };

    UnitDelayBrick() = default;

    UnitDelayBrick(const AudioBuffer* audio_in)
    {
        set_audio_input(0, audio_in);
    }

    void set_input(const AudioBuffer* audio_in)
    {
        set_audio_input(0, audio_in);
    }

    void render() override;

private:
    AudioBuffer         _unit_delay;
};


/* Provides basic functionality common to all delays */
class BasicDelay : public DspBrickImpl<1, 0, 1, 1>
{
public:
    enum AudioOutput
    {
        DELAY_OUT = 0
    };

    BasicDelay(std::chrono::microseconds max_delay) : _max_delay_time(max_delay)
    {
        set_max_delay_time(max_delay);
    }

    ~BasicDelay() override = default;

    void set_samplerate(float samplerate) override
    {
        DspBrickImpl::set_samplerate(samplerate);
        set_max_delay_time(_max_delay_time);
    }

    virtual void reset() override
    {
        std::fill(_buffer, _buffer + _max_samples + PROC_BLOCK_SIZE, 0.0f);
    }

    void set_max_delay_time(std::chrono::duration<float> delay)
    {
        int samples = samplerate() * delay.count();
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
    enum AudioOutput
    {
        DELAY_OUT = 0
    };

    explicit FixedDelayBrick(std::chrono::microseconds max_delay = std::chrono::seconds(1)) : BasicDelay(max_delay)
    {
        set_delay_time(max_delay);
    }

    explicit FixedDelayBrick(const AudioBuffer* audio_in, std::chrono::microseconds max_delay = std::chrono::seconds(1))
                             : BasicDelay(max_delay)
    {
        set_audio_input(DEFAULT_INPUT, audio_in);
        set_delay_time(max_delay);
    }

    void set_delay_time(std::chrono::duration<float> delay)
    {
        set_delay_samples(samplerate() * delay.count());
    }

    /* Note that this can still be a fraction if interpolation is not NONE */
    void set_delay_samples(float samples)
    {
        _delay = clamp(samples, 0, _max_delay);
    }

    void render() override
    {
        _copy_audio_in(_input_buffer(DEFAULT_INPUT));
        auto read_index = _get_read_index();
        auto& audio_out = _output_buffer(AudioOutput::DELAY_OUT);
        for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
        {
            assert(read_index <= _max_samples + PROC_BLOCK_SIZE);
            assert(read_index >= 0);

            if constexpr (type == InterpolationType::NONE)
                audio_out[i] = _buffer[read_index++];

            else if constexpr (type == InterpolationType::LIN)
                audio_out[i] = linear_int(read_index++, _buffer);

            else if constexpr (type == InterpolationType::CUBIC)
                audio_out[i] = cubic_int(read_index++, _buffer);

            else if constexpr (type == InterpolationType::CR_CUB)
                audio_out[i] = catmull_rom_cubic_int(read_index++, _buffer);
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
    enum ControlInput
    {
        DELAY_MOD
    };

    enum AudioOutput
    {
        DELAY_OUT = 0
    };

    ModDelayBrick(std::chrono::microseconds max_delay = std::chrono::seconds(1)) : BasicDelay(max_delay)
    {
        set_delay_time(max_delay / 2);
    }

    ModDelayBrick(const float* delay_mod, const AudioBuffer* audio_in,
                  std::chrono::microseconds max_delay = std::chrono::seconds(1)) : BasicDelay(max_delay)
    {
        set_control_input(ControlInput::DELAY_MOD, delay_mod);
        set_audio_input(DEFAULT_INPUT, audio_in);
        set_delay_time(max_delay / 2);
    }

    void set_delay_time(std::chrono::duration<float> delay)
    {
        _delay = clamp(samplerate() * delay.count(), 0, _max_delay);
    }

    void reset() override
    {
        _mod_lag.reset();
        BasicDelay::reset();
    }

    void render() override
    {
        _copy_audio_in(_input_buffer(DEFAULT_INPUT));
        auto mod_lag = _mod_lag;
        mod_lag.set(clamp(_ctrl_value(ControlInput::DELAY_MOD), 0.0f, 1.0f));
        auto& audio_out = _output_buffer(AudioOutput::DELAY_OUT);

        for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
        {
            float delay = clamp(_delay * (1.0f - _mod_lag.get()) - i, 0, _max_delay);
            auto read_index = _get_read_index(delay);
            assert(read_index < _max_samples + PROC_BLOCK_SIZE);

            if constexpr (type == InterpolationType::LIN)
                audio_out[i] = linear_int(read_index, _buffer);

            else if constexpr (type == InterpolationType::CUBIC)
                audio_out[i] = cubic_int(read_index, _buffer);

            else if constexpr (type == InterpolationType::CR_CUB)
                audio_out[i] = catmull_rom_cubic_int(read_index, _buffer);
        }
        _mod_lag = mod_lag;
    }

private:
    float _get_read_index(float delay)
    {
        auto pos = _write_index - delay- PROC_BLOCK_SIZE;
        return pos >= 0? pos : _max_samples + pos;
    }

    ControlSmootherLinear _mod_lag;
    float                 _delay{0};
};


/* Delayline with controllable delay time and tape-like behaviour when modulated */
class ModulatedDelayBrick : public DspBrickImpl<1, 0, 1, 1>
{
public:
    enum ControlInput
    {
        DELAY_TIME
    };

    enum AudioOutput
    {
        DELAY_OUT = 0
    };

    ModulatedDelayBrick(float max_delay_seconds = 1) : _max_seconds(max_delay_seconds)
    {
        set_max_delay_time(_max_seconds);
        _delay_time_lag.set(1.0f),
        _delay_time_lag.get_all();
    }

    ModulatedDelayBrick(const float* delay_ctrl, const AudioBuffer* audio_in, float max_delay_seconds= 1) : _max_seconds(max_delay_seconds)
    {
        set_control_input(ControlInput::DELAY_TIME, delay_ctrl);
        set_audio_input(DEFAULT_INPUT, audio_in);
        set_max_delay_time(_max_seconds);
        _delay_time_lag.set(1.0f),
        _delay_time_lag.get_all();
    }

    ~ModulatedDelayBrick() override
    {
        delete[] _buffer;
        delete[] _rec_times;
    }

    void set_samplerate(float samplerate) override
    {
        DspBrickImpl::set_samplerate(samplerate);
        set_max_delay_time(_max_seconds);
    }

    void set_max_delay_time(float max_delay_seconds);

    void reset() override;

    void render() override;

private:
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
class BitRateReducerBrick : public DspBrickImpl<1, 0, 1, 1>
{
public:
    enum ControlInput
    {
        BIT_DEPTH = 0
    };

    enum AudioOutput
    {
        BITRED_OUT = 0
    };

    BitRateReducerBrick() = default;

    BitRateReducerBrick(const float* bit_depth, const AudioBuffer* audio_in)
    {
        set_control_input(ControlInput::BIT_DEPTH, bit_depth);
        set_audio_input(0, audio_in);
    }

    void render() override;
};

/* Reduce the sample rate continuously from 44100Hz to 20 Hz
 * Linear interpolation in both up and down sampling */
class SampleRateReducerBrick : public DspBrickImpl<1, 0, 1, 1>
{
public:
    enum ControlInput
    {
        SAMPLE_RATE = 0
    };

    enum AudioOutput
    {
        DOWNSAMPLED_OUT = 0
    };

    SampleRateReducerBrick() = default;

    SampleRateReducerBrick(const float* rate, const AudioBuffer* audio_in)
    {
        set_control_input(ControlInput::SAMPLE_RATE, rate);
        set_audio_input(0, audio_in);
    }

    void reset() override;

    void render() override;

private:
    static constexpr int SAMPLE_DELAY = 2;

    float           _down_phase{0.0f};
    float           _up_phase{0.0f};

    AlignedArray<float, PROC_BLOCK_SIZE + SAMPLE_DELAY * 2> _delay_buffer;
    AlignedArray<float, PROC_BLOCK_SIZE + SAMPLE_DELAY * 2> _downsampled_buffer;
};

} // end namespace bricks
#endif //BRICKS_DSP_MODULATOR_BRICKS_H
