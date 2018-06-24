#ifndef BRICKS_DSP_UTILS_H
#define BRICKS_DSP_UTILS_H

#include <cmath>

namespace bricks {

/* Map a float in range [0, 1] to true or false */
inline bool control_to_bool(float v)
{
    return v > 0.5f;
}

/* Map a float in range [0, 1] to a given, linear, integer range */
inline int control_to_range(float v, int min_range, int max_range)
{
    return std::round(v * (max_range - min_range) - min_range);
}

/* Map a midi note number to a float in the range [0, 1] where 0.0 is
 * 20 hz and scales with 0.1 / octave */
inline float note_to_control(int midi_note)
{
    constexpr float SEMITONES_IN_RANGE = 120.0f;
    constexpr float SHIFT_FACTOR = 15.486822;
    return (midi_note - SHIFT_FACTOR) / SEMITONES_IN_RANGE;
}

/* Proper mapping from a linear range to a 60 dB exponential response */
inline float to_db(float lin)
{
    constexpr float DB_RANGE = -60.0f;
    return powf(10, (1.0f - lin) * DB_RANGE / 20.0f);
}

/* For vcas and audio controls, x³ is a pretty good approximation of a
 * 30 dB exponential response */
inline float to_db_aprox(float lin)
{
    return lin * lin * lin;
}

/* clamp/clip a value between min and max. With -ffast-math this seems to
 * compile to branchless and very efficent code for use on an audio buffer */
inline float clamp(float x, float min, float max)
{
    if (x > max)
    {
        x = max;
    }
    if (x < min)
    {
        x = min;
    }
    return x;
}

/* Linear interpolations over N samples */
template <size_t length>
class LinearInterpolator
{
public:
    void set(float target) {_step = (target - _lag) / static_cast<float>(length);}

    float get() {return _lag += _step;};

    std::array<float, length> get_all()
    {
        std::array<float, length> values;
        for (auto& v : values)
        {
            v = this->get();
        }
        return values;
    };

private:
    float _lag{0};
    float _step{0};
};

/* 1 pole filtering over N samples */
template <size_t length>
class OnePoleLag
{
public:
    void set(float target) {_target = target;}

    float get() {return _lag = COEFF_B0 * _target + COEFF_A0 * _lag;}

    std::array<float, length> get_all()
    {
        std::array<float, length> values;
        for (auto& v : values)
        {
            v = this->get();
        }
        return values;
    };

private:
// TODO - Room for tweaking.
static constexpr float TIMECONSTANTS_PER_BLOCK = 2.5;
const float COEFF_A0 = std::exp(-1.0f * TIMECONSTANTS_PER_BLOCK / length);
const float COEFF_B0 = 1 - COEFF_A0;

    float _target{0};
    float _lag{0};
};

} // namespace bricks

#endif //BRICKS_DSP_UTILS_H
