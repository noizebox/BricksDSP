#ifndef BRICKS_DSP_MODULATOR_BRICKS_H
#define BRICKS_DSP_MODULATOR_BRICKS_H

#include <chrono>
#include <type_traits>
#include <memory>

#include "dsp_brick.h"
#include "utils.h"

namespace bricks {

enum class ClipType
{
    SOFT,
    HARD
};

/* Simple and cheap sample-by sample clipper with a choice of tanh saturation or brickwall clipping */
template <ClipType type, size_t os_factor = 1>
class SaturationBrick : public DspBrickImpl<os_factor, 1, 0, 1, 1>
{
public:
    SaturationBrick() = default;

    void render(float gain, const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        for (int i = 0; i < PROC_BLOCK_SIZE * os_factor; ++i)
        {
            float x = audio_in[i] * gain;
            if constexpr (type == ClipType::SOFT)
            {
                x = clamp(x, -3.0f, 3.0f);
                audio_out[i] = tanh_approx(x);
            }
            if constexpr (type == ClipType::HARD)
            {
                audio_out[i] = clamp(x, -1.0f, 1.0f);
            }
        }
    }
};

/* Sigm or hard clipping with infinite linear oversampling according to :
 * https://www.researchgate.net/publication/308020367_Reducing_The_Aliasing_Of_Nonlinear_Waveshaping_Using_Continuous-Time_Convolution
 */
template <ClipType type = ClipType::HARD, size_t os_factor = 1>
class AASaturationBrick : public DspBrickImpl<os_factor, 1, 0, 1, 1>
{
public:
    AASaturationBrick() = default;

