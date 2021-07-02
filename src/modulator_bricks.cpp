#include <algorithm>
#include <cmath>
#include <iostream>

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
    DspBrickImpl::set_samplerate(samplerate);
    /* Values directly from the schematic */
    _op_hp.set(470.0 * 0.001, samplerate, true);
    _op_lp.set(33 * 0.000000250, samplerate, true);
    _env_hp.set((22.2 + 2.2) * 0.00068, samplerate, true);
    _env_lp.set(470.0 * 0.00022, samplerate, true);
}

void SustainerBrick::reset()
{
    _op_hp.reset();
    _op_lp.reset();
    _env_hp.reset();
    _env_lp.reset();
}

constexpr float DIODE_THRESHOLD = 0.66;
constexpr float OP_FB_RES = 470;
constexpr float JFET_OPEN_RES = 47000;
constexpr float JFET_V_CUTOFF = 5;
constexpr float CLIP_ASYMMETRY = 0.3;
constexpr float SLEWRATE = 0.18;

constexpr float OPEN_LOOP_OP_GAIN = 300;
constexpr float OUTPUT_GAIN = 4;

constexpr float ENV_OPEN_RC = 2.2 * (0.00022 + 0.00068);
constexpr float ENV_CLOSED_RC = 470 * 0.00022;

constexpr float OP_INTERNAL_LOWPASS = 33 * 0.000000250;

void SustainerBrick::render()
{
    float gain = 10 * to_db_approx(_ctrl_value(ControlInput::GAIN));
    float asymmetry = CLIP_ASYMMETRY;
    float compression_param = _ctrl_value(ControlInput::COMPRESSION);
    float time_param = 1.0f - _ctrl_value(ControlInput::TIME) + 0.02f;
    time_param *= time_param;

    //float slewrate = 100;

    const AudioBuffer& audio_in = _input_buffer(DEFAULT_INPUT);
    AudioBuffer& audio_out = _output_buffer(AudioOutput::SUSTAIN_OUT);
    float op_gain = _op_gain;
    float fet_gain = _fet_gain;
    float prev_audio_out = _prev_op_out;
    float lag_hist = _lag_hist;
    bool plot = true;

    // scale down the gain with less compression for better controls
    gain *= (1.0f - 0.6f * (1.0f - compression_param));

    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        // gain control
        float x = audio_in[i] * gain;

        // non-inv amp with soft clip
        float op_out = _op_hp.render_hp(x) * op_gain;
        float audio_out_sample = sigm_hard(op_out * 0.5f + asymmetry) - asymmetry;

        // Slew rate method 2, filter sample by diff of prev sample, actually gives a decent lp
        /*float diff = std::abs(audio_out_sample - prev_audio_out) - slewrate;
        float slew_diff = std::clamp(diff * 8, 0.0f, 0.90f);
        audio_out_sample = audio_out_sample * (1.0f - slew_diff) + prev_audio_out * slew_diff;
        prev_audio_out = audio_out_sample;*/

        //audio_out_sample = _op_lp.render_lp(audio_out_sample);

        float op_clip = audio_out_sample * JFET_V_CUTOFF * compression_param;


        audio_out[i] = audio_out_sample * OUTPUT_GAIN;

        // calc feedback env follower,
        float env_in = _env_hp.render_hp(op_clip);

        // Simplest diode model (hard knee)
        // If diode is conducting, R5 and C3 forms a lp filter
        // If diode it not conducting, C3 is discharged through R3
        // In schematic only the negative side is considered (half wave rectified)
        float rect;
        float cur_env = _env_lp.state();

        if (env_in < -1.0f * (cur_env + DIODE_THRESHOLD))
        {
            rect = - (env_in + DIODE_THRESHOLD);
            _env_lp.set_approx(ENV_OPEN_RC * time_param * 90.0f, samplerate(), false);
        }
        else
        {
            rect = 0.0f;
            _env_lp.set_approx(ENV_CLOSED_RC * time_param * 90.0f, samplerate(), false);
        }

        fet_gain = _env_lp.render_lp(std::min(rect, JFET_V_CUTOFF + 1));
        float gain_ctrl = bricks::clamp(fet_gain * 2.5f, 0, JFET_V_CUTOFF);

        // calc new op-gain
        op_gain = 1.0f + 0.9f * (JFET_V_CUTOFF - gain_ctrl) / JFET_V_CUTOFF * OPEN_LOOP_OP_GAIN;

        if (plot)
        {
            //std::cout << "SDD: gain: "<< gain << ", op_gain: " << op_gain << ", gain_ctrl: " << gain_ctrl << ", rect: " << rect << std::endl;
            plot = false;
        }
    }
    _op_gain = op_gain;
    _fet_gain = fet_gain;
    _prev_op_out = prev_audio_out;
    _lag_hist = lag_hist;

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
    size_t samples = max_delay_seconds * samplerate();
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
    float ratio = clamp(control_to_freq(_ctrl_value(ControlInput::SAMPLE_RATE)) / samplerate() * 2.0f, 0.0f, 1.0f);
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