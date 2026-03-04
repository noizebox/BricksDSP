#ifndef BRICKS_DSP_OSCILLATORS_H
#define BRICKS_DSP_OSCILLATORS_H

#include <cassert>

#include "data/wavetables_x4.h"
#include "dsp_brick.h"
#include "random_device.h"

namespace bricks {

enum class Waveform
{
    SAW,
    PULSE,
    TRIANGLE,
    SINE
};

/* Naive saw/sq/tri oscillator with control rate pitch input */
template <int os_factor = 1>
class OscillatorBrick : public DspBrickv2Impl<os_factor, 1, 0, 0, 1>
{
public:
    void set_waveform(Waveform waveform)
    {
        _waveform = waveform;
    }

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override
    {
        _phase = 0.0f;
        _pitch_lag.reset();
    }

    void render(float pitch, AudioOsBuffer<os_factor>& audio_out)
    {
        float base_freq = control_to_freq(pitch);
        float phase_inc = base_freq * _samplerate_inv;
        float phase = _phase;

        switch (_waveform)
        {
            case Waveform::SAW:
            {
                for (auto& sample : audio_out)
                {
                    phase += phase_inc;
                    if (phase > 0.5)
                        phase -= 1;
                    sample = phase;
                }
                break;
            }
            case Waveform::PULSE:
            {
                for (auto& sample : audio_out)
                {
                    phase += phase_inc;
                    if (phase > 0.5)
                        phase -= 1;
                    sample = std::signbit(phase) ? 0.5f : -0.5f;
                }
                break;
            }
            case Waveform::SINE:
            case Waveform::TRIANGLE:
            {
                int dir = _tri_dir;
                for (auto& sample : audio_out)
                {
                    phase += phase_inc * dir * 2.0f;
                    if (phase > 0.5)
                        dir = dir * -1;
                    sample = phase;
                }
                _tri_dir = dir;
            }
        }
        _phase = phase;
    }

private:
    ControlSmootherLag  _pitch_lag;
    float               _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float               _phase{0};
    Waveform            _waveform{Waveform::SAW};
    int                 _tri_dir{1};
};

/* Naive saw/sq/tri oscillator with audio rate linear fm */
template <size_t os_factor = 1>
class FmOscillatorBrick : public DspBrickv2Impl<os_factor, 1, 0, 1, 1>
{
public:
    FmOscillatorBrick() = default;

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override {_phase = 0.0f;}

