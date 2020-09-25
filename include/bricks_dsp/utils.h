#ifndef BRICKS_DSP_UTILS_H
#define BRICKS_DSP_UTILS_H
#include <cmath>

#include "aligned_array.h"

namespace bricks {

/* Map a float in range [0, 1] to true or false */
inline bool control_to_bool(float v)
{
    return v > 0.5f;
}

/* Map a float in range [0, 1] to a given, linear, integer range */
inline int control_to_range(float v, int min_range, int max_range)
{
    return std::round(v * (max_range - min_range) + min_range);
}

/* Map a midi note number to a float in the range [0, 1] where 0.0 is
 * 20 hz and scales with 0.1 / octave */
inline constexpr float note_to_control(int midi_note)
{
    constexpr float SEMITONES_IN_RANGE = 120.0f;
    constexpr float SHIFT_FACTOR = 15.486822;
    return (midi_note - SHIFT_FACTOR) / SEMITONES_IN_RANGE;
}

/* Map a control input to a 0.1 per octave pitch control to a frequency in Hz */
inline float control_to_freq(float v)
{
    constexpr float OSC_BASE_FREQ = 20.0f;
    return OSC_BASE_FREQ * powf(2, v * 10.0f);
}

/* Proper mapping from a linear range to a 60 dB exponential response */
inline float to_db(float lin)
{
    constexpr float DB_RANGE = -60.0f;
    return powf(10, (1.0f - lin) * DB_RANGE / 20.0f);
}

/* For vcas and audio controls, x³ is a pretty good approximation of a
 * 30 dB exponential response */
inline float to_db_approx(float lin)
{
    return lin * lin * lin;
}

/* clamp/clip a value between min and max. With -ffast-math this seems to
 * compile to branchless and very efficient code for use on an audio buffer
 * Same as std::clamp, but faster with clang for some weird reason */
inline float clamp(const float& x, const float& min, const float& max)
{
    if (x > max)
    {
        return max;
    }
    if (x < min)
    {
        return min;
    }
    return x;
}

/* Linear interpolations over N samples */
template <int length>
class LinearInterpolator
{
public:
    void set(float target) {_step = (target - _lag) / static_cast<float>(length);}

    float get() {return _lag += _step;};

    AlignedArray<float, length> get_all()
    {
        AlignedArray<float, length> values;
        for (auto& v : values)
        {
            v = this->get();
        }
        return values;
    };

    float step() {return _step;}
private:
    float _lag{0};
    float _step{0};
};

/* 1 pole filtering over N samples */
template <int length>
class OnePoleLag
{
public:
    void set(float target) {_target = target;}

    float get() {return _lag = COEFF_B0 * _target + COEFF_A0 * _lag;}

    AlignedArray<float, length> get_all()
    {
        AlignedArray<float, length> values;
        for (auto& v : values)
        {
            v = this->get();
        }
        return values;
    };

private:
// TODO - Room for tweaking.
static constexpr float TIMECONSTANTS_PER_BLOCK = 2.5;
#ifdef LINUX
static constexpr float COEFF_A0 = std::exp(-1.0f * TIMECONSTANTS_PER_BLOCK / length);
static constexpr float COEFF_B0 = 1 - COEFF_A0;
#endif
#ifdef WINDOWS
static inline const float COEFF_A0 = std::exp(-1.0f * TIMECONSTANTS_PER_BLOCK / length);
static inline const float COEFF_B0 = 1 - COEFF_A0;
#endif

    float _target{0};
    float _lag{0};
};

/* Interpolation functions, these assume that data points to a continuous
 * buffer and will interpolate a position in the buffer.
 *
 * The first order functions need to access data[pos] and data[pos + 1]
 * The second order functions access data[pos - 1] to data[pos + 2]
 * These data accesses must not result in an out of bounds error */

/* 0:th order hold 'interpolation' (no interpolation) */
template <typename T>
inline T zeroth_int(T pos, const T* data)
{
    return data[static_cast<int>(pos)];
}

/* First order linear interpolation */
template <typename T>
inline T linear_int(T pos, const T* data)
{
    auto first = static_cast<int>(pos);
    T frac = pos - std::floor(pos);
    T d1 = data[first];
    T d2 = data[first + 1];
    return d1 + frac * (d2 - d1);
}

/* Normal cubic interpolation */
template <typename T>
inline T cubic_int(T pos, T* data)
{
    auto first = static_cast<int>(pos);
    T frac = pos - std::floor(pos);

    T d0 = data[first - 1];
    T d1 = data[first];
    T d2 = data[first + 1];
    T d3 = data[first + 2];

    T f2 = frac * frac;
    T a0 = d3 - d2 - d0 + d1;
    T a1 = d0 - d1 - a0;
    T a2 = d2 - d0;
    T a3 = d1;

    return(a0 * frac * f2 + a1 * f2 + a2 * frac + a3);
}

/* Catmull-Rom splines, aka Hermite interpolation (Second order) */
template <typename T>
inline T catmull_rom_cubic_int(T pos, T* data)
{
    auto first = static_cast<int>(pos);
    T frac = pos - std::floor(pos);

    T d0 = data[first - 1];
    T d1 = data[first];
    T d2 = data[first + 1];
    T d3 = data[first + 2];

    T f2 = frac * frac;
    T a0 = -0.5f * d0 + 1.5f * d1 - 1.5f * d2 + 0.5f * d3;
    T a1 = d0 - 2.5f * d1 + 2.0f * d2 - 0.5f * d3;
    T a2 = -0.5f * d0 + 0.5f * d2;
    T a3 = d1;

    return(a0 * frac * f2 + a1 * f2 + a2 * frac + a3);
}

/* Cosine interpolation, good for smooth transitions */
template <typename T>
inline T cosine_int(T pos, T* data)
{
    auto first = static_cast<int>(pos);
    T frac = pos - std::floor(pos);
    T d1 = data[first];
    T d2 = data[first + 1];

    T f2 = (1.0f - std::cos(frac * static_cast<T>(M_PI))) / 2.0f;
    return(d1 * (1.0f - f2) + d2 * f2);
}

} // namespace bricks

#endif //BRICKS_DSP_UTILS_H
