#ifndef BRICKS_DSP_DSP_BRICK_H
#define BRICKS_DSP_DSP_BRICK_H

#include <array>
#include <cassert>

#include "utils.h"

#ifndef DSP_BRICKS_BLOCK_SIZE
#define DSP_BRICKS_BLOCK_SIZE   32
#endif

namespace bricks {

constexpr size_t PROC_BLOCK_SIZE = DSP_BRICKS_BLOCK_SIZE;

typedef std::array<float, PROC_BLOCK_SIZE> AudioBuffer;
typedef AudioBuffer* AudioPort;
typedef float* ControlPort;

/* Kind of a hack currently */
constexpr float SAMPLERATE = 44100;


/*
 * The basic building block of DspBricks.
 * Each derived brick module should implement a constructor that takes its
 * audio and control ports as inputs and maps them to internal placeholders.
 * The signal chain is then configured at construction and fixed for the
 * lifetime of the bricks.
 * The Brick is required to provide storage for output ports, floats for
 * control ports and AudioBuffers for audio ports
 */

class DspBrick
{
public:
    virtual ~DspBrick() = default;

    virtual ControlPort control_output(int /*n*/) {assert(false);}
    virtual AudioPort audio_output(int /*n*/) {assert(false);}

    virtual void render() = 0;

protected:
    DspBrick() = default;
};


typedef LinearInterpolator<PROC_BLOCK_SIZE> ControlSmootherLinear;
typedef OnePoleLag<PROC_BLOCK_SIZE> ControlSmootherLag;


} // namespace bricks

#endif //BRICKS_DSP_DSP_BRICK_H