    void render(float gain, const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        float F1_1 = _prev_F1;
        float x_1 = _prev_x;
        constexpr float STATIONARY_TH = 0.0002f;

        for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
        {
            float x = audio_in[i] * gain;
            float F1;
            float y;

            if constexpr (type == ClipType::SOFT)
            {
                F1 = sigm_antiderivative(x);
            }
            else if constexpr (type == ClipType::HARD)
            {
                F1 = clip_antiderivate(x);
            }

            // For numerical stability if x is (almost) stationary
            if (std::abs(x - x_1) > STATIONARY_TH)
            {
                y = (F1 - F1_1) / (x - x_1);
            }
            else
            {
                if constexpr (type == ClipType::SOFT)
                {
                    y = sigm(x);
                }
                else if constexpr (type == ClipType::HARD)
                {
                    y = clamp(x, -1.0f, 1.0f);
                }
            }
            audio_out[i] = y;
            F1_1 = F1;
            x_1 = x;
        }
        _prev_F1 = F1_1;
        _prev_x = x_1;
    }

private:
    float           _prev_F1{0.0f};
    float           _prev_x{0};
};

/* Clone of Soft Distortion Sustainer circuit from Roland AG5 pedal */
template <size_t os_factor = 1>
class SustainerBrick : public DspBrickImpl<os_factor,4, 0, 2, 2>
{
public:
    void render(float gain, float compression, float stereo_mode, float time,
                const AudioOsBuffer<os_factor>& audio_in_l,
                const AudioOsBuffer<os_factor>& audio_in_r,
                AudioOsBuffer<os_factor>& audio_out_l,
                AudioOsBuffer<os_factor>& audio_out_r)
    {
        constexpr int LEFT = 0;
        constexpr int RIGHT = 1;
        enum Mode
        {
            STEREO = 0,
            LINKED,
            MONO
        };

        float gain_lin = 10 * to_db_approx(gain);
        float time_param = 1.0f - time + 0.02f;
        time_param = time_param * time_param * 90.0f;

        int mode  =  control_to_range(stereo_mode, 0, 2);
        assert(mode == Mode::STEREO || mode == Mode::LINKED || mode == Mode::MONO);

        auto op_gain = _op_gain;
        auto fet_gain = _fet_gain;
        auto op_hp = _op_hp;
        auto env_hp = _env_hp;
        auto env_lp = _env_lp;

        // scale down the gain with less compression for better controls
        gain_lin *= (1.0f - 0.6f * (1.0f - compression));

        for (int i = 0; i < PROC_BLOCK_SIZE * os_factor; ++i)
        {
            // gain control
            float x_l = audio_in_l[i] * gain_lin;
            float x_r = audio_in_r[i] * gain_lin;

            if (mode == Mode::MONO)
            {
                x_l = (x_l + x_r) * 0.5f;
            }

            // non-inv amp with soft clip
            float op_out_l = op_hp[LEFT].render_hp(x_l) * op_gain[LEFT];
            float op_out_r = op_hp[RIGHT].render_hp(x_r) * op_gain[RIGHT];
            float out_sample_l = sigm_hard(op_out_l * 0.5f + CLIP_ASYMMETRY * COMP_VAR_1) - CLIP_ASYMMETRY * COMP_VAR_1;
            float out_sample_r = sigm_hard(op_out_r * 0.5f + CLIP_ASYMMETRY) - CLIP_ASYMMETRY;
            float op_clip_l = out_sample_l * JFET_V_CUTOFF * compression;
            float op_clip_r = out_sample_r * JFET_V_CUTOFF * compression;

            audio_out_l[i] = out_sample_l * OUTPUT_GAIN;
            if (mode == Mode::MONO)
            {
                audio_out_r[i] = out_sample_l * OUTPUT_GAIN;
            }
            else
            {
                audio_out_r[i] = out_sample_r * OUTPUT_GAIN;
            }

            // calc feedback env follower,
            float env_in_l = env_hp[LEFT].render_hp(op_clip_l);
            float env_in_r = env_hp[RIGHT].render_hp(op_clip_r);

            // Simplest diode model (hard knee)
            // If diode is conducting, R5 and C3 forms a lp filter
            // If diode is not conducting, C3 is discharged through R3
            // In schematic only the negative side is considered (half-wave rectified)
            float rect_l;
            float rect_r;
            float cur_env_l = env_lp[LEFT].state();
            float cur_env_r = env_lp[RIGHT].state();

            if (env_in_l < -1.0f * (cur_env_l + DIODE_THRESHOLD))
            {
                rect_l = - (env_in_l + DIODE_THRESHOLD);
                env_lp[LEFT].set_approx(ENV_OPEN_RC * time_param, _samplerate, false);
            }
            else
            {
                rect_l = 0.0f;
                env_lp[LEFT].set_approx(ENV_CLOSED_RC * time_param, _samplerate, false);
            }

            if (env_in_r < -1.0f * (cur_env_r + DIODE_THRESHOLD * COMP_VAR_3))
            {
                rect_r = - (env_in_r + DIODE_THRESHOLD * COMP_VAR_3);
                env_lp[RIGHT].set_approx(ENV_OPEN_RC * time_param, _samplerate, false);
            }
            else
            {
                rect_r = 0.0f;
                env_lp[RIGHT].set_approx(ENV_CLOSED_RC * time_param, _samplerate, false);
            }

            fet_gain[LEFT] = env_lp[LEFT].render_lp(std::min(rect_l, JFET_V_CUTOFF + 1));
            float gain_ctrl_l = bricks::clamp(fet_gain[LEFT] * 2.5f, 0, JFET_V_CUTOFF);
            fet_gain[RIGHT] = env_lp[RIGHT].render_lp(std::min(rect_r, JFET_V_CUTOFF + 1));
            float gain_ctrl_r = bricks::clamp(fet_gain[RIGHT] * 2.5f, 0, JFET_V_CUTOFF);

            // calc new op-gain
            op_gain[LEFT] = 1.0f + 0.9f * COMP_VAR_1 * (JFET_V_CUTOFF - gain_ctrl_l) / JFET_V_CUTOFF * OPEN_LOOP_OP_GAIN;
            op_gain[RIGHT] = 1.0f + 0.9f * (JFET_V_CUTOFF - gain_ctrl_r) / JFET_V_CUTOFF * OPEN_LOOP_OP_GAIN;

            if (mode == Mode::LINKED)
            {
                float link_gain = std::min(op_gain[LEFT], op_gain[RIGHT]);

                op_gain[LEFT] = op_gain[LEFT] * (1.0f - STEREO_MIX_FACTOR) + link_gain * STEREO_MIX_FACTOR;
                op_gain[RIGHT] = op_gain[RIGHT] * (1.0f - STEREO_MIX_FACTOR) + link_gain * STEREO_MIX_FACTOR;
            }
        }

        _op_gain = op_gain;
        _fet_gain = fet_gain;
        _op_hp = op_hp;
        _env_hp = env_hp;
        _env_lp = env_lp;
    }

