#ifndef BRICKS_DSP_ENVELOPE_BRICKS_H
#define BRICKS_DSP_ENVELOPE_BRICKS_H

#include <iostream>
#include <ostream>

#include "dsp_brick.h"
#include "random_device.h"

namespace bricks {

constexpr float SHORTEST_ENVELOPE_TIME = 1.0e-5f;
constexpr float LOWEST_LFO_SPEED = 0.05f;

/* Linear slope ADSR envelope generated at audio rate */
template <int os_factor = 1>
class AudioRateADSRBrick : public DspBrickv2Impl <os_factor, 1, 0, 0, 1>
{
public:
    AudioRateADSRBrick() = default;

    /* Not part of the general interface. Analogous to the gate signal on an analog
     * envelope. Setting gate to true will start the envelope in the attack phase
     * and setting it to false will start the release phase. */
    void gate(bool gate)
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


    bool finished()
    {
        return _state == EnvelopeState::OFF;
    }

    void set_samplerate(float samplerate) override
    {
        _samplerate = samplerate;
    }

    void reset() override
    {
        _state = EnvelopeState::OFF;
        _level = 0.0f;
    }

    void render(float attack, float decay, float sustain, float release,
                AudioOsBuffer<os_factor>& env_out)
    {
        float samplerate = _samplerate;
        float attack_factor = std::max(1.0f / (samplerate * attack), SHORTEST_ENVELOPE_TIME);
        float decay_factor = std::max((1.0f - sustain) / (samplerate * decay), SHORTEST_ENVELOPE_TIME);
        float sustain_level = sustain;
        float release_factor = std::max(sustain / (samplerate * release), SHORTEST_ENVELOPE_TIME);
        // TODO - Perhaps the release factor needs scaling if the envelope goes direcly to amp_release from attack or decay, see Apollo code

        for (auto& sample : env_out)
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

private:
    enum class EnvelopeState
    {
        OFF,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE,
    };

    EnvelopeState _state{EnvelopeState::OFF};
    float         _level{0};
    float         _samplerate{DEFAULT_SAMPLERATE};
};

/* Control rate linear ADSR envelope with linear slopes */
class LinearADSREnvelopeBrick : public DspBrickv2Impl<1, 4, 1, 0, 0>
{
public:
    LinearADSREnvelopeBrick() = default;

    /* Not part of the general interface. Analogous to the gate signal on an analog
     * envelope. Setting gate to true will start the envelope in the attack phase
     * and setting it to false will start the release phase. */
    void gate(bool gate)
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

    bool finished()
    {
        return _state == EnvelopeState::OFF;
    }

    void set_samplerate(float samplerate) override
    {
        _samplerate = samplerate;
    }

    void reset() override
    {
        _state = EnvelopeState::OFF;
        _level = 0.0f;
    }

    void render(float attack, float decay, float sustain, float release, float& env_out)
    {
        float level = _level;
        switch (_state)
        {
        case EnvelopeState::OFF:
            break;

        case EnvelopeState::ATTACK:
        {
            level += attack > 0 ? PROC_BLOCK_SIZE / (_samplerate * attack) : 1.0f;
            if (level >= 1)
            {
                _state = EnvelopeState::DECAY;
                level = 1.0f;
            }
            break;
        }

        case EnvelopeState::DECAY:
        {
            level -= decay > 0 ? sustain * PROC_BLOCK_SIZE / (_samplerate * decay) : 0.0f;
            if (level <= sustain)
            {
                _state = EnvelopeState::SUSTAIN;
                level = sustain;
            }
            break;
        }

        case EnvelopeState::SUSTAIN:
        {
            level = sustain;
            break;
        }

        case EnvelopeState::RELEASE:
        {
            level -= release > 0 ? sustain * PROC_BLOCK_SIZE / (_samplerate * release) : sustain;
            if (level <= 0.0f)
            {
                _state = EnvelopeState::OFF;
                level = 0.0f;
            }
            break;
        }
        }
        _level = level;
        env_out = _level;
    }
private:
    enum class EnvelopeState
    {
        OFF,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE,
    };

    EnvelopeState _state{EnvelopeState::OFF};
    float         _level{0};
    float         _samplerate{DEFAULT_SAMPLERATE};
};

/* ADSR Envelope intended for audio control with linear attack phase and
 * exponentially falling decay and release phases */
class AudioADSREnvelopeBrick : public DspBrickv2Impl<1, 4, 1, 0, 0>
{
public:
    AudioADSREnvelopeBrick() = default;

