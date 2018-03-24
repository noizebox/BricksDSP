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

constexpr float DEFAULT_SAMPLERATE = 44100;

typedef std::array<float, PROC_BLOCK_SIZE> AudioBuffer;

/* Thin wrappers around control and audio ports */
class ControlPort
{
public:
    ControlPort() = delete;

    ControlPort(const float& data) : _data(&data) {}

    float value() const {return *_data;}

private:
    const float* _data;
};

class AudioPort
{
public:
    AudioPort() = delete;

    AudioPort(const AudioBuffer& data) : _data(data) {}

    const AudioBuffer& buffer() const {return _data;}

private:
    const AudioBuffer& _data;
};

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

    virtual const float& control_output(int /*n*/) {assert(false);}

    virtual const AudioBuffer& audio_output(int /*n*/) {assert(false);}

    virtual void render() = 0;

    virtual void set_samplerate(float samplerate) {_samplerate = samplerate;}

protected:
    DspBrick() = default;
    float _samplerate{DEFAULT_SAMPLERATE};
};


typedef LinearInterpolator<PROC_BLOCK_SIZE> ControlSmootherLinear;
typedef OnePoleLag<PROC_BLOCK_SIZE> ControlSmootherLag;


} // namespace bricks

#endif //BRICKS_DSP_DSP_BRICK_H
