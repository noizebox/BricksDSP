#ifndef BRICKS_DSP_OSCILLATORS_H
#define BRICKS_DSP_OSCILLATORS_H

#include <cassert>

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
class OscillatorBrick : public DspBrickImpl<os_factor, 1, 0, 0, 1>
{
public:
    void set_waveform(Waveform waveform)
    {
        _waveform = waveform;
    }

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / (samplerate * os_factor);
    }

    void reset() override
    {
        _phase = 0.0f;
        _pitch_lag.reset();
    }

    void render(float pitch, AudioOsBuffer<os_factor>& audio_out)
    {
        float base_freq = clamp(control_to_freq(pitch), 1, 20000);
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
                    phase += phase_inc * 2.0f;
                    if (phase > 0.5)
                    {
                        phase -= 1;
                        dir = dir * -1;
                    }
                    sample = phase * dir;
                }
                _tri_dir = dir;
            }
        }
        _phase = phase;
    }

private:
    LinearLag<os_factor> _pitch_lag;
    float                _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float                _phase{0};
    Waveform             _waveform{Waveform::SAW};
    int                  _tri_dir{1};
};

/* Naive saw/sq/tri oscillator with audio rate linear fm */
template <size_t os_factor = 1>
class FmOscillatorBrick : public DspBrickImpl<os_factor, 1, 0, 1, 1>
{
public:
    FmOscillatorBrick() = default;

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / (samplerate * os_factor);
    }

    void reset() override {_phase = 0.0f;}

    void render(float pitch, const AudioOsBuffer<os_factor>& fm_mod, AudioOsBuffer<os_factor>& audio_out)
    {
        float base_freq = clamp(control_to_freq(pitch), 1, 20000);
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
                phase += phase_inc * 2.0f;
                if (phase > 0.5)
                {
                    phase -= 1;
                    dir = dir * -1;
                }
                audio_out[i] = phase * dir;
            }
            _tri_dir = dir;
        }

        }
        _phase = phase;
    }

private:
    LinearLag<os_factor>  _pitch_lag;
    float                 _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float                 _phase{0};
    Waveform              _waveform{Waveform::SAW};
    int                   _tri_dir{1};
};

/* Better aa:ed oscillator with control rate pitch input */
template <typename blep_type = PolyBlepSpline, int os_factor = 1>
class BlepOscillatorBrick : public DspBrickImpl<os_factor, 1, 0, 0, 1>
{
public:
    void set_waveform(Waveform waveform)
    {
        _waveform = waveform;
    }

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / (samplerate * os_factor);
    }

    void reset() override
    {
        _phase = 0.0f;
        _pitch_lag.reset();
    }

    void render(float pitch, float pulsewidth, AudioOsBuffer<os_factor>& audio_out)
    {
        float base_freq = clamp(control_to_freq(pitch), 1, 20000);
        pulsewidth = clamp((pulsewidth + .5f), 0.001f, 0.999f);
        float phase_inc = base_freq * _samplerate_inv;
        float phase = _phase;

        switch (_waveform)
        {
            case Waveform::SAW:
            {
                for (auto& sample : audio_out)
                {
                    phase += phase_inc;
                    if (phase > 1.0)
                        phase -= 1;
                    sample = phase - _blep.blep(phase, phase_inc);
                }
                break;
            }
            case Waveform::PULSE:
            {
                for (auto& sample : audio_out)
                {
                    phase += phase_inc;
                    if (phase > 1.0)
                        phase -= 1;

                    float phase_2 = phase + pulsewidth;
                    if (phase_2 > 1.0)
                        phase_2 -= 1.0;

                    sample = phase - _blep.blep(phase, phase_inc) - phase_2 + _blep.blep(phase_2, phase_inc);
                }
                break;
            }
            case Waveform::SINE:
            case Waveform::TRIANGLE:
            {
                    break;
            }
        }
        _phase = phase;
    }

private:
    LinearLag<os_factor> _pitch_lag;
    float                _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float                _phase{0};
    Waveform             _waveform{Waveform::SAW};
    int                  _tri_dir{1};
    blep_type            _blep;
};

