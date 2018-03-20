#include "modulator_bricks.h"

namespace bricks {


void SaturationBrick::render()
{
    const AudioBuffer& in = _audio_in.buffer();
    float comp = _clip_ctrl.value();
    float clip_level = 1.0f / _clip_ctrl.value();
    for (unsigned int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        float x = in[i] * clip_level;
        x = clamp(x, -3.0f, 3.0f);
        _audio_out[i] = comp * x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
    }

}
} // namespace bricks