    void reset() override
    {
        _op_hp[0].reset();
        _op_hp[1].reset();
        _op_lp[0].reset();
        _op_lp[1].reset();
        _env_hp[0].reset();
        _env_hp[1].reset();
        _env_lp[0].reset();
        _env_lp[1].reset();
    }

    void set_samplerate(float samplerate) override
    {
        /* Values directly from the schematic */
        _op_hp[0].set(470.0 * 0.001, samplerate, true);
        _op_hp[1].set(470.0 * 0.001, samplerate, true);
        _op_lp[0].set(33 * 0.000000250, samplerate, true);
        _op_lp[1].set(33 * 0.000000250, samplerate, true);
        _env_hp[0].set((22.2 + 2.2) * 0.00068, samplerate, true);
        _env_hp[1].set((22.2 + 2.2) * 0.00068, samplerate, true);
        _env_lp[0].set(470.0 * 0.00022, samplerate, true);
        _env_lp[1].set(470.0 * 0.00022, samplerate, true);
        _samplerate = samplerate;
    }

private:
    static constexpr float DIODE_THRESHOLD = 0.66;
    //static constexpr float OP_FB_RES = 470;
    //static constexpr float JFET_OPEN_RES = 47000;
    static constexpr float JFET_V_CUTOFF = 5;
    static constexpr float CLIP_ASYMMETRY = 0.3;
    //static constexpr float SLEWRATE = 0.18;

    static constexpr float OPEN_LOOP_OP_GAIN = 300;
    static constexpr float OUTPUT_GAIN = 4;

    static constexpr float ENV_OPEN_RC = 2.2 * (0.00022 + 0.00068);
    static constexpr float ENV_CLOSED_RC = 470 * 0.00022;

    //static constexpr float OP_INTERNAL_LOWPASS = 33 * 0.000000250;

    static constexpr float STEREO_MIX_FACTOR = 0.85;

    /* Simulate component variation by having slightly different values on left and right channels */
    static constexpr float COMPONENT_VARIATION = 0.03;
    static constexpr float COMP_VAR_1 = 3 * COMPONENT_VARIATION + 1.0;
    //static constexpr float COMP_VAR_2 = 5 * COMPONENT_VARIATION + 1.0;
    static constexpr float COMP_VAR_3 = 7 * COMPONENT_VARIATION + 1.0;


    float            _samplerate{DEFAULT_SAMPLERATE};

    std::array<float, 2>  _op_gain{0,0};
    std::array<float, 2>  _fet_gain{0,0};

    std::array<RCStage<double>, 2>  _op_hp;
    std::array<RCStage<double>, 2>  _op_lp;
    std::array<RCStage<double>, 2>  _env_hp;
    std::array<RCStage<double>, 2>  _env_lp;
};

/* Delay audio one process block, used to break circular dependencies from feedback
 * loops. Input must must be set with set_input before calling render() */
template<size_t os_factor = 1>
class UnitDelayBrick : public DspBrickImpl<os_factor, 0, 0, 1, 1>
{
public:
    UnitDelayBrick() = default;

    void render(const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        audio_out = _unit_delay;
        _unit_delay = audio_in;
    }

private:
    AudioOsBuffer<os_factor> _unit_delay;
};


/* Provides basic functionality common to all delays */
template<size_t os_factor = 1>
class BasicDelay : public DspBrickImpl<os_factor, 1, 0, 1, 1>
{
public:
    BasicDelay(std::chrono::microseconds max_delay) : _max_delay_time(max_delay)
    {
        set_max_delay_time(max_delay);
    }