    /* Not part of the general interface. Analogous to the gate signal on an analog
     * envelope. Setting gate to true will start the envelope in the attack phase
     * and setting it to false will start the release phase. */
    void gate(bool gate)
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

    bool finished()
    {
        return _state == EnvelopeState::OFF;
    }

    void set_samplerate(float samplerate) override
    {
        _samplerate = samplerate;
    }

    void reset() override
    {
        _state = EnvelopeState::OFF;
        _level = 0.0f;
    }

    void render(float attack, float decay, float sustain, float release, float& env_out)
    {
        constexpr float ENVELOPE_EPS = 0.00001f;

        float samplerate = _samplerate;
        float level = _level;
        switch (_state)
        {
        case EnvelopeState::OFF:
            break;

        case EnvelopeState::ATTACK:
        {
            float attack_time = attack;
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
            sustain *= sustain;
            /* 1 - 1/x is a quick approximation of e^(-1 / x) for small values of x */
            float b0 = PROC_BLOCK_SIZE / (samplerate * decay);
            float a0 = 1.0f - b0;
            _level = _level * a0 + b0 * sustain;
            /* Note, as decay approaches the sustain level asymptotically,
             * we don't actually have to move to the sustain phase */
            break;
        }
        case EnvelopeState::RELEASE:
        {
            float a0 = 1.0f - PROC_BLOCK_SIZE / (samplerate * release);
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
        env_out = _level;
    }

private:
    enum class EnvelopeState
    {
        OFF,
        ATTACK,
        DECAY,
        RELEASE,
    };

    EnvelopeState _state{EnvelopeState::OFF};
    float         _level{0};
    float         _samplerate{DEFAULT_SAMPLERATE};
};


/* Control rate LFO  */
class LfoBrick : public DspBrickv2Impl<1, 1, 1, 0, 0>
{
public:
    enum class Waveform
    {
        SAW,
        PULSE,
        TRIANGLE,
        SINE,
        SAMPLE_HOLD, // Sampled noise
        NOISE,       // White noise
        RANDOM,      // lp filtered noise
    };

    LfoBrick() = default;

    void set_waveform(Waveform waveform)
    {
        _waveform = waveform;
    }

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override
    {
        _phase = 0;
        _level = 0;
    }

    void render(float rate, float& lfo_out)
    {
        float base_freq = LOWEST_LFO_SPEED * powf(2.0f, rate * 10.0f);
        float phase_inc = base_freq * PROC_BLOCK_SIZE * _samplerate_inv;
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
        lfo_out = level;
        _level = level;
        _phase = phase;
    }

private:
    float          _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float          _phase{0};
    float          _level{0};
    Waveform       _waveform{Waveform::TRIANGLE};
    int            _tri_dir{1};
    RandomDevice   _rand_device;
};

/* Optimised LFO with only sine as waveform */
class SineLfoBrick : public DspBrickv2Impl<1, 1, 1, 0, 0>
{
public:
    SineLfoBrick() = default;

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override
    {
        _phase = 0;
    }

    void render(float rate, float& lfo_out)
    {
        // TODO - Cheaper power function
        float base_freq = 2.0f * static_cast<float>(M_PI) * LOWEST_LFO_SPEED * powf(2.0f, rate * 10.0f);
        _phase += base_freq * PROC_BLOCK_SIZE * _samplerate_inv;
        lfo_out = std::sin(_phase);
        if (_phase > 2 * M_PI)
        {
            _phase -= 1.0f;
        }
    }

private:
    float _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float _phase{0};
};

/* Optimised low frequency random generation */
class RandLfoBrick : public DspBrickv2Impl<1, 0, 1, 0, 0>
{
public:
    RandLfoBrick() = default;

    void set_samplerate(float samplerate) override
    {
        _coeff_a0 = std::exp(-2.0f * static_cast<float>(M_PI) * LP_CUTOFF / (samplerate / PROC_BLOCK_SIZE));
    }

    void render(float& lfo_out)
    {
        float out = _rand_device.get_norm() * GAIN_COMP;
        float level = (1.0f - _coeff_a0) * out + _coeff_a0 * _level;
        lfo_out = level;
        _level = level;
    }

    static constexpr float LP_CUTOFF = 0.05f;
    static constexpr float GAIN_COMP = 100.0f;

private:
    float        _coeff_a0{0};
    float        _level{0};
    RandomDevice _rand_device;
};
}// namespace bricks

#endif //BRICKS_DSP_ENVELOPE_BRICKS_H
