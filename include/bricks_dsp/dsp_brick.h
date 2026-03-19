#ifndef BRICKS_DSP_DSP_BRICK_H
#define BRICKS_DSP_DSP_BRICK_H

#include <array>
#include <cassert>

#include "utils.h"
#include "aligned_array.h"

#ifndef DSP_BRICKS_BLOCK_SIZE
#define DSP_BRICKS_BLOCK_SIZE 32
#endif

namespace bricks {

constexpr int PROC_BLOCK_SIZE = DSP_BRICKS_BLOCK_SIZE;

constexpr float DEFAULT_SAMPLERATE = 44100;

template <int os_factor = 1>
using AudioOsBuffer = AlignedArray<float, PROC_BLOCK_SIZE * os_factor>;
using AudioBuffer = AlignedArray<float, PROC_BLOCK_SIZE>;

template <int os_factor = 1>
using LinearLag = LinearInterpolator<PROC_BLOCK_SIZE * os_factor>;

typedef LinearInterpolator<PROC_BLOCK_SIZE> ControlSmootherLinear;
typedef OnePoleLag<PROC_BLOCK_SIZE> ControlSmootherLag;

/*
 * The basic building block of DspBricks.
 */
class DspBrick
{
public:
    virtual ~DspBrick() = default;

    virtual void set_samplerate(float samplerate) {};

    virtual void reset() {};
};

/* Implementation class that concrete implementations should inherit from */
template <int os_factor, int ctrl_ins, int ctrl_outs, int audio_ins, int audio_outs>
class DspBrickImpl : public DspBrick
{
public:
    ~DspBrickImpl() override = default;

    //virtual void render() = 0;
    /*
     * Derived classed should implement a render callback like this for brick where inputs and outputs are well defined.
     *
     * void render(float ctrl_1, float ctrl_2, float& ctrl_out, const AudioBuffer<os_factor>* input, AudioOsBuffer<os_factor>* output)
     *
     * For brick where the number of ctrl and audio inputs and/or outputs are templated, the signature should like below
     * for the templated arguments.
     *
     * void render(const std::array<float, ctrl_ins>& control_inputs,
                   const std::array<float*, ctrl_outs>& control_outputs,
                   const std::array<const AudioOsBuffer<os_factor>*, audio_ins>& audio_inputs,
                   const std::array<AudioOsBuffer<os_factor>*, audio_outs>& audio_outputs);
    */

protected:
    static constexpr size_t block_size = os_factor * PROC_BLOCK_SIZE;
};

} // namespace bricks

#endif //BRICKS_DSP_DSP_BRICK_H