    ~BasicDelay() override = default;

    void set_samplerate(float samplerate) override
    {
        _samplerate = samplerate;
        set_max_delay_time(_max_delay_time);
    }

    void reset() override
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

    int                         _max_delay{0};
    int                         _max_samples{0};
    int                         _write_index{PROC_BLOCK_SIZE};
    float                       _samplerate{DEFAULT_SAMPLERATE};
    float*                      _buffer{nullptr};

private:
    static constexpr int        INTERPOLATION_MARGIN = 2;
    std::unique_ptr<float[]>    _buffer_data{nullptr};
    std::chrono::microseconds   _max_delay_time;
};


/* Fixed delay line with selectable interpolation. For efficient memory usage,
 * do not use with a max delay much longer than the actual delay used.
 * Changing the delay line while running is safe but will cause artifacts */
template <typename Interpolator, size_t os_factor = 1>
class FixedDelayBrick : public BasicDelay<os_factor>
{
public:
    enum AudioOutput
    {
        DELAY_OUT = 0
    };

    explicit FixedDelayBrick(std::chrono::microseconds max_delay = std::chrono::seconds(1)) : BasicDelay<os_factor>(max_delay)
    {
        set_delay_time(max_delay);
    }

    void set_delay_time(std::chrono::duration<float> delay)
    {
        set_delay_samples(this->_samplerate * delay.count());
    }

    /* Note that this can still be a fraction if interpolation is not NONE */
    void set_delay_samples(float samples)
    {
        _delay = clamp(samples, 0, this->_max_delay);
    }

    void render(float time, const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        this->_copy_audio_in(audio_in);
        auto read_index = _get_read_index();
        auto inter = _interpolator;

        for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
        {
            assert(read_index <= this->_max_samples + PROC_BLOCK_SIZE);
            assert(read_index >= 0);
            audio_out[i] = inter.interpolate(read_index++, this->_buffer);
        }
        _interpolator = inter;
    }

private:
    using DelayIndex = std::conditional_t<std::is_same_v<Interpolator, ZerothInterpolation<>>, int, float>;

    DelayIndex _get_read_index()
    {
        auto pos = this->_write_index - _delay - PROC_BLOCK_SIZE;
        return pos >= 0? pos : this->_max_samples + pos;
    }

    DelayIndex   _delay{0};
    Interpolator _interpolator;
};


/* Modulated delay line with selectable interpolation. Intended for small
 * and slow modulations. Think chorus/flanger/reverb as this modulates
 * the delay offset directly.
 * For heavy, tape-like modulation. Use the delay line below instead */
template <typename Interpolator, size_t os_factor = 1>
class ModDelayBrick : public BasicDelay<os_factor>
{
public:
    ModDelayBrick(std::chrono::microseconds max_delay = std::chrono::seconds(1)) : BasicDelay<os_factor>(max_delay)
    {
        set_delay_time(max_delay / 2);
    }

    void set_delay_time(std::chrono::duration<float> delay)
    {
        set_delay_samples(this->_samplerate * delay.count());
    }

    /* Note that this can still be a fraction if interpolation is not NONE */
    void set_delay_samples(float samples)
    {
        _delay = clamp(samples, 0, this->_max_delay);
    }

    void reset() override
    {
        _mod_lag.reset();
        BasicDelay<os_factor>::reset();
    }

    void render(float delay_mod, const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        this->_copy_audio_in(audio_in);
        auto mod_lag = _mod_lag;
        mod_lag.set(clamp(delay_mod, 0.0f, 1.0f));
        auto inter = _interpolator;

        for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
        {
            /* 0.5 is the mid-point. Delay is modulated around the set delay */
            float delay = clamp(_delay * (mod_lag.get() * 2.0f) - i, 0, this->_max_delay);
            auto read_index = _get_read_index(delay);
            assert(read_index < this->_max_samples + PROC_BLOCK_SIZE);

            audio_out[i] = inter.interpolate(read_index++, this->_buffer);
        }
        _mod_lag = mod_lag;
        _interpolator = inter;
    }

private:
    float _get_read_index(float delay)
    {
        auto pos = this->_write_index - delay - PROC_BLOCK_SIZE;
        return pos >= 0? pos : this->_max_samples + pos;
    }

