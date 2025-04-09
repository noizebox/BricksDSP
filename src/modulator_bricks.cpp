#include <algorithm>
#include <cmath>

#include "modulator_bricks.h"

namespace bricks {

constexpr float MAX_BIT_DEPTH = 24;

/* Pade approximation of tanh, valid within [-3, 3] */
inline float tanh_approx(const float& x)
{
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

inline double tanh_antiderivative(const double& x)
{
    return std::log(std::cosh(x));
    //return 14.0f / 9.0f * std::log(std::abs(9 * x * x + 27));
}

/* One of the computationally cheapest soft clipping functions */
inline float sigm(const float& x)
{
    return x / std::sqrt(1 + x * x);
}

/* A harder clipped version of the above */
inline float sigm_hard(float x)
{
    constexpr float HARDNESS = 1.0;

    float x_t = x + HARDNESS * x * x * x;
    return x_t / std::sqrt(1 + x_t * x_t);
}

/* Integral of the above sigmoid clipping function */
inline float sigm_antiderivative(const float& x)
{
    // Todo -  verify if this is the correct integral of sigm()
    return std::sqrt(1 + x * x);
}

/* Primitive function of a rectangular clip function
 * that is 1 for x > 1, -1 for x < -1 and x otherwise */
inline float clip_antiderivate(const float& x)
{
    if (x >= 1)
    {
        return x - 0.5f;
    }
    if (x <= -1)
    {
        return -x - 0.5f;
    }
    return  0.5f * x * x;
}

template<>
void SaturationBrick<ClipType::SOFT>::render()
{
    const AudioBuffer& in = _input_buffer(0);
    AudioBuffer& audio_out = _output_buffer(AudioOutput::CLIP_OUT);
    float gain = _ctrl_value(ControlInput::GAIN);
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        float x = in[i] * gain;
        x = clamp(x, -3.0f, 3.0f);
        audio_out[i] = tanh_approx(x);
    }
}

template<>
void SaturationBrick<ClipType::HARD>::render()
{
    const AudioBuffer& in = _input_buffer(0);
    AudioBuffer& audio_out = _output_buffer(AudioOutput::CLIP_OUT);
    float gain = _ctrl_value(ControlInput::GAIN);
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        float x = in[i] * gain;
        audio_out[i] = clamp(x, -1.0f, 1.0f);
    }
}

template <ClipType type>
inline void render_aa_clipping(const AudioBuffer& in, AudioBuffer& out, float gain, float& prev_F1, float& prev_x)
{
    float F1_1 = prev_F1;
    float x_1 = prev_x;
    constexpr float STATIONARY_TH = 0.0002f;

    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        float x = in[i] * gain;
        float F1;
        float y;

        if constexpr (type == ClipType::SOFT)
        {
            F1 = sigm_antiderivative(x);
        }
        else if (type == ClipType::HARD)
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
            else if (type == ClipType::HARD)
            {
                y = clamp(x, -1.0f, 1.0f);
            }
        }
        out[i] = y;
        F1_1 = F1;
        x_1 = x;
    }
    prev_F1 = F1_1;
    prev_x = x_1;
}

template <>
void AASaturationBrick<ClipType::SOFT>::render()
{
    const AudioBuffer& audio_in = _input_buffer(0);
    AudioBuffer& audio_out = _output_buffer(AudioOutput::CLIP_OUT);
    float gain = _ctrl_value(ControlInput::GAIN);
    render_aa_clipping<ClipType::SOFT>(audio_in, audio_out, gain, _prev_F1, _prev_x);
}

template <>
void AASaturationBrick<ClipType::HARD>::render()
{
    const AudioBuffer& audio_in = _input_buffer(0);
    AudioBuffer& audio_out = _output_buffer(AudioOutput::CLIP_OUT);
    float gain = _ctrl_value(ControlInput::GAIN);
    render_aa_clipping<ClipType::HARD>(audio_in, audio_out, gain, _prev_F1, _prev_x);
}


void SustainerBrick::set_samplerate(float samplerate)
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

void SustainerBrick::reset()
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

constexpr float DIODE_THRESHOLD = 0.66;
//constexpr float OP_FB_RES = 470;
//constexpr float JFET_OPEN_RES = 47000;
constexpr float JFET_V_CUTOFF = 5;
constexpr float CLIP_ASYMMETRY = 0.3;
//constexpr float SLEWRATE = 0.18;

constexpr float OPEN_LOOP_OP_GAIN = 300;
constexpr float OUTPUT_GAIN = 4;

constexpr float ENV_OPEN_RC = 2.2 * (0.00022 + 0.00068);
constexpr float ENV_CLOSED_RC = 470 * 0.00022;

//constexpr float OP_INTERNAL_LOWPASS = 33 * 0.000000250;

constexpr float STEREO_MIX_FACTOR = 0.85;

