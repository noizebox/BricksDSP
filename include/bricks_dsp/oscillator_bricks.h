#ifndef BRICKS_DSP_OSCILLATORS_H
#define BRICKS_DSP_OSCILLATORS_H

#include <cassert>

#include "dsp_brick.h"
#include "random_device.h"

namespace bricks {

/* Naive saw/sq/tri oscillator with control rate pitch input */
class OscillatorBrick : public DspBrickImpl<1, 0, 0, 1>
{
public:
    enum class Waveform
    {
        SAW,
        PULSE,
        TRIANGLE,
        SINE
    };

    enum ControlInput
    {
        PITCH = 0
    };

    enum AudioOutput
    {
        OSC_OUT = 0
    };

    OscillatorBrick() = default;

    OscillatorBrick(const float* pitch)
    {
        set_control_input(ControlInput::PITCH, pitch);
    }

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override {_phase = 0.0f;}

    void render() override;

private:
    ControlSmootherLag  _pitch_lag;
    float               _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float               _phase{0};
    Waveform            _waveform{Waveform::SAW};
    int                 _tri_dir{1};
};

/* Naive saw/sq/tri oscillator with audio rate linear fm */
class FmOscillatorBrick : public DspBrickImpl<1, 0, 1, 1>
{
public:
    enum class Waveform
    {
        SAW,
        PULSE,
        TRIANGLE,
        SINE
    };

    enum ControlInput
    {
        PITCH = 0
    };

    enum AudioInput
    {
        LIN_FM = 0
    };

    enum AudioOutput
    {
        OSC_OUT = 0
    };

    FmOscillatorBrick() = default;

    FmOscillatorBrick(const float* pitch, const AudioBuffer* lin_fm)
    {
        set_control_input(ControlInput::PITCH, pitch);
        set_audio_input(AudioInput::LIN_FM, lin_fm);
    }

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override {_phase = 0.0f;}

    void render() override;

private:
    ControlSmootherLag  _pitch_lag;
    float               _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float               _phase{0};
    Waveform            _waveform{Waveform::SAW};
    int                 _tri_dir{1};
};


/* Wavetable based saw/sq/tri oscillator with control rate pitch input */
class WtOscillatorBrick : public DspBrickImpl<1, 0, 0, 1>
{
public:
    enum class Waveform
    {
        SAW,
        PULSE,
        TRIANGLE,
        SINE
    };

    enum ControlInput
    {
        PITCH = 0
    };

    enum AudioOutput
    {
        OSC_OUT = 0
    };

    WtOscillatorBrick() = default;

    WtOscillatorBrick(const float* pitch)
    {
        set_control_input(ControlInput::PITCH, pitch);
    }

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void set_samplerate(float samplerate) override
    {
        _samplerate = samplerate;
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override {_phase = 0.0f;}

    void render() override;

private:
    float           _samplerate{DEFAULT_SAMPLERATE};
    float           _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float           _phase{0};
    Waveform        _waveform{Waveform::SAW};
};

/* Noise generator with 3 levels of lp filtering */
class NoiseGeneratorBrick : public DspBrickImpl<0, 0, 0, 1>
{
public:
    enum class Waveform
    {
        WHITE,
        PINK,
        BROWN
    };

    enum AudioOutput
    {
        NOISE_OUT = 0,
    };

    NoiseGeneratorBrick() = default;

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void set_samplerate(float samplerate) override;

    void render() override;

private:
    float               _pink_coeff_a0;
    float               _brown_coeff_a0;
    Waveform            _waveform{Waveform::WHITE};
    RandomDevice        _rand_device;
};

}// namespace bricks

#endif //BRICKS_DSP_OSCILLATORS_H
