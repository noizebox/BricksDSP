#include "random_device.h"

namespace bricks {

constexpr uint32_t INITIAL_SEED = 12345;

uint32_t RandomDevice::_seed;

RandomDevice::RandomDevice()
{
    /* Each device is initialised with a different seed to get independent
     * random generators / noise */
    _device.seed(_seed);
    ++_seed;
}

} // namespace bricks