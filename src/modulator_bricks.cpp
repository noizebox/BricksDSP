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