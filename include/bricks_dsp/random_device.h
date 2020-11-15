#ifndef BRICKS_DSP_RANDOM_DEVICE_H
#define BRICKS_DSP_RANDOM_DEVICE_H

#include <random>

namespace bricks {

class RandomDevice
{
public:
    RandomDevice();

    uint32_t get() {return _device();}

    /* Random value normalised to [-1, 1] */
    float get_norm()
    {
        return (static_cast<float>(_device()) - FLOAT_MIN) / (FLOAT_MAX * 0.5f) - 1.0f;
    }
    static constexpr float FLOAT_MIN = static_cast<float>(std::minstd_rand::min());
    static constexpr float FLOAT_MAX = static_cast<float>(std::minstd_rand::max());

private:
    static uint32_t  _seed;
    std::minstd_rand _device;
};



} // namespace bricks

#endif //BRICKS_DSP_RANDOM_DEVICE_H
