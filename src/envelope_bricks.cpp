#include <cmath>

#include "envelope_bricks.h"
namespace bricks {

constexpr float SHORTEST_ENVELOPE_TIME = 1.0e-5f;

void ADSREnvelopeBrick::gate(bool gate)
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

void ADSREnvelopeBrick::render()
{
    float attack_factor = std::max(1 / (SAMPLERATE * *_controls[ATTACK]), SHORTEST_ENVELOPE_TIME);
    float decay_factor = std::max((1.0f - *_controls[SUSTAIN]) / (SAMPLERATE * *_controls[DECAY]), SHORTEST_ENVELOPE_TIME);
    float sustain_level = *_controls[SUSTAIN];
    float release_factor = std::max(*_controls[SUSTAIN] / (SAMPLERATE * *_controls[RELEASE]), SHORTEST_ENVELOPE_TIME);
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

} // namespace bricks