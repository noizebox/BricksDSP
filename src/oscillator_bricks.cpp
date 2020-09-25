#include <cmath>

#include "oscillator_bricks.h"
#include "data/wavetables_x4.h"

namespace bricks {

void OscillatorBrick::render()
{
    float base_freq = control_to_freq(_pitch_port.value());
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
    float base_freq = control_to_freq(_pitch_port.value());
    //_pitch_lag.set(_pitch_port.value());
    float phase_inc = base_freq / _samplerate;
    const AudioBuffer& fm_mod = _lin_fm_port.buffer();
    float phase = _phase;
    switch (_waveform)
    {
        case Waveform::SAW:
        {
            for (int i = 0; i < _buffer.size(); ++i)
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
            for (int i = 0; i < _buffer.size(); ++i)
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
            for (int i = 0; i < _buffer.size(); ++i)
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

void WtOscillatorBrick::render()
{
    /* If the samplerate it less than 80kHz, use the wavetables 1 octave above
     * which has less harmonics to make sure they dont alias when interpolated */
    int wt_shift = _samplerate > 80000? 0 : _samplerate > 40000? 1 : 2;
    float base_freq = control_to_freq(_pitch_port.value());
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

    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        phase += phase_inc;
        if (phase > 1.0f)
            phase -= 1.0f;
        float pos = phase * table_len;
        float sample = linear_int(pos, table);
        _buffer[i] = sample;
    }
    _phase = phase;
}

constexpr float PINK_CUTOFF_FREQ = 100;
constexpr float PINK_GAIN_CORR = 4.0f;
constexpr float BROWN_CUTOFF_FREQ = 0.03;
constexpr float BROWN_GAIN_CORR = 50.0f;

void NoiseGeneratorBrick::render()
{
    for (auto& s : _buffer)
    {
        s = _rand_device.get_norm();
    }
    if (_waveform == Waveform::PINK)
    {
        float hist = _buffer[PROC_BLOCK_SIZE - 1];
        for (auto& s : _buffer)
        {
            s *= PINK_GAIN_CORR;
            s = (1.0f - _pink_coeff_a0) * s + _pink_coeff_a0 * hist;
            hist = s;
        }
    }
    if (_waveform == Waveform::BROWN)
    {
        float hist = _buffer[PROC_BLOCK_SIZE - 1];
        for (auto& s : _buffer)
        {
            s *= BROWN_GAIN_CORR;
            s = (1.0f - _brown_coeff_a0) * s + _brown_coeff_a0 * hist;
            hist = s;
        }
    }
}

void NoiseGeneratorBrick::set_samplerate(float samplerate)
{
    _pink_coeff_a0 = std::exp(-2.0f * M_PI * PINK_CUTOFF_FREQ / samplerate);
    _brown_coeff_a0 = std::exp(-2.0f * M_PI * BROWN_CUTOFF_FREQ / samplerate);
    DspBrick::set_samplerate(samplerate);
}

} // namespace bricks