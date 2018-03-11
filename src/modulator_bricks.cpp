#include "modulator_bricks.h"

namespace bricks {


void SaturationBrick::render()
{
    const AudioBuffer& in = _audio_in.buffer();
    float comp = _clip_ctrl.value();
    float clip_level = 1.0f / comp;
    for (unsigned int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        float x = in[i] * clip_level;
        _audio_out[i] =  comp * x / (1.0f + 0.26f * (x * x));
    }

}
} // namespace bricks