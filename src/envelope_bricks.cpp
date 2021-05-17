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
    float attack = _ctrl_value(ControlInput::ATTACK);
    float decay = _ctrl_value(ControlInput::RELEASE);
    float sustain = _ctrl_value(ControlInput::SUSTAIN);
    float release = _ctrl_value(ControlInput::RELEASE);
    float samplerate = this->samplerate();
    float attack_factor = std::max(1.0f / (samplerate * attack), SHORTEST_ENVELOPE_TIME);
    float decay_factor = std::max((1.0f - sustain) / (samplerate * decay), SHORTEST_ENVELOPE_TIME);
    float sustain_level = sustain;
    float release_factor = std::max(sustain / (samplerate * release), SHORTEST_ENVELOPE_TIME);
    // TODO - Perhaps the release factor needs scaling if the envelope goes direcly to amp_release from attack or decay, see Apollo code

    AudioBuffer& out = _output_buffer(AudioOutput::ENV_OUT);

    for (auto& sample : out)
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
    float samplerate = this->samplerate();
    switch (_state)
    {
        case EnvelopeState::OFF:
            break;

        case EnvelopeState::ATTACK:
        {
            float attack_time = _ctrl_value(ControlInput::ATTACK);
            level += attack_time > 0 ? PROC_BLOCK_SIZE / (samplerate * attack_time) : 1.0f;
            if (level >= 1)
            {
                _state = EnvelopeState::DECAY;
                level = 1.0f;
            }
            break;
        }

        case EnvelopeState::DECAY:
        {
            float decay_time = _ctrl_value(ControlInput::DECAY);
            float sustain_level = _ctrl_value(ControlInput::SUSTAIN);
            level -= decay_time > 0 ? sustain_level * PROC_BLOCK_SIZE / (samplerate * decay_time) : 0.0f;
            if (level <= sustain_level)
            {
                _state = EnvelopeState::SUSTAIN;
                level = sustain_level;
            }
            break;
        }

        case EnvelopeState::SUSTAIN:
        {
            level = _ctrl_value(ControlInput::SUSTAIN);
            break;
        }

        case EnvelopeState::RELEASE:
        {
            float release_time = _ctrl_value(ControlInput::RELEASE);
            float sustain_level = _ctrl_value(ControlInput::SUSTAIN);
            level -= release_time > 0 ? sustain_level * PROC_BLOCK_SIZE / (samplerate * release_time) : sustain_level;
            if (level <= 0.0f)
            {
                _state = EnvelopeState::OFF;
                level = 0.0f;
            }
            break;
        }
    }
    _level = level;
    _set_ctrl_value(ControlOutput::ENV_OUT, level);
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
    float samplerate = this->samplerate();
    float level = _level;
    switch (_state)
    {
        case EnvelopeState::OFF:
            break;

        case EnvelopeState::ATTACK:
        {
            float attack_time = _ctrl_value(ControlInput::ATTACK);
            _level += attack_time > 0 ? PROC_BLOCK_SIZE / (samplerate * attack_time) : 1.0f;
            if (_level >= 1)
            {
                _state = EnvelopeState::DECAY;
                _level = 1.0f;
            }
            break;
        }

        case EnvelopeState::DECAY:
        {
            float decay_rate = _ctrl_value(ControlInput::DECAY);
            float sustain_level = _ctrl_value(ControlInput::SUSTAIN);
            sustain_level *= sustain_level;
            /* 1 - 1/x is a quick approximation of e^(-1 / x) for small values of x */
            float b0 = PROC_BLOCK_SIZE / (samplerate * decay_rate);
            float a0 = 1.0f - b0;
            _level = _level * a0 + b0 * sustain_level;
            /* Note, as decay approaches the sustain level asymptotically,
             * we don't actually have to move to the sustain phase */
            break;
        }
        case EnvelopeState::RELEASE:
        {
            float release_rate = _ctrl_value(ControlInput::RELEASE);
            float a0 = 1.0f - PROC_BLOCK_SIZE / (samplerate * release_rate);
            _level = _level * a0;
            if (_level < ENVELOPE_EPS)
            {
                _state = EnvelopeState::OFF;
                _level = 0.0f;
            }
            break;
        }
    }
    _level = level;
    _set_ctrl_value(ControlOutput::ENV_OUT, level);
}


void LfoBrick::render()
{
    float base_freq = LOWEST_LFO_SPEED * powf(2.0f, _ctrl_value(ControlInput::RATE) * 10.0f);
    float phase_inc = base_freq * PROC_BLOCK_SIZE / samplerate();
    float phase = _phase;
    float level = _level;
    switch (_waveform)
    {
        case Waveform::SAW:
            phase += phase_inc;
            if (phase > 0.5)
                phase -= 1;
            level = phase;
            break;

        case Waveform::PULSE:
            phase += phase_inc;
            if (phase > 0.5)
                phase -= 1;
            level = std::signbit(phase) ? 0.5f : -0.5f;
            break;

        case Waveform::TRIANGLE:
            phase += phase_inc * _tri_dir * 2.0f;
            if (std::abs(phase) > 0.5)
            {
                _tri_dir = _tri_dir * -1;
            }
            level = phase;
            break;

        case Waveform::SINE:
            phase += phase_inc;
            if (phase > 0.5)
                phase -= 1;
            level = std::sin(phase * 2.0f * static_cast<float>(M_PI));
            break;

        case Waveform::SAMPLE_HOLD:
            phase += phase_inc;
            if (phase > 0.5)
            {
                phase -= 1;
                level = _rand_device.get_norm();
            }

        case Waveform::NOISE:
            level = _rand_device.get_norm();
            break;

        case Waveform::RANDOM:
            float out = _rand_device.get_norm() * 100;
            level = out = (1.0f - 0.9997f) * out + 0.9997f * level;
            break;
    }
    _level = level;
    _phase = phase;
    _set_ctrl_value(ControlOutput::LFO_OUT, level);
}

void SineLfoBrick::render()
{
    // TODO - Cheaper power function
    float rate = _ctrl_value(ControlInput::RATE);
    float base_freq = 2.0f * static_cast<float>(M_PI) * LOWEST_LFO_SPEED * powf(2.0f, rate * 10.0f);
    _phase += base_freq * PROC_BLOCK_SIZE / samplerate();
    _set_ctrl_value(ControlOutput::LFO_OUT, std::sin(_phase));
    if (_phase > 2 * M_PI)
    {
        _phase -= 1.0f;
    }
}

void RandLfoBrick::set_samplerate(float samplerate)
{
    DspBrickImpl::set_samplerate(samplerate);
    _coeff_a0 = std::exp(-2.0f * static_cast<float>(M_PI) * LP_CUTOFF / (samplerate / PROC_BLOCK_SIZE));
}
} // namespace bricks