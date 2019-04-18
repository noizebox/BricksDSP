#include "modulator_bricks.h"

namespace bricks {


void SaturationBrick::render()
{
    const AudioBuffer& in = _audio_in.buffer();
    float comp = _clip_ctrl.value();
    float clip_level = 1.0f / _clip_ctrl.value();
    for (unsigned int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        float x = in[i] * clip_level;
        x = clamp(x, -3.0f, 3.0f);
        _audio_out[i] = comp * x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
    }
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

    for (unsigned int i = 0; i < PROC_BLOCK_SIZE; ++i)
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