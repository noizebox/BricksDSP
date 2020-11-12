#include <cmath>

#include "oscillator_bricks.h"
#include "data/wavetables_x4.h"

namespace bricks {

void OscillatorBrick::render()
{
    float base_freq = control_to_freq(_ctrl_value(ControlInput::PITCH));
    float phase_inc = base_freq / _sample_rate();
    float phase = _phase;
    AudioBuffer& audio_out = _output_buffer(AudioOutput::OSC_OUT);

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

void FmOscillatorBrick::render()
{
    float base_freq = control_to_freq(_ctrl_value(ControlInput::PITCH));
    //_pitch_lag.set(_pitch_port.value());
    float phase_inc = base_freq / _sample_rate();
    float phase = _phase;
    const AudioBuffer& fm_mod = _input_buffer(AudioInput::LIN_FM);
    AudioBuffer& audio_out = _output_buffer(AudioOutput::OSC_OUT);

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

void WtOscillatorBrick::render()
{
    /* If the samplerate it less than 80kHz, use the wavetables 1 octave above
     * which has less harmonics to make sure they dont alias when interpolated */
    float sr = _sample_rate();
    int wt_shift = sr > 80000? 0 : sr > 40000? 1 : 2;
    float pitch = _ctrl_value(ControlInput::PITCH);
    float base_freq = control_to_freq(pitch);
    float phase_inc = base_freq / sr;
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

    AudioBuffer& audio_out = _output_buffer(AudioOutput::OSC_OUT);
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

constexpr float PINK_CUTOFF_FREQ = 100;
constexpr float PINK_GAIN_CORR = 4.0f;
constexpr float BROWN_CUTOFF_FREQ = 0.03;
constexpr float BROWN_GAIN_CORR = 50.0f;

void NoiseGeneratorBrick::render()
{
    AudioBuffer& audio_out = _output_buffer(AudioOutput::NOISE_OUT);

    for (auto& s : audio_out)
    {
        s = _rand_device.get_norm();
    }
    if (_waveform == Waveform::PINK)
    {
        float hist = audio_out[PROC_BLOCK_SIZE - 1];
        for (auto& s : audio_out)
        {
            s *= PINK_GAIN_CORR;
            s = (1.0f - _pink_coeff_a0) * s + _pink_coeff_a0 * hist;
            hist = s;
        }
    }
    if (_waveform == Waveform::BROWN)
    {
        float hist = audio_out[PROC_BLOCK_SIZE - 1];
        for (auto& s : audio_out)
        {
            s *= BROWN_GAIN_CORR;
            s = (1.0f - _brown_coeff_a0) * s + _brown_coeff_a0 * hist;
            hist = s;
        }
    }
}

void NoiseGeneratorBrick::set_samplerate(float samplerate)
{
    _pink_coeff_a0 = std::exp(-2.0f * M_PI * PINK_CUTOFF_FREQ / samplerate);
    _brown_coeff_a0 = std::exp(-2.0f * M_PI * BROWN_CUTOFF_FREQ / samplerate);
    DspBrickImpl::set_samplerate(samplerate);
}

} // namespace bricks