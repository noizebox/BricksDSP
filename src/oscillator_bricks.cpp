#include <cmath>

#include "oscillator_bricks.h"
#include "data/wavetables_x4.h"

namespace bricks {

constexpr float OSC_BASE_FREQ = 20.0f;

void OscillatorBrick::render()
{
    float base_freq = OSC_BASE_FREQ * powf(2.0f, _pitch_port.value() * 10.0f);
    float phase_inc = base_freq / _samplerate;
    float phase = _phase;
    switch (_waveform)
    {
        case Waveform::SAW:
        {
            for (auto& sample : _buffer)
            {
                phase += phase_inc;
                if (phase > 0.5)
                    phase -= 1;
                sample = phase;
            }
            break;
        }
        case Waveform::PULSE:
        {
            for (auto& sample : _buffer)
            {
                phase += phase_inc;
                if (phase > 0.5)
                    phase -= 1;
                sample = std::signbit(phase) ? 0.5f : -0.5f;
            }
            break;
        }
        case Waveform::SINE:
        case Waveform::TRIANGLE:
        {
            int dir = _tri_dir;
            for (auto& sample : _buffer)
            {
                phase += phase_inc * dir * 2.0f;
                if (phase > 0.5)
                    dir = dir * -1;
                sample = phase;
            }
            _tri_dir = dir;
        }
    }
    _phase = phase;
}

void FmOscillatorBrick::render()
{
    float base_freq = OSC_BASE_FREQ * powf(2, _pitch_port.value() * 10);
    //_pitch_lag.set(_pitch_port.value());
    float phase_inc = base_freq / _samplerate;
    const AudioBuffer& fm_mod = _lin_fm_port.buffer();
    float phase = _phase;
    switch (_waveform)
    {
        case Waveform::SAW:
        {
            for (unsigned int i = 0; i < _buffer.size(); ++i)
            {
                phase += phase_inc * (1.0f + fm_mod[i]);
                if (phase > 0.5)
                    phase -= 1;
                _buffer[i] = phase;
            }
        }
            break;
        case Waveform::PULSE:
        {
            for (unsigned int i = 0; i < _buffer.size(); ++i)
            {
                phase += phase_inc * (1.0f + fm_mod[i]);
                if (phase > 0.5)
                    phase -= 1;
                _buffer[i] = std::signbit(phase) ? 0.5f : -0.5f;
            }
        }
            break;
        case Waveform::SINE:
        case Waveform::TRIANGLE:
        {
            int dir = _tri_dir;
            for (unsigned int i = 0; i < _buffer.size(); ++i)
            {
                phase += phase_inc * 2 * dir * (1.0f + fm_mod[i]);
                if (phase > 0.5)
                    dir = dir * -1;
                _buffer[i] = phase;
            }
            _tri_dir = dir;
        }

    }
    _phase = phase;
}

float cubic_herm_int(float A, float B, float C, float D, float pos)
{
    float a = -A/2.0f + (3.0f*B)/2.0f - (3.0f*C)/2.0f + D/2.0f;
    float b = A - (5.0f*B)/2.0f + 2.0f*C - D / 2.0f;
    float c = -A/2.0f + C/2.0f;
    float d = B;

    return a * pos * pos * pos + b * pos * pos + c * pos + d;
}

void WtOscillatorBrick::render()
{
    /* If the samplerate it less than 80kHz, use the wavetables 1 octave above
     * which has less harmonics to make sure they dont alias when interpolated */
    int wt_shift = _samplerate > 80000? 0 : _samplerate > 40000? 1 : 2;
    float base_freq = OSC_BASE_FREQ * powf(2, _pitch_port.value() * 10);
    float phase_inc = base_freq / _samplerate;
    float phase = _phase;
    int oct = std::max(0, std::min(9, static_cast<int>(_pitch_port.value() * 10) + wt_shift));
    float table_len = wavetables::lengths[oct];
    const float* table{nullptr};
    switch (_waveform)
    {
        case Waveform::SAW:
            table = wavetables::saw + wavetables::offsets[oct];
            break;
        case Waveform::PULSE:
            table = wavetables::square + wavetables::offsets[oct];
            break;
        case Waveform::TRIANGLE:
            table = wavetables::triangle + wavetables::offsets[oct];
            break;
        case Waveform::SINE:
            table = wavetables::sine + wavetables::offsets[oct];
            break;
    }
    assert(*table == 0.0f);

    for (unsigned int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        phase += phase_inc;
        if (phase > 1.0f)
            phase -= 1.0f;
        float pos = phase * table_len;
        int first = static_cast<int>(pos);
        float weight = pos - std::floor(pos);
        float sample = table[first] * (1.0f - weight) + table[first + 1] * weight;
        //float sample = cubic_herm_int(table[(first-1)%len],table[first], table[first+1], table[(first+2)%len], weight);
        _buffer[i] = sample;
    }
    _phase = phase;
}

} // namespace bricks