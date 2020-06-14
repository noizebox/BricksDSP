#include <algorithm>
#include <cmath>

#include "filter_bricks.h"

namespace bricks {

enum COEFF
{
    A1 = 0,
    A2,
    B0,
    B1,
    B2,
};
enum REGISTERS
{
    Z1 = 0,
    Z2,
};

using Coefficients = std::array<float, 5>;

/* Coefficient generation from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt */
inline Coefficients calc_lowpass(float freq, float q, float samplerate)
{
    Coefficients coeff;
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / q;
    float norm = 1.0f / (1.0f + alpha);
    float b0 = (1.0f - w0_cos) / 2.0f * norm;

    coeff[A1] = -2.0f * w0_cos * norm;
    coeff[A2] = (1 - alpha) * norm;
    coeff[B0] = b0;
    coeff[B1] = (1 - w0_cos) * norm;
    coeff[B2] = b0;
    return coeff;
};

inline Coefficients calc_highpass(float freq, float q, float samplerate)
{
    Coefficients coeff;
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / q;
    float norm = 1.0f / (1.0f + alpha);
    float b0 = (1.0f + w0_cos) / 2.0f * norm;

    coeff[A1] = -2.0f * w0_cos * norm;
    coeff[A2] = (1 - alpha) * norm;
    coeff[B0] = b0;
    coeff[B1] = -(1 + w0_cos) * norm;
    coeff[B2] = b0;
    return coeff;
};

inline Coefficients calc_bandpass(float freq, float q, float samplerate)
{
    Coefficients coeff;
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / q;
    float norm = 1.0f / (1.0f + alpha);
    float b0 = alpha * norm;

    coeff[A1] = -2.0f * w0_cos * norm;
    coeff[A2] = (1 - alpha) * norm;
    coeff[B0] = b0;
    coeff[B1] = 0.0f;
    coeff[B2] = -b0;
    return coeff;
};

inline Coefficients calc_allpass(float freq, float q, float samplerate)
{
    Coefficients coeff;
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / q;
    float norm = 1.0f / (1.0f + alpha);
    float a1 =  -2.0f * w0_cos * norm;
    float b0 = (1.0f - alpha) * norm;

    coeff[A1] = a1;
    coeff[A2] = b0;
    coeff[B0] = b0;
    coeff[B1] = a1;
    coeff[B2] = 1.0f;
    return coeff;
};

/* freq in Hz, gain in dB */
inline Coefficients calc_peaking(float freq, float gain, float q,  float samplerate)
{
    Coefficients coeff;
    float A = powf(10.0f, gain / 40.0f) ;
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / q;
    float norm = 1.0f / (1.0f + alpha / A);
    float b0 = (1.0f + alpha * A) * norm;

    coeff[A1] = -2.0f * w0_cos * norm;
    coeff[A2] = (1 - alpha / A) * norm;
    coeff[B0] = b0;
    coeff[B1] = -2.0f * w0_cos * norm;
    coeff[B2] = (1.0f - alpha * A) * norm;
    return coeff;
};

inline Coefficients calc_lowshelf(float freq, float gain, float slope, float samplerate)
{
    Coefficients coeff;
    float A = powf(10.0f, gain / 40.0f);
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float A2_sqrt_alpha = 2.0f * w0_sin * std::sqrt((A * A + 1.0f) * (1.0f / slope - 1.0f) + 2.0f * A);
    float A_inc_cos_w0 = (A + 1.0f) * w0_cos;
    float A_dec_cos_w0 = (A - 1.0f) * w0_cos;
    float norm = 1.0f / ((A + 1.0f) + A_dec_cos_w0 + A2_sqrt_alpha);

    coeff[A1] = -2.0f * (A - 1.0f + A_inc_cos_w0) * norm;
    coeff[A2] = (A + 1.0f + A_dec_cos_w0 - A2_sqrt_alpha) * norm;
    coeff[B0] = A * (A + 1.0f - A_dec_cos_w0 + A2_sqrt_alpha) * norm;
    coeff[B1] = 2.0f * A * (A - 1.0f - A_inc_cos_w0) * norm;
    coeff[B2] = A * (A + 1.0f - A_dec_cos_w0 - A2_sqrt_alpha) * norm;
    return coeff;
};

inline Coefficients calc_highshelf(float freq, float gain, float slope, float samplerate)
{
    Coefficients coeff;
    float A = powf(10.0f, gain / 40.0f);
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float A2_sqrt_alpha = 2.0f * w0_sin * std::sqrt((A * A + 1.0f) * (1.0f / slope - 1.0f) + 2.0f * A);
    float A_inc_cos_w0 = (A + 1.0f) * w0_cos;
    float A_dec_cos_w0 = (A - 1.0f) * w0_cos;
    float norm = 1.0f / ((A + 1.0f) - A_dec_cos_w0 + A2_sqrt_alpha);

    coeff[A1] = 2.0f * (A - 1.0f - A_inc_cos_w0) * norm;
    coeff[A2] = (A + 1.0f - A_dec_cos_w0 - A2_sqrt_alpha) * norm;
    coeff[B0] = A * (A + 1.0f + A_dec_cos_w0 + A2_sqrt_alpha) * norm;
    coeff[B1] = -2.0f * A * (A - 1.0f + A_inc_cos_w0) * norm;
    coeff[B2] = A * (A + 1.0f + A_dec_cos_w0 - A2_sqrt_alpha) * norm;
    return coeff;
};

void BiquadFilterBrick::render()
{
    float freq = 20 * powf(2.0f, _cutoff_ctrl.value() * 10.0f);
    //std::cout << "Frec: " << freq << ",.. " << _cutoff_ctrl.value() << std::endl;
    float res = _res_q_ctrl.value();
    float gain = _gain_ctrl.value() * 15.0f; // 15dB -/+ range for peaking and shelving modes
    Coefficients new_coeff;
    switch (_mode)
    {
        case Mode::LOWPASS:
            res =  0.6f + 5.0f * res;
            new_coeff = calc_lowpass(freq, res, _samplerate);
            break;

        case Mode::HIGHPASS:
            res =  0.6f + 5.0f * res;
            new_coeff = calc_highpass(freq, res, _samplerate);
            break;

        case Mode::BANDPASS:
            new_coeff = calc_bandpass(freq, res, _samplerate);
            break;

        case Mode::ALLPASS:
            new_coeff = calc_allpass(freq, res, _samplerate);
            break;

        case Mode::PEAKING:
            new_coeff = calc_peaking(freq, res, gain,_samplerate);
            break;

        case Mode::LOW_SHELF:
            new_coeff = calc_lowshelf(freq, res, gain,_samplerate);
            break;

        case Mode::HIGH_SHELF:
            new_coeff = calc_highshelf(freq, res, gain,_samplerate);
            break;

    }
    for (size_t i = 0; i < _coeff.size(); ++i)
    {
        _coeff[i].set(new_coeff[i]);
    }

    const AudioBuffer& in = _audio_in.buffer();
    auto reg = _reg;
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        Coefficients coeff;
        for (size_t c = 0; c < coeff.size(); ++c)
        {
            coeff[c] = _coeff[c].get();
        }
        /* Direct form 2 transposed */
        float out = in[i] * coeff[B0] + _reg[Z1];
        _reg[Z1] = in[i] * coeff[B1] +  _reg[Z2] - coeff[A1] * out;
        _reg[Z2] = in[i] * coeff[B2]- coeff[A2] * out;
        _audio_out[i] = out;
    }
    _reg = reg;
}

