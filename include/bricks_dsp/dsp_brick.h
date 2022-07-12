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

typedef AlignedArray<float, PROC_BLOCK_SIZE> AudioBuffer;
typedef LinearInterpolator<PROC_BLOCK_SIZE> ControlSmootherLinear;
typedef OnePoleLag<PROC_BLOCK_SIZE> ControlSmootherLag;

/*
 * The basic building block of DspBricks.
 * Each derived brick module should implement a default constructor as well
 * as a constructor that takes its audio and control inputs as arguments and
 * sets them as inputs.
 */
class DspBrick
{
public:
    enum DefaultAudioInput
    {
        DEFAULT_INPUT = 0
    };

    virtual ~DspBrick() = default;

    virtual void render() = 0;

    virtual void set_samplerate(float samplerate) {};

    virtual void reset() {};

    virtual int n_control_inputs() const = 0;

    virtual int n_control_outputs() const = 0;

    virtual int n_audio_inputs() const = 0;

    virtual int n_audio_outputs() const = 0;

    virtual void set_control_input(int input_no, const float* input) = 0;

    virtual void set_audio_input(int input_no, const AudioBuffer* input) = 0;

#ifndef BRICKS_DSP_INTERNAL_BUFFERS
    virtual void set_audio_output(int output_no, AudioBuffer* output) = 0;
#endif

protected:
    DspBrick() = default;
};

/* Implementation class that provides boilerplate code for setting up and storing
 * control and audio in/outputs. Templated so input and output pointers and objects
 * can be stored directly inside the object without extra redirections through
 * heap allocations */
template <int ctrl_ins, int ctrl_outs, int audio_ins, int audio_outs>
class DspBrickImpl : public DspBrick
{
public:
    int n_control_inputs() const final {return ctrl_ins;}

    int n_control_outputs() const final {return ctrl_outs;}

    int n_audio_inputs() const final {return audio_ins;}

    int n_audio_outputs() const final {return audio_outs;}

    void set_control_input(int input_no, const float* input) final
    {
        assert(input_no <= ctrl_ins);
        _ctrl_ins[input_no] = input;
    }

    void set_audio_input(int input_no, const AudioBuffer* input) final
    {
        assert(input_no <= audio_ins);
        _audio_ins[input_no] = input;
    }
#ifndef BRICKS_DSP_INTERNAL_BUFFERS
    void set_audio_output(int output_no, AudioBuffer* output) final
    {
        assert(output_no <= audio_outs);
        _audio_outs[output_no] = output;
    }

#endif

    const float* control_output(int output_no)
    {
        assert(output_no <= ctrl_outs);
        return &_ctrl_outs[output_no];
    }

    const AudioBuffer* audio_output(int output_no)
    {
        assert(output_no <= audio_outs);
#ifdef BRICKS_DSP_INTERNAL_BUFFERS
        return &_audio_outs[output_no];
#else
        return _audio_outs[output_no];
#endif
    }
protected:

    /* Get the value of a control input */
    float _ctrl_value(int input_no) const
    {
        assert(input_no <= ctrl_ins);
        return *_ctrl_ins[input_no];
    }

    /* Output a value to a control output */
    void _set_ctrl_value(int output_no, float value)
    {
        assert(output_no <= ctrl_outs);
        _ctrl_outs[output_no] = value;
    }

    /* Get an input buffer for reading audio data */
    const AudioBuffer& _input_buffer(int input_no) const
    {
        assert(input_no <= audio_ins);
        return *_audio_ins[input_no];
    }

    /* Get an output buffer for writing audio data */
    AudioBuffer& _output_buffer(int output_no)
    {
        assert(output_no <= audio_outs);
#ifdef BRICKS_DSP_INTERNAL_BUFFERS
        return _audio_outs[output_no];
#else
        assert(_audio_outs[output_no]);
        return *_audio_outs[output_no];
#endif
    }

private:
    std::array<const float*, ctrl_ins>          _ctrl_ins;
    std::array<float, ctrl_outs>                _ctrl_outs;
    std::array<const AudioBuffer*, audio_ins>   _audio_ins;
#ifdef BRICKS_DSP_INTERNAL_BUFFERS
    std::array<AudioBuffer, audio_outs>         _audio_outs;
#else
    std::array<AudioBuffer*, audio_outs>        _audio_outs;
#endif
};


} // namespace bricks

#endif //BRICKS_DSP_DSP_BRICK_H