/* Simulate component variation by having slightly different values on left and right channels */
constexpr float COMPONENT_VARIATION = 0.03;
constexpr float COMP_VAR_1 = 3 * COMPONENT_VARIATION + 1.0;
//constexpr float COMP_VAR_2 = 5 * COMPONENT_VARIATION + 1.0;
constexpr float COMP_VAR_3 = 7 * COMPONENT_VARIATION + 1.0;

void SustainerBrick::render()
{
    constexpr int LEFT = 0;
    constexpr int RIGHT = 1;
    enum Mode
    {
        STEREO = 0,
        LINKED,
        MONO
    };

    float gain = 10 * to_db_approx(_ctrl_value(ControlInput::GAIN));
    float compression_param = _ctrl_value(ControlInput::COMPRESSION);
    float time_param = 1.0f - _ctrl_value(ControlInput::TIME) + 0.02f;
    time_param = time_param * time_param * 90.0f;

    int mode  =  control_to_range(_ctrl_value(ControlInput::STEREO_MODE), 0, 2);
    assert(mode == Mode::STEREO || mode == Mode::LINKED || mode == Mode::MONO);

    //float slewrate = 100;

    const AudioBuffer& audio_in_l = _input_buffer(0);
    const AudioBuffer& audio_in_r = _input_buffer(1);
    AudioBuffer& audio_out_l = _output_buffer(AudioOutput::LEFT_OUT);
    AudioBuffer& audio_out_r = _output_buffer(AudioOutput::RIGHT_OUT);

    auto op_gain = _op_gain;
    auto fet_gain = _fet_gain;
    auto op_hp = _op_hp;
    auto env_hp = _env_hp;
    auto env_lp = _env_lp;

    // scale down the gain with less compression for better controls
    gain *= (1.0f - 0.6f * (1.0f - compression_param));

    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        // gain control
        float x_l = audio_in_l[i] * gain;
        float x_r = audio_in_r[i] * gain;

        if (mode == Mode::MONO)
        {
            x_l = (x_l + x_r) * 0.5f;
        }

        // non-inv amp with soft clip
        float op_out_l = op_hp[LEFT].render_hp(x_l) * op_gain[LEFT];
        float op_out_r = op_hp[RIGHT].render_hp(x_r) * op_gain[RIGHT];
        float out_sample_l = sigm_hard(op_out_l * 0.5f + CLIP_ASYMMETRY * COMP_VAR_1) - CLIP_ASYMMETRY * COMP_VAR_1;
        float out_sample_r = sigm_hard(op_out_r * 0.5f + CLIP_ASYMMETRY) - CLIP_ASYMMETRY;
        float op_clip_l = out_sample_l * JFET_V_CUTOFF * compression_param;
        float op_clip_r = out_sample_r * JFET_V_CUTOFF * compression_param;

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
        // If diode it not conducting, C3 is discharged through R3
        // In schematic only the negative side is considered (half wave rectified)
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


void UnitDelayBrick::render()
{
    const AudioBuffer& audio_in = _input_buffer(0);
    AudioBuffer& audio_out = _output_buffer(AudioOutput::DELAY_OUT);
    audio_out = _unit_delay;
    _unit_delay = audio_in;
}

void ModulatedDelayBrick::set_max_delay_time(float max_delay_seconds)
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

void ModulatedDelayBrick::reset()
{
    _delay_time_lag.reset();
    std::fill(_buffer, _buffer + _max_samples, 0.0f);
}

void ModulatedDelayBrick::render()
{
    float current_time = clamp(_ctrl_value(ControlInput::DELAY_TIME), 0.01f, 1.0f);
    const auto& audio_in = _input_buffer(DEFAULT_INPUT);
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
    auto& audio_out = _output_buffer(AudioOutput::DELAY_OUT);

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

void BitRateReducerBrick::render()
{
    float bit_gain = std::exp2f(1.0f + _ctrl_value(ControlInput::BIT_DEPTH) * MAX_BIT_DEPTH);
    float gain_red = 1.0f / (bit_gain - 1.0f);
    const auto& audio_in = _input_buffer(0);
    auto& audio_out = _output_buffer(AudioOutput::BITRED_OUT);
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        audio_out[i] = static_cast<float>(static_cast<int>(audio_in[i] * bit_gain)) * gain_red;
    }
}

void SampleRateReducerBrick::reset()
{
    _down_phase = 0;
    _up_phase = 0;
    _delay_buffer.fill(0);
    _downsampled_buffer.fill(0);
}

void SampleRateReducerBrick::render()
{
    // Added to keep the upsampling phase from drifting away
    constexpr float NUDGE_FACTOR = 0.00005f;
    float ratio = clamp(control_to_freq(_ctrl_value(ControlInput::SAMPLE_RATE)) * _samplerate_inv * 2.0f, 0.0f, 1.0f);
    const auto& audio_in = _input_buffer(0);

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
    auto& audio_out = _output_buffer(AudioOutput::DOWNSAMPLED_OUT);

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
} // namespace bricks