void SVFFilterBrick::render()
{
    const AudioBuffer& in = _audio_in.buffer();
    float freq = 20 * powf(2.0f, _cutoff_ctrl.value() * 10.0f);
    freq = std::clamp(freq, 20.0f, 18000.0f);
    float k = 2 - 2 * _res_ctrl.value();
    _g_lag.set(std::tan(static_cast<float>(M_PI) * freq / _samplerate));
    AudioBuffer g_lag = _g_lag.get_all();
    for (unsigned int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        float g = g_lag[i];
        float a1 = 1 / (1 + g * (g + k));
        float a2 = g * a1;
        float a3 = g * a2;
        float v3 = in[i] - _reg[1];
        float v1 = a1 * _reg[0] + a2 * v3;
        float v2 = _reg[1] + a2 * _reg[0] + a3 * v3;
        _reg[0] = 2.0f * v1 - _reg[0];
        _reg[1] = 2.0f * v2 - _reg[1];
        _lowpass_out[i] = v2;
        _bandpass_out[i] = v1;
        _highpass_out[i] = in[i] - k * v1 - v2;
    }
}

void FixedFilterBrick::render()
{
    const AudioBuffer& in = _audio_in.buffer();
    auto reg = _reg;
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        /* Direct form 2 transposed */
        float out = in[i] * _coeff[B0] + reg[Z1];
        reg[Z1] = in[i] * _coeff[B1] +  reg[Z2] - _coeff[A1] * out;
        reg[Z2] = in[i] * _coeff[B2]- _coeff[A2] * out;
        _audio_out[i] = out;
    }
    _reg = reg;
}

