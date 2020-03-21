#include "modulator_bricks.h"
#include <algorithm>
#include <iostream>


namespace bricks {

// Pade approximation of tanh, valid within [-3, 3]
inline float tanh_approx(const float& x)
{
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

inline double tanh_antiderivative(const double& x)
{
    return std::log(std::cosh(x));
    //return 14.0f / 9.0f * std::log(std::abs(9 * x * x + 27));
}

// One of the computationally cheapest soft clipping functions
inline float sigm(const float& x)
{
    return x / std::sqrt(1 + x * x);
}

// Integral of the above sigmoid clipping function
inline float sigm_antiderivative(const float& x)
{
    // Todo -  verify if this is the correct integral of sigm()
    return std::sqrt(1 + x * x);
}

// Primitive function of a rectangular clip function that is 1 for x > 1, -1 for x < -1 and x otherwise
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
    const AudioBuffer& in = _audio_in.buffer();
    float gain = _gain.value();
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        float x = in[i] * gain;
        x = clamp(x, -3.0f, 3.0f);
        _audio_out[i] = tanh_approx(x);
    }
}

template<>
void SaturationBrick<ClipType::HARD>::render()
{
    const AudioBuffer& in = _audio_in.buffer();
    float gain = _gain.value();
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        float x = in[i] * gain;
        _audio_out[i] = clamp(x, -1.0f, 1.0f);
    }
}

template <ClipType type>
void render_saturation_aa(const AudioBuffer& in, AudioBuffer& out, float gain, float& prev_F1, float& prev_x)
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
                y = std::clamp(x, -1.0f, 1.0f);
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
    float gain = _gain.value();
    render_saturation_aa<ClipType::SOFT>(_audio_in.buffer(), _audio_out, gain, _prev_F1, _prev_x);
}

template <>
void AASaturationBrick<ClipType::HARD>::render()
{
    float gain = _gain.value();
    render_saturation_aa<ClipType::HARD> (_audio_in.buffer(), _audio_out, gain, _prev_F1, _prev_x);
}

void UnitDelayBrick::render()
{
    assert(_audio_in != nullptr);
    _audio_out = _unit_delay;
    _unit_delay = *_audio_in;

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

inline float linear_int(float pos, const float*data)
{
    int first = static_cast<int>(pos);
    float weight = pos - std::floor(pos);
    return data[first] * (1.0f - weight) + data[first + 1] * weight;
}

void ModulatedDelayBrick::render()
{
    float current_time = clamp(_delay_ctrl.value(), 0.01f, 1.0f);
    std::copy(_audio_in.buffer().data(), &_audio_in.buffer().data()[PROC_BLOCK_SIZE], &_buffer[_rec_head]);
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
        _audio_out[i] = linear_int(_play_head, _buffer);
    }

}
} // namespace bricks