    ControlSmootherLinear _mod_lag;
    float                 _delay{0};
    Interpolator          _interpolator;
};


/* Delayline with controllable delay time and tape-like behaviour when modulated */
template <size_t os_factor = 1>
class ModulatedDelayBrick : public DspBrickImpl<os_factor, 1, 0, 1, 1>
{
public:
    ModulatedDelayBrick(float max_delay_seconds = 1) : _max_seconds(max_delay_seconds)
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

    void set_samplerate(float samplerate) override
    {
        _samplerate = samplerate;
        set_max_delay_time(_max_seconds);
    }

    void set_max_delay_time(float max_delay_seconds)
    {
        _max_seconds = max_delay_seconds;
        if (_buffer && _rec_times)
        {
            delete[] _buffer;
            delete[] _rec_times;
        }
        /* Samples is rounded up to nearest multiple of PROC_BLOCK_SIZE plus 1 extra for interpolation*/
        size_t samples = max_delay_seconds * _samplerate;
        samples = (samples / PROC_BLOCK_SIZE + 2) * PROC_BLOCK_SIZE;
        _max_samples = samples;
        _buffer = new float[samples];
        _rec_times = new float[samples / PROC_BLOCK_SIZE];
        std::fill(_buffer, _buffer + _max_samples, 0.0f);
        std::fill(_rec_times, _rec_times + _max_samples/PROC_BLOCK_SIZE, 1.0f);

        _rec_head = 0;
        _rec_speed_index = 0;
        _rec_wraparound = _max_samples - PROC_BLOCK_SIZE;
        _play_wraparound = _rec_wraparound;
        _play_head = _rec_wraparound /2;
    }

    void reset() override
    {
        _delay_time_lag.reset();
        std::fill(_buffer, _buffer + _max_samples, 0.0f);
    }

    void render(float delay_time, const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        float current_time = clamp(delay_time, 0.01f, 1.0f);
        std::copy(audio_in.data(), audio_in.data() + PROC_BLOCK_SIZE, &_buffer[_rec_head]);
        // Get the delay time when audio was recorded in order to compensate
        float rec_time = _rec_times[static_cast<int>(_play_head / PROC_BLOCK_SIZE)];
        // then store the delay time of the current input
        _rec_times[_rec_speed_index++] = current_time;

        _rec_head += PROC_BLOCK_SIZE;
        if(_rec_head >= _rec_wraparound)
        {
            _rec_head = 0;
            _rec_speed_index = 0;
        }

        float readout_speed =  rec_time / current_time;
        _delay_time_lag.set(readout_speed);

        for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
        {
            _play_head += _delay_time_lag.get();
            while (_play_head > _play_wraparound)
            {
                _play_head -= _play_wraparound;
            }
            audio_out[i] = linear_int(_play_head, _buffer);
        }
    }

private:
    ControlSmootherLinear _delay_time_lag;
    float               _samplerate{DEFAULT_SAMPLERATE};
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

/* Schroeder Allpass delay for diffusion in reverbs and delays.
 * Currently only implemented with no interpolation - i.e. for
 * non-modulated delays.
 * Can probably be more efficiently interpolated using block based
 * processing for fixed delays */
template<int length, size_t os_factor = 1>
class AllpassDelayBrick : public DspBrickImpl<os_factor, 2, 0, 1, 1>
{
public:
    void reset() override
    {
        _buffer.fill(0.0f);
    }