void FixedFilterBrick::set_lowpass(float freq, float q, bool clear)
{
    _coeff = calc_lowpass(freq, q, _samplerate);
    if (clear)
    {
        _reg = {0, 0};
    }
}

void FixedFilterBrick::set_highpass(float freq, float q, bool clear)
{
    _coeff = calc_highpass(freq, q, _samplerate);
    if (clear)
    {
        reset();
    }
}

void FixedFilterBrick::set_bandpass(float freq, float q, bool clear)
{
    _coeff = calc_bandpass(freq, q, _samplerate);
    if (clear)
    {
        reset();
    }
}

void FixedFilterBrick::set_peaking(float freq, float gain, float q, bool clear)
{
    _coeff = calc_peaking(freq, gain, q, _samplerate);
    if (clear)
    {
        reset();
    }
}

void FixedFilterBrick::set_allpass(float freq, float q, bool clear)
{
    _coeff = calc_allpass(freq, q, _samplerate);
    if (clear)
    {
        reset();
    }
}

void FixedFilterBrick::set_lowshelf(float freq, float gain, float q, bool clear)
{
    _coeff = calc_lowshelf(freq, gain, q, _samplerate);
    if (clear)
    {
        reset();
    }
}

void FixedFilterBrick::set_highshelf(float freq, float gain, float q, bool clear)
{
    _coeff = calc_highshelf(freq, gain, q, _samplerate);
    if (clear)
    {
        reset();
    }
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
    const AudioBuffer& in = _audio_in.buffer();
    float freq = 20 * powf(2.0f, _cutoff_ctrl.value() * 10.0f);
    freq = std::clamp(freq, 20.0f, 22000.0f);
    _freq_lag.set(std::tan(static_cast<float>(M_PI) * freq / _samplerate));
    double r = (40.0/9.0) * _res_ctrl.value();

    auto s = _states;
    auto zi = _zi;

    for(int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        double f = _freq_lag.get();
        // input with half delay, for non-linearities
        double ih = 0.5 * (in[i] + _zi); _zi = in[i];

        // evaluate the non-linear gains
        double t0 = tanhXdX(ih - r * s[3]);
        double t1 = tanhXdX(s[0]);
        double t2 = tanhXdX(s[1]);
        double t3 = tanhXdX(s[2]);
        double t4 = tanhXdX(s[3]);

        // g# the denominators for solutions of individual stages
        double g0 = 1 / (1 + f*t1), g1 = 1 / (1 + f*t2);
        double g2 = 1 / (1 + f*t3), g3 = 1 / (1 + f*t4);

        // f# are just factored out of the feedback solution
        double f3 = f*t3*g3, f2 = f*t2*g2*f3, f1 = f*t1*g1*f2, f0 = f*t0*g0*f1;

        // solve feedback
        double y3 = (g3*s[3] + f3*g2*s[2] + f2*g1*s[1] + f1*g0*s[0] + f0*in[i]) / (1 + r * f0);

        // then solve the remaining outputs (with the non-linear gains here)
        double xx = t0*(in[i] - r * y3);
        double y0 = t1*g0*(s[0] + f*xx);
        double y1 = t2*g1*(s[1] + f*y0);
        double y2 = t3*g2*(s[2] + f*y1);

        // update state
        s[0] += 2*f * (xx - y0);
        s[1] += 2*f * (y0 - y1);
        s[2] += 2*f * (y1 - y2);
        s[3] += 2*f * (y2 - t4*y3);

        _audio_out[i] = y3;
    }
    _zi = zi;
    _states = s;
}

} // namepspace bricks