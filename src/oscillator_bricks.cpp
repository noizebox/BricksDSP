#include <cmath>

#include "oscillator_bricks.h"

namespace bricks {

constexpr float OSC_BASE_FREQ = 20.0f;

void OscillatorBrick::render()
{
    float base_freq = OSC_BASE_FREQ * powf(2, _pitch_port.value() * 10);
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



} // namespace bricks