    void render(float delay_time, float g_coeff, const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        float mod = clamp(delay_time, 0.0f, 1.0f);
        float gain = g_coeff;
        int mod_int = mod * length;
        int write_index = _write_index;

        for (int i = 0; i < PROC_BLOCK_SIZE * os_factor; ++i)
        {
            int index = (write_index + mod_int);
            while (index >= length)
            {
                index -= length;
            }
            float delay_out = _buffer[index];

            float delay_in = audio_in[i] + gain * delay_out;
            _buffer[write_index++] = delay_in;
            if (write_index >= length)
            {
                write_index = 0;
            }

            audio_out[i] = delay_out + -1.0f * gain * delay_in;
        }

        _write_index = write_index;
    }

private:
    int _write_index{0};
    std::array<float, length> _buffer;
};


/* Reduce the bit depth continuously from 24 to 1 */
template<size_t os_factor = 1>
class BitRateReducerBrick : public DspBrickImpl<os_factor, 1, 0, 1, 1>
{
public:
    BitRateReducerBrick() = default;

    void render(float bitrate, const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        float bit_gain = std::exp2f(1.0f + bitrate * MAX_BIT_DEPTH);
        float gain_red = 1.0f / (bit_gain - 1.0f);
        for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
        {
            audio_out[i] = static_cast<float>(static_cast<int>(audio_in[i] * bit_gain)) * gain_red;
        }
    }
};

/* Reduce the sample rate continuously from 44100Hz to 20 Hz
 * Linear interpolation in both up and down sampling */
template<size_t os_factor = 1>
class SampleRateReducerBrick : public DspBrickImpl<os_factor, 1, 0, 1, 1>
{
public:
    SampleRateReducerBrick() = default;

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override
    {
        _down_phase = 0;
        _up_phase = 0;
        _delay_buffer.fill(0);
        _downsampled_buffer.fill(0);
    }
    void render(float samplerate, const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        // Added to keep the upsampling phase from drifting away
        constexpr float NUDGE_FACTOR = 0.00005f;
        float ratio = clamp(control_to_freq(samplerate) * _samplerate_inv * 2.0f, 0.0f, 1.0f);

        // Delay for interpolation
        for (int i = 0; i < SAMPLE_DELAY; ++i)
        {
            _delay_buffer[i] = _delay_buffer[PROC_BLOCK_SIZE + i];
        }
        std::copy(audio_in.begin(), audio_in.end(), &_delay_buffer[SAMPLE_DELAY]);

        // Downsample to internal buffer
        float down_phase = _down_phase;
        int down_samples = 0;
        while (down_phase <= PROC_BLOCK_SIZE)
        {
            _downsampled_buffer[SAMPLE_DELAY + down_samples] = linear_int(down_phase, _delay_buffer.data());
            down_phase += 1.0f / ratio;
            down_samples++;
        }
        if (down_phase > PROC_BLOCK_SIZE)
        {
            down_phase -= PROC_BLOCK_SIZE;
        }
        _down_phase = down_phase;
        assert(down_phase >= 0.0f);

        // Upsample and interpolate from this buffer

        float up_phase = std::min(_up_phase, 1.0f);
        for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
        {
            audio_out[i] = linear_int(up_phase, _downsampled_buffer.data());
            up_phase += ratio;
        }
        _up_phase = std::max(up_phase - up_phase * NUDGE_FACTOR - down_samples, 0.0f);

        assert(_up_phase >= 0.0f);
        assert(_up_phase < PROC_BLOCK_SIZE + 2);

        // 'rewind' the downsampled buffer
        for (int i = 0; i < SAMPLE_DELAY; ++i)
        {
            _downsampled_buffer[i] = _downsampled_buffer[down_samples + i];
        }
    }

private:
    static constexpr int SAMPLE_DELAY = 2;

    float           _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float           _down_phase{0.0f};
    float           _up_phase{0.0f};

    AlignedArray<float, PROC_BLOCK_SIZE + SAMPLE_DELAY * 2> _delay_buffer;
    AlignedArray<float, PROC_BLOCK_SIZE + SAMPLE_DELAY * 2> _downsampled_buffer;
};

} // end namespace bricks
#endif //BRICKS_DSP_MODULATOR_BRICKS_H
