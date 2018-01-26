#ifndef BRICKS_DSP_UTILS_H
#define BRICKS_DSP_UTILS_H

namespace bricks {

inline bool control_to_bool(float v)
{
    return v > 0.5f;
}

inline int control_to_range(float v, int min_range, int max_range)
{
    return std::round(v * (max_range - min_range) - min_range);
}

} // namespace bricks

#endif //BRICKS_DSP_UTILS_H