static constexpr int WT_OCTAVES = 10;

class WaveTable
{
public:
    WaveTable() = default;
    struct Slot
    {
        const int size;
        const float* table;
    };
    Slot table_at(int octave) const
    {
        assert(octave >= 0 && octave < WT_OCTAVES);
        return {_sizes[octave], _tables[octave]};
    }

private:
    std::array<int, WT_OCTAVES>            _sizes;
    std::array<const float*, WT_OCTAVES>   _tables;
    std::unique_ptr<float[]>            _data;

    friend WaveTable create_wavetable(Waveform, const std::array<int, WT_OCTAVES>&);
    friend WaveTable* create_default_wavetable();
};

WaveTable create_wavetable(Waveform, int initial_table_length = 8192, int shortest_table_length = 16);
WaveTable create_wavetable(Waveform waveform, const std::array<int, WT_OCTAVES>& table_sizes);

WaveTable* create_default_wavetable();

/* Wavetable based saw/sq/tri oscillator with control rate pitch input */
template <typename Interpolator = LinearInterpolation<float>, size_t os_factor = 1>
class WtOscillatorBrick : public DspBrickImpl<os_factor, 1, 0, 0, 1>
{
public:
    WtOscillatorBrick()
    {
        _table = create_default_wavetable();
    }

    void set_waveform(Waveform waveform) {_waveform = waveform;}

    void set_wavetable(const WaveTable* table)
    {
        _table = table;
    }

    void set_samplerate(float samplerate) override
    {
        _samplerate = samplerate * os_factor;
        _samplerate_inv = 1.0f / _samplerate;
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
        auto slot = _table->table_at(oct);

        float table_len = slot.size;
        const float* table = slot.table;

        assert(*table == 0.0f);

        for (int i = 0; i < PROC_BLOCK_SIZE * os_factor; ++i)
        {
            phase += phase_inc;
            if (phase > 1.0f)
                phase -= 1.0f;
            float pos = phase * table_len;
            float sample = _interpolator.interpolate(pos, table);
            audio_out[i] = sample;
        }
        _phase = phase;
    }

private:
    float               _samplerate{DEFAULT_SAMPLERATE};
    float               _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    float               _phase{0};
    Waveform            _waveform{Waveform::SAW};
    Interpolator         _interpolator;
    const WaveTable*    _table;
};

/* Noise generator with 3 levels of lp filtering */
template <size_t os_factor = 1>
class NoiseGeneratorBrick : public DspBrickImpl<os_factor, 0, 0, 0, 1>
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

        _pink_coeff_a0 = std::exp(-2.0f * M_PI * PINK_CUTOFF_FREQ / (samplerate * os_factor));
        _brown_coeff_a0 = std::exp(-2.0f * M_PI * BROWN_CUTOFF_FREQ / (samplerate * os_factor));
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
    float               _state{0.0f};
    NoiseType           _waveform{NoiseType::WHITE};
    RandomDevice        _rand_device;
};

/* Crushed velvet noise, i.e. noise with only -1 and 1 amplitude */
template <size_t os_factor = 1>
class VelvetNoiseBrick : public DspBrickImpl<os_factor, 2, 0, 0, 1>
{
public:
    VelvetNoiseBrick() = default;

    void render(float density, float polarity, AudioOsBuffer<os_factor>& audio_out)
    {
        constexpr float MAX_DENSITY = 0.5f;
        constexpr float MAX_POLARITY = 0.5f;

        int density_samples = static_cast<int>(density * density * MAX_DENSITY * RandomDevice::FLOAT_MAX - RandomDevice::FLOAT_MIN);
        int polarity_samples = static_cast<int>(MAX_POLARITY * (polarity + 1.0f) * RandomDevice::FLOAT_MAX);

        for (auto& s : audio_out)
        {
            if (_rand_device.get() < density_samples)
            {
                s = _rand_device.get() < polarity_samples ? 1.0f : -1.0;
            }
            else
            {
                s = 0.0f;
            }
        }
    }

private:
    RandomDevice _rand_device;
};

}// namespace bricks

#endif //BRICKS_DSP_OSCILLATORS_H