    void render(float pitch, const AudioOsBuffer<os_factor>& fm_mod, AudioOsBuffer<os_factor>& audio_out)
    {
        float base_freq = control_to_freq(pitch);
        //_pitch_lag.set(_pitch_port.value());
        float phase_inc = base_freq * _samplerate_inv;
        float phase = _phase;

        switch (_waveform)
        {
        case Waveform::SAW:
        {
            for (int i = 0; i < audio_out.size(); ++i)
            {
                phase += phase_inc * (1.0f + fm_mod[i]);
                if (phase > 0.5)
                    phase -= 1;
                audio_out[i] = phase;
            }
        }
            break;
        case Waveform::PULSE:
        {
            for (int i = 0; i < audio_out.size(); ++i)
            {
                phase += phase_inc * (1.0f + fm_mod[i]);
                if (phase > 0.5)
                    phase -= 1;
                audio_out[i] = std::signbit(phase) ? 0.5f : -0.5f;
            }
        }
            break;
        case Waveform::SINE:
        case Waveform::TRIANGLE:
        {
            int dir = _tri_dir;
            for (int i = 0; i < audio_out.size(); ++i)
            {
                phase += phase_inc * 2 * dir * (1.0f + fm_mod[i]);
                if (phase > 0.5)
                    dir = dir * -1;
                audio_out[i] = phase;
            }
            _tri_dir = dir;
        }

        }
        _phase = phase;
    }

private:
    ControlSmootherLag  _pitch_lag;
    float               _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float               _phase{0};
    Waveform            _waveform{Waveform::SAW};
    int                 _tri_dir{1};
};


/* Wavetable based saw/sq/tri oscillator with control rate pitch input */
template <size_t os_factor = 1>
class WtOscillatorBrick : public DspBrickv2Impl<os_factor, 1, 0, 0, 1>
{
public:
    WtOscillatorBrick() = default;

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void set_samplerate(float samplerate) override
    {
        _samplerate = samplerate;
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override {_phase = 0.0f;}

    void render(float pitch, AudioOsBuffer<os_factor>& audio_out)
    {
        /* If the samplerate it less than 80kHz, use the wavetables 1 octave above
         * which has less harmonics to make sure they dont alias when interpolated */
        int wt_shift = _samplerate > 80000? 0 : _samplerate > 40000? 1 : 2;
        float base_freq = control_to_freq(pitch);
        float phase_inc = base_freq * _samplerate_inv;
        float phase = _phase;

        int oct = std::max(0, std::min(9, static_cast<int>(pitch * 10) + wt_shift));
        float table_len = wavetables::lengths[oct];
        const float* table{nullptr};

        switch (_waveform)
        {
        case Waveform::SAW:
            table = wavetables::saw + wavetables::offsets[oct];
            break;
        case Waveform::PULSE:
            table = wavetables::square + wavetables::offsets[oct];
            break;
        case Waveform::TRIANGLE:
            table = wavetables::triangle + wavetables::offsets[oct];
            break;
        case Waveform::SINE:
            table = wavetables::sine + wavetables::offsets[oct];
            break;
        }
        assert(*table == 0.0f);

        for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
        {
            phase += phase_inc;
            if (phase > 1.0f)
                phase -= 1.0f;
            float pos = phase * table_len;
            float sample = linear_int(pos, table);
            audio_out[i] = sample;
        }
        _phase = phase;
    }

private:
    float           _samplerate{DEFAULT_SAMPLERATE};
    float           _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float           _phase{0};
    Waveform        _waveform{Waveform::SAW};
};

/* Noise generator with 3 levels of lp filtering */
template <size_t os_factor = 1>
class NoiseGeneratorBrick : public DspBrickv2Impl<os_factor, 0, 0, 0, 1>
{
public:
    enum class NoiseType
    {
        WHITE,
        PINK,
        BROWN
    };

    NoiseGeneratorBrick() = default;

    void set_waveform(NoiseType waveform) {_waveform = waveform;}

    void set_samplerate(float samplerate) override
    {
        constexpr float PINK_CUTOFF_FREQ = 100;
        constexpr float BROWN_CUTOFF_FREQ = 0.03;

        _pink_coeff_a0 = std::exp(-2.0f * M_PI * PINK_CUTOFF_FREQ / samplerate);
        _brown_coeff_a0 = std::exp(-2.0f * M_PI * BROWN_CUTOFF_FREQ / samplerate);
    }

    void render(AudioOsBuffer<os_factor>& audio_out)
    {
        constexpr float PINK_GAIN_CORR = 4.0f;
        constexpr float BROWN_GAIN_CORR = 50.0f;

        for (auto& s : audio_out)
        {
            s = _rand_device.get_norm();
        }
        if (_waveform == NoiseType::PINK)
        {
            float hist = _state;
            for (auto& s : audio_out)
            {
                s *= PINK_GAIN_CORR;
                s = (1.0f - _pink_coeff_a0) * s + _pink_coeff_a0 * hist;
                hist = s;
            }
            _state = hist;
        }
        if (_waveform == NoiseType::BROWN)
        {
            float hist = _state;
            for (auto& s : audio_out)
            {
                s *= BROWN_GAIN_CORR;
                s = (1.0f - _brown_coeff_a0) * s + _brown_coeff_a0 * hist;
                hist = s;
            }
            _state = hist;
        }
    }

private:
    float               _pink_coeff_a0;
    float               _brown_coeff_a0;
    float               _state;
    NoiseType           _waveform{NoiseType::WHITE};
    RandomDevice        _rand_device;
};

}// namespace bricks

#endif //BRICKS_DSP_OSCILLATORS_H
