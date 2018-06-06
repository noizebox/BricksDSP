#include <cmath>

#include "envelope_bricks.h"
namespace bricks {

constexpr float SHORTEST_ENVELOPE_TIME = 1.0e-5f;

constexpr float LOWEST_LFO_SPEED = 0.05f;

void AudioRateADSRBrick::gate(bool gate)
{
    if (gate) /* If the envelope is running, it's simply restarted here */
    {
        _state = EnvelopeState::ATTACK;
        _level = 0.0f;
    } else /* Gate off - go to release phase */
    {
        _state = EnvelopeState::RELEASE;
    }
}

void AudioRateADSRBrick::render()
{
    float attack_factor = std::max(1 / (_samplerate * _controls[ATTACK].value()), SHORTEST_ENVELOPE_TIME);
    float decay_factor = std::max((1.0f - _controls[SUSTAIN].value()) / (_samplerate * _controls[DECAY].value()), SHORTEST_ENVELOPE_TIME);
    float sustain_level = _controls[SUSTAIN].value();
    float release_factor = std::max(_controls[SUSTAIN].value() / (_samplerate * _controls[RELEASE].value()), SHORTEST_ENVELOPE_TIME);
    // TODO - Perhaps the release factor needs scaling if the envelope goes direcly to release from attack or decay, see Apollo code

    for (auto& sample : _envelope)
    {
        switch (_state)
        {
        case EnvelopeState::OFF:
            break;

        case EnvelopeState::ATTACK:
            _level += attack_factor;
            if (_level >= 1)
            {
                _state = EnvelopeState::DECAY;
                _level = 1.0f;
            }
            break;

        case EnvelopeState::DECAY:
            _level -= decay_factor;
            if (_level <= sustain_level)
            {
                _state = EnvelopeState::SUSTAIN;
                _level = sustain_level;
            }
            break;

        case EnvelopeState::SUSTAIN:
            /* fixed level, wait for a gate release/note off */
            break;

        case EnvelopeState::RELEASE:
            _level -= release_factor;
            if (_level < 0.0f)
            {
                _state = EnvelopeState::OFF;
                _level = 0.0f;
            }
            break;
        }
        sample = _level;
    }
}

void LinearADSREnvelopeBrick::gate(bool gate)
{
    if (gate) /* If the envelope is running, it's simply restarted here */
    {
        _state = EnvelopeState::ATTACK;
        _level = 0.0f;
    } else /* Gate off - go to release phase */
    {
        _state = EnvelopeState::RELEASE;
    }
}

void LinearADSREnvelopeBrick::render()
{
    float level = _level;
    switch (_state)
    {
        case EnvelopeState::OFF:
            break;

        case EnvelopeState::ATTACK:
        {
            float attack_time = _controls[ATTACK].value();
            level += attack_time > 0 ? PROC_BLOCK_SIZE / (_samplerate * attack_time) : 1.0f;
            if (level >= 1)
            {
                _state = EnvelopeState::DECAY;
                level = 1.0f;
            }
            break;
        }

        case EnvelopeState::DECAY:
        {
            float decay_time = _controls[DECAY].value();
            float sustain_level = _controls[SUSTAIN].value();
            level -= decay_time > 0 ? sustain_level * PROC_BLOCK_SIZE / (_samplerate * decay_time) : 0;
            if (level <= sustain_level)
            {
                _state = EnvelopeState::SUSTAIN;
                level = sustain_level;
            }
            break;
        }

        case EnvelopeState::SUSTAIN:
        {
            level = _controls[SUSTAIN].value();
            break;
        }

        case EnvelopeState::RELEASE:
        {
            float release_time = _controls[RELEASE].value();
            float sustain_level = _controls[SUSTAIN].value();
            level -= release_time > 0 ? sustain_level * PROC_BLOCK_SIZE / (_samplerate * release_time) : 0;
            if (level <= 0.0f)
            {
                _state = EnvelopeState::OFF;
                level = 0.0f;
            }
            break;
        }
    }
    _level = level;
}

void AudioADSREnvelopeBrick::gate(bool gate)
{
    if (gate) /* If the envelope is running, it's simply restarted here */
    {
        _state = EnvelopeState::ATTACK;
        _level = 0.0f;
    } else /* Gate off - go to release phase */
    {
        _state = EnvelopeState::RELEASE;
    }
}

constexpr float ENVELOPE_EPS = 0.00001f;

void AudioADSREnvelopeBrick::render()
{
    switch (_state)
    {
        case EnvelopeState::OFF:
            break;

        case EnvelopeState::ATTACK:
        {
            float attack_time = _attack.value();
            _level += attack_time > 0 ? PROC_BLOCK_SIZE / (_samplerate * attack_time) : 1.0f;
            if (_level >= 1)
            {
                _state = EnvelopeState::DECAY;
                _level = 1.0f;
            }
            break;
        }

        case EnvelopeState::DECAY:
        {
            float decay_rate = _decay.value();
            float sustain_level = _sustain.value();
            sustain_level *= sustain_level;
            /* 1 - 1/x is a quick approximation of e^(-1 / x) for small values of x */
            float b0 = PROC_BLOCK_SIZE / (_samplerate * decay_rate);
            float a0 = 1.0f - b0;
            _level = _level * a0 + b0 * sustain_level;
            /* Note, as decay approaches the sustain level asymptotically,
             * we don't actually have to move to the sustain phase */
            break;
        }
        case EnvelopeState::RELEASE:
        {
            float release_rate = _release.value();
            float a0 = 1.0f - PROC_BLOCK_SIZE / (_samplerate * release_rate);
            _level = _level * a0;
            if (_level < ENVELOPE_EPS)
            {
                _state = EnvelopeState::OFF;
                _level = 0.0f;
            }
            break;
        }
    }
}

void LfoBrick::render()
{
    float base_freq = LOWEST_LFO_SPEED * powf(2.0f, _rate_port.value() * 10.0f);
    float phase_inc = base_freq * PROC_BLOCK_SIZE / _samplerate;
    float phase = _phase;
    switch (_waveform)
    {
        case Waveform::SAW:
            phase += phase_inc;
            if (phase > 0.5)
                phase -= 1;
            _level = phase;
            break;

        case Waveform::PULSE:
            phase += phase_inc;
            if (phase > 0.5)
                phase -= 1;
            _level = std::signbit(phase) ? 0.5f : -0.5f;
            break;

        case Waveform::SINE:
        case Waveform::TRIANGLE:
            phase += phase_inc * _tri_dir * 2.0f;
            if (std::abs(phase) > 0.5)
            {
                _tri_dir = _tri_dir * -1;
            }
            _level = phase;
    }
    _phase = phase;
}
} // namespace bricks