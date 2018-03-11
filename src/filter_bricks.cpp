#include <cmath>

#include <iostream>
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


void BiquadFilterBrick::_calc_coefficients()
{
    /* Lowpass from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt */
    float freq = 20 * powf(2.0f, _cutoff_ctrl.value() * 10.0f);
    float res =  0.6f + 5.0f *_res_ctrl.value();
    float w0 = 2.0f * M_PI * freq / _samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / res;
    float norm = 1.0f / (1.0f + alpha);
    float b0 = (1.0f - w0_cos) / 2.0f * norm;

    _coeff[A1].set(-2.0f * w0_cos * norm);
    _coeff[A2].set((1 - alpha) * norm);
    _coeff[B0].set(b0);
    _coeff[B1].set((1 - w0_cos) * norm);
    _coeff[B2].set(b0);
}


void BiquadFilterBrick::render()
{
    _calc_coefficients();
    std::array<float, 2> in = _in;
    std::array<float, 2> out = _out;

    for (unsigned int s = 0; s < PROC_BLOCK_SIZE; ++s)
    {
        std::array<float, 5> coeff;
        for (unsigned int i = 0; i <= coeff.size(); ++i)
        {
            coeff[i] = _coeff[i].get();
        }
        float x = _audio_in.buffer()[s];
        float y = coeff[B0] * x + coeff[B1] * in[0] + coeff[B2]*in[1] - coeff[A1] * out[0] - coeff[A2] *out[1];

        in[1] = in[0];
        in[0] = x;
        out[1] = out[0];
        out[0] = y;
        _audio_out[s] = y;
    }
    _in = in;
    _out = out;
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

void FixedFilterBrick::set_lowpass(float cutoff, float q)
{
    /* Lowpass from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt */
    float res =  0.6f + 5.0f * q;
    float w0 = 2.0f * M_PI * cutoff / _samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / res;
    float norm = 1.0f / (1.0f + alpha);
    float b0 = (1.0f - w0_cos) / 2.0f * norm;

    _coeff[A1] = -2.0f * w0_cos * norm;
    _coeff[A2] = (1 - alpha) * norm;
    _coeff[B0] = b0;
    _coeff[B1] = (1 - w0_cos) * norm;
    _coeff[B2] = b0;

}
} // namepspace bricks