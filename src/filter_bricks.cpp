#include <algorithm>
#include <cmath>

#include "filter_bricks.h"

namespace bricks {

void SVFFilterBrick::render()
{
    const auto& audio_in = _input_buffer(0);
    auto& lowpass_out = _output_buffer(AudioOutput::LOWPASS);
    auto& bandpass_out = _output_buffer(AudioOutput::BANDPASS);
    auto& highpass_out = _output_buffer(AudioOutput::HIGHPASS);

    float freq = 20 * powf(2.0f, _ctrl_value(ControlInput::CUTOFF) * 10.0f);
    freq = std::clamp(freq, 5.0f, 19000.0f);
    float k = 2 - 2 * _ctrl_value(ControlInput::RESONANCE);
    _g_lag.set(std::tan(static_cast<float>(M_PI) * freq * _samplerate_inv));
    auto reg = _reg;
    auto g_lag = _g_lag;
    for (unsigned int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        float g = g_lag.get();
        float a1 = 1 / (1 + g * (g + k));
        float a2 = g * a1;
        float a3 = g * a2;
        float v3 = audio_in[i] - reg[1];
        float v1 = a1 * reg[0] + a2 * v3;
        float v2 = reg[1] + a2 * reg[0] + a3 * v3;
        reg[0] = 2.0f * v1 - reg[0];
        reg[1] = 2.0f * v2 - reg[1];

        lowpass_out[i] = v2;
        bandpass_out[i] = v1;
        highpass_out[i] = audio_in[i] - k * v1 - v2;
    }
    _reg = reg;
    _g_lag = g_lag;
}

void FixedFilterBrick::set_lowpass(float freq, float q, bool clear)
{
    _coeff = calc_lowpass(freq, q, samplerate());
    if (clear)
    {
        _reg = {0, 0};
    }
}

void FixedFilterBrick::set_highpass(float freq, float q, bool clear)
{
    _coeff = calc_highpass(freq, q, samplerate());
    if (clear)
    {
        reset();
    }
}

void FixedFilterBrick::set_bandpass(float freq, float q, bool clear)
{
    _coeff = calc_bandpass(freq, q, samplerate());
    if (clear)
    {
        reset();
    }
}

void FixedFilterBrick::set_peaking(float freq, float gain, float q, bool clear)
{
    _coeff = calc_peaking(freq, gain, q, samplerate());
    if (clear)
    {
        reset();
    }
}

void FixedFilterBrick::set_allpass(float freq, float q, bool clear)
{
    _coeff = calc_allpass(freq, q, samplerate());
    if (clear)
    {
        reset();
    }
}

void FixedFilterBrick::set_lowshelf(float freq, float gain, float q, bool clear)
{
    _coeff = calc_lowshelf(freq, gain, q, samplerate());
    if (clear)
    {
        reset();
    }
}

void FixedFilterBrick::set_highshelf(float freq, float gain, float q, bool clear)
{
    _coeff = calc_highshelf(freq, gain, q, samplerate());
    if (clear)
    {
        reset();
    }
}

void FixedFilterBrick::render()
{
    const AudioBuffer& audio_in = _input_buffer(0);
    AudioBuffer& audio_out = _output_buffer(AudioOutput::FILTER_OUT);
    render_df2_biquad(audio_in, audio_out, _coeff, _reg);
}

/* tanh(x)/x approximation, flatline at very high inputs
 * so might not be safe for very large feedback gains
 * [limit is 1/15 so very large means ~15 or +23dB] */
inline double tanhXdX(const double& x)
{
    double a = x*x;
    // IIRC I got this as Pade-approx for tanh(sqrt(x))/sqrt(x)
    return ((a + 105)*a + 945) / ((15*a + 420)*a + 945);
}

void MystransLadderFilter::render()
{
    const auto& in = _input_buffer(0);
    auto& audio_out = _output_buffer(0);
    float freq = 20 * powf(2.0f, _ctrl_value(ControlInput::CUTOFF) * 10.0f);
    freq = clamp(freq, 20.0f, 22000.0f);
    _freq_lag.set(std::tan(static_cast<float>(M_PI) * freq * _samplerate_inv));
    double r = (40.0/9.0) * _ctrl_value(ControlInput::RESONANCE);

    auto s = _states;
    auto zi = _zi;
    auto freq_lag = _freq_lag;

    for(int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        double f = freq_lag.get();
        // input with half delay, for non-linearities
        double ih = 0.5 * (in[i] + zi);
        zi = in[i];

        // evaluate the non-linear gains
        double t0 = tanhXdX(ih - r * s[3]);
        double t1 = tanhXdX(s[0]);
        double t2 = tanhXdX(s[1]);
        double t3 = tanhXdX(s[2]);
        double t4 = tanhXdX(s[3]);

        // g# the denominators for solutions of individual stages
        double g0 = 1 / (1 + f * t1), g1 = 1 / (1 + f * t2);
        double g2 = 1 / (1 + f * t3), g3 = 1 / (1 + f * t4);

        // f# are just factored out of the feedback solution
        double f3 = f * t3 * g3;
        double f2 = f * t2 * g2 * f3;
        double f1 = f * t1 * g1 * f2;
        double f0 = f * t0 * g0 * f1;

        // solve feedback
        double y3 = (g3 * s[3] + f3 * g2 * s[2] + f2 * g1 * s[1] + f1 * g0 * s[0] + f0 * in[i]) / (1 + r * f0);

        // then solve the remaining outputs (with the non-linear gains here)
        double xx = t0 * (in[i] - r * y3);
        double y0 = t1 * g0 * (s[0] + f * xx);
        double y1 = t2 * g1 * (s[1] + f * y0);
        double y2 = t3 * g2 * (s[2] + f * y1);

        // update state
        s[0] += 2 * f * (xx - y0);
        s[1] += 2 * f * (y0 - y1);
        s[2] += 2 * f * (y1 - y2);
        s[3] += 2 * f * (y2 - t4 * y3);

        audio_out[i] = y3;
    }
    _zi = zi;
    _states = s;
    _freq_lag = freq_lag;
}

} // namepspace bricks