#ifndef BRICKS_DSP_UTILS_H
#define BRICKS_DSP_UTILS_H

#include <cmath>

namespace bricks {

inline bool control_to_bool(float v)
{
    return v > 0.5f;
}

inline int control_to_range(float v, int min_range, int max_range)
{
    return std::round(v * (max_range - min_range) - min_range);
}


/* Linear interpolations over N samples */
template <size_t length>
class LinearInterpolator
{
public:
    void set(float target) {_step = (target - _lag) / length;}

    float get() {return _lag += _step;};

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

private:
// TODO - Room for tweaking.
static constexpr float TIMECONSTANTS_PER_BLOCK = 2.5;
static constexpr float COEFF_A0 = std::exp(-1.0f * TIMECONSTANTS_PER_BLOCK / length);
static constexpr float COEFF_B0 = 1 - COEFF_A0;

    float _target{0};
    float _lag{0};
};

} // namespace bricks

#endif //BRICKS_DSP_UTILS_H
