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

/* Coefficient generation from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt */
inline std::array<float, 5> calc_lowpass(float freq, float q, float samplerate)
{
    std::array<float, 5> coeff;
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

inline std::array<float, 5> calc_highpass(float freq, float q, float samplerate)
{
    std::array<float, 5> coeff;
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

inline std::array<float, 5> calc_bandpass(float freq, float q, float samplerate)
{
    std::array<float, 5> coeff;
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

/* freq in Hz, gain in dB */
inline std::array<float, 5> calc_peaking(float freq, float gain, float q,  float samplerate)
{
    std::array<float, 5> coeff;
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

inline std::array<float, 5> calc_allpass(float freq, float q, float samplerate)
{
    std::array<float, 5> coeff;
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / q;
    float norm = 1.0f / (1.0f + alpha);

    coeff[A1] = -2.0f * w0_cos * norm;
    coeff[A2] = (1 - alpha) * norm;
    coeff[B0] = (1 - alpha) * norm;
    coeff[B1] = -2.0f * w0_cos * norm;
    coeff[B2] = 1.0f;
    return coeff;
};

void BiquadFilterBrick::render()
{
    float freq = 20 * powf(2.0f, _cutoff_ctrl.value() * 10.0f);
    float res =  0.6f + 5.0f *_res_ctrl.value();
    auto updated_coeff = calc_lowpass(freq, res, _samplerate);
    for (size_t i = 0; i < _coeff.size(); ++i)
    {
        _coeff[i].set(updated_coeff[i]);
    }

    const AudioBuffer& in = _audio_in.buffer();
    for (unsigned int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        std::array<float, 5> coeff;
        for (size_t c = 0; c < coeff.size(); ++c)
        {
            coeff[c] = _coeff[c].get();
        }
        float w = in[i] - coeff[A1] * _reg[Z1] - coeff[A2] * _reg[Z2];
        _audio_out[i] = coeff[B1] * _reg[Z1] + coeff[B2] * _reg[Z2] + coeff[B0] * w;
        _reg[Z2] = _reg[Z1];
        _reg[Z1] = w;
    }
}

void SVFFilterBrick::render()
{
    /* From https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf */
    const AudioBuffer& in = _audio_in.buffer();
    float freq = 20 * powf(2.0f, _cutoff_ctrl.value() * 10.0f);
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
        _audio_out[i] = v2;
    }
}

void FixedFilterBrick::render()
{
    const AudioBuffer& in = _audio_in.buffer();
    for (unsigned int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        float w = in[i] - _coeff[A1] * _reg[Z1] - _coeff[A2] * _reg[Z2];
        _audio_out[i] = _coeff[B1] * _reg[Z1] + _coeff[B2] * _reg[Z2] + _coeff[B0] * w;
        _reg[Z2] = _reg[Z1];
        _reg[Z1] = w;
    }
}

void FixedFilterBrick::set_lowpass(float freq, float q)
{
    _coeff = calc_lowpass(freq, q, _samplerate);
    _reg = {0, 0};
}

void FixedFilterBrick::set_highpass(float freq, float q)
{
    _coeff = calc_highpass(freq, q, _samplerate);
    _reg = {0, 0};
}

void FixedFilterBrick::set_bandpass(float freq, float q)
{
    _coeff = calc_bandpass(freq, q, _samplerate);
    _reg = {0, 0};
}

void FixedFilterBrick::set_peaking(float freq, float gain, float q)
{
    _coeff = calc_peaking(freq, gain, q, _samplerate);
    _reg = {0, 0};
}

void FixedFilterBrick::set_allpass(float freq, float q)
{
    _coeff = calc_allpass(freq, q, _samplerate);
    _reg = {0, 0};
}


} // namepspace bricks