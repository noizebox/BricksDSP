#ifndef BRICKS_DSP_FILTER_BRICKS_H
#define BRICKS_DSP_FILTER_BRICKS_H

#include "dsp_brick.h"
namespace bricks {

#ifdef BRICKS_DSP_CONSTEXPR_MATH
constexpr float DEFAULT_Q = 1 / sqrtf(2.0f);
#else
constexpr float DEFAULT_Q = 1 / 1.42f;
#endif

template<typename FloatType>
struct BiquadCoefficients
{
    FloatType a1{0.2}; // Defaults are just there to avoid benchmarks to optimise away everything completely
    FloatType a2{0.3};
    FloatType b0{0.4};
    FloatType b1{0.5};
    FloatType b2{0.6};
};

template<typename FloatType>
struct BiquadRegisters
{
    FloatType z1;
    FloatType z2;
};

/* Direct form 2 transposed biquad calculation */
template <typename FloatType>
inline float render_biquad_sample(float in,
                                  const BiquadCoefficients<FloatType>& coeff,
                                  BiquadRegisters<FloatType>& reg)
{
    FloatType out = in * coeff.b0 + reg.z1;
    reg.z1 = in * coeff.b1 + reg.z2 - coeff.a1 * out;
    reg.z2 = in * coeff.b2 - coeff.a2 * out;
    return out;
}

template <typename FloatType, int BlockSize>
void render_df2_biquad(const AlignedArray<float, BlockSize>& in,
                       AlignedArray<float, BlockSize>& out,
                       const BiquadCoefficients<FloatType>& coeff,
                       BiquadRegisters<FloatType>& registers)
{
    auto reg = registers;
    for (int i = 0; i < in.size(); ++i)
    {
        out[i] = render_biquad_sample(in[i], coeff, reg);
    }
    registers = reg;
}

using Coefficients = BiquadCoefficients<float>;
using Registers = BiquadRegisters<float>;

/* Coefficient generation from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt */
template <typename  FloatType = float>
BiquadCoefficients<FloatType> calc_lowpass(float freq, float q, float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    FloatType w0 = 2.0f * static_cast<FloatType>(M_PI) * freq / samplerate;
    FloatType w0_cos = std::cos(w0);
    FloatType w0_sin = std::sin(w0);
    FloatType alpha = w0_sin / q;
    FloatType norm = 1.0f / (1.0f + alpha);
    FloatType b0 = (1.0f - w0_cos) / 2.0f * norm;

    coeff.a1 = -2.0f * w0_cos * norm;
    coeff.a2 = (1 - alpha) * norm;
    coeff.b0 = b0;
    coeff.b1 = (1 - w0_cos) * norm;
    coeff.b2 = b0;
    return coeff;
};

template <typename  FloatType = float>
BiquadCoefficients<FloatType> calc_highpass(float freq, float q, float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    FloatType w0 = 2.0f * static_cast<FloatType>(M_PI) * freq / samplerate;
    FloatType w0_cos = std::cos(w0);
    FloatType w0_sin = std::sin(w0);
    FloatType alpha = w0_sin / q;
    FloatType norm = 1.0f / (1.0f + alpha);
    FloatType b0 = (1.0f + w0_cos) / 2.0f * norm;

    coeff.a1 = -2.0f * w0_cos * norm;
    coeff.a2 = (1 - alpha) * norm;
    coeff.b0 = b0;
    coeff.b1 = -(1 + w0_cos) * norm;
    coeff.b2 = b0;
    return coeff;
};

template <typename  FloatType = float>
inline BiquadCoefficients<FloatType> calc_bandpass(float freq, float q, float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / q;
    float norm = 1.0f / (1.0f + alpha);
    float b0 = alpha * norm;

    coeff.a1 = -2.0f * w0_cos * norm;
    coeff.a2 = (1 - alpha) * norm;
    coeff.b0 = b0;
    coeff.b1 = 0.0f;
    coeff.b2 = -b0;
    return coeff;
};

template <typename  FloatType = float>
inline BiquadCoefficients<FloatType> calc_allpass(float freq, float q, float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / q;
    float norm = 1.0f / (1.0f + alpha);
    float a1 =  -2.0f * w0_cos * norm;
    float b0 = (1.0f - alpha) * norm;

    coeff.a1 = a1;
    coeff.a2 = b0;
    coeff.b0 = b0;
    coeff.b1 = a1;
    coeff.b2 = 1.0f;
    return coeff;
};

/* freq in Hz, gain in dB */
template <typename  FloatType = float>
inline BiquadCoefficients<FloatType> calc_peaking(float freq, float gain, float q,  float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    float A = powf(10.0f, gain / 40.0f) ;
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin / q;
    float norm = 1.0f / (1.0f + alpha / A);
    float b0 = (1.0f + alpha * A) * norm;

    coeff.a1 = -2.0f * w0_cos * norm;
    coeff.a2 = (1 - alpha / A) * norm;
    coeff.b0 = b0;
    coeff.b1 = -2.0f * w0_cos * norm;
    coeff.b2 = (1.0f - alpha * A) * norm;
    return coeff;
};

template <typename  FloatType = float>
inline BiquadCoefficients<FloatType> calc_lowshelf(float freq, float gain, float slope, float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    float A = powf(10.0f, gain / 40.0f);
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float A2_sqrt_alpha = 2.0f * w0_sin * std::sqrt((A * A + 1.0f) * (1.0f / slope - 1.0f) + 2.0f * A);
    float A_inc_cos_w0 = (A + 1.0f) * w0_cos;
    float A_dec_cos_w0 = (A - 1.0f) * w0_cos;
    float norm = 1.0f / ((A + 1.0f) + A_dec_cos_w0 + A2_sqrt_alpha);

    coeff.a1 = -2.0f * (A - 1.0f + A_inc_cos_w0) * norm;
    coeff.a2 = (A + 1.0f + A_dec_cos_w0 - A2_sqrt_alpha) * norm;
    coeff.b0 = A * (A + 1.0f - A_dec_cos_w0 + A2_sqrt_alpha) * norm;
    coeff.b1 = 2.0f * A * (A - 1.0f - A_inc_cos_w0) * norm;
    coeff.b2 = A * (A + 1.0f - A_dec_cos_w0 - A2_sqrt_alpha) * norm;
    return coeff;
};

template <typename  FloatType = float>
inline BiquadCoefficients<FloatType> calc_highshelf(float freq, float gain, float slope, float samplerate)
{
    BiquadCoefficients<FloatType> coeff;
    float A = powf(10.0f, gain / 40.0f);
    float w0 = 2.0f * static_cast<float>(M_PI) * freq / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float A2_sqrt_alpha = 2.0f * w0_sin * std::sqrt((A * A + 1.0f) * (1.0f / slope - 1.0f) + 2.0f * A);
    float A_inc_cos_w0 = (A + 1.0f) * w0_cos;
    float A_dec_cos_w0 = (A - 1.0f) * w0_cos;
    float norm = 1.0f / ((A + 1.0f) - A_dec_cos_w0 + A2_sqrt_alpha);

    coeff.a1 = 2.0f * (A - 1.0f - A_inc_cos_w0) * norm;
    coeff.a2 = (A + 1.0f - A_dec_cos_w0 - A2_sqrt_alpha) * norm;
    coeff.b0 = A * (A + 1.0f + A_dec_cos_w0 + A2_sqrt_alpha) * norm;
    coeff.b1 = -2.0f * A * (A - 1.0f + A_inc_cos_w0) * norm;
    coeff.b2 = A * (A + 1.0f + A_dec_cos_w0 - A2_sqrt_alpha) * norm;
    return coeff;
};

/* Standard Biquad with non-modulated filter parameters */
template <size_t os_factor = 1>
class FixedFilterBrick : public DspBrickv2Impl<os_factor, 0, 0, 1, 1>
{
public:
    FixedFilterBrick()
    {
        set_lowpass(500, 0.2, true); // Set sensible defaults mainly for benchmarks
    }

    void set_lowpass(float freq, float q, bool clear = true)
    {
        _coeff = calc_lowpass(freq, q, _samplerate);
        if (clear)
        {
            _reg = {0, 0};
        }
    }

    void set_highpass(float freq, float q, bool clear = true)
    {
        _coeff = calc_highpass(freq, q, _samplerate);
        if (clear)
        {
            reset();
        }
    }

    void set_bandpass(float freq, float q, bool clear = true)
    {
        _coeff = calc_bandpass(freq, q, _samplerate);
        if (clear)
        {
            reset();
        }
    }

    void set_peaking(float freq, float gain, float q, bool clear = true)
    {
        _coeff = calc_peaking(freq, gain, q, _samplerate);
        if (clear)
        {
            reset();
        }
    }

    void set_allpass(float freq, float q, bool clear = true)
    {
        _coeff = calc_allpass(freq, q, _samplerate);
        if (clear)
        {
            reset();
        }
    }

    void set_lowshelf(float freq, float gain, float q, bool clear = true)
    {
        _coeff = calc_lowshelf(freq, gain, q, _samplerate);
        if (clear)
        {
            reset();
        }
    }

    void set_highshelf(float freq, float gain, float q, bool clear = true)
    {
        _coeff = calc_highshelf(freq, gain, q, _samplerate);
        if (clear)
        {
            reset();
        }
    }


    void set_coeffs(const Coefficients& coeffs)
    {
        _coeff = coeffs;
    }

    void set_samplerate(float samplerate) override
    {
        _samplerate = samplerate;
    }

    void render(const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        render_df2_biquad(audio_in, audio_out, _coeff, _reg);
    }

    void reset() override
    {
        _reg = {0, 0};
    }

private:
    float           _samplerate{DEFAULT_SAMPLERATE};
    Coefficients    _coeff{0,0,0,0,0};
    Registers       _reg{0,0};
};

/* Fixed Biquad with non-modulated filter parameters and templated number of stages */
template<size_t stages, size_t os_factor = 1, typename FloatType = float>
class MultiStageFilterBrick : public DspBrickv2Impl<os_factor, 0, 0, 1, 1>
{
public:
    MultiStageFilterBrick() = default;

    void set_coeffs(const std::array<BiquadCoefficients<FloatType>, stages>& coeffs)
    {
        _coeff = coeffs;
    }

    void render(const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        auto regs = _reg;
        /* Can't partially specialise a member function, this is a workaround */
        if constexpr (stages % 2 == 0)
        {
            /* With an even number of stages, first render to the buffer, then ping pong
             * between audio_out and buffer to avoid an extra copy in the end */
            AudioBuffer buffer;
            render_df2_biquad<FloatType, os_factor * PROC_BLOCK_SIZE>(audio_in, buffer, _coeff[0], regs[0]);
            for (int i = 1; i < stages; ++i)
            {
                const AudioBuffer& in = i % 2 ? buffer : audio_out;
                AudioBuffer& out = i % 2 ? audio_out : buffer;
                render_df2_biquad<FloatType, os_factor * PROC_BLOCK_SIZE>(in, out, _coeff[i], regs[i]);
            }
        }
        else
        {
            /* With an odd number of stages, first render to audio_out, then ping-pong
             * between buffer and audio out */
            AudioBuffer buffer;
            render_df2_biquad<FloatType, os_factor * PROC_BLOCK_SIZE>(audio_in, audio_out, _coeff[0], regs[0]);
            for (int i = 1; i < stages; ++i)
            {
                const AudioBuffer& in = i % 2 ? audio_out: buffer;
                AudioBuffer& out = i % 2 ? buffer : audio_out;
                render_df2_biquad<FloatType, os_factor * PROC_BLOCK_SIZE>(in, out, _coeff[i], regs[i]);
            }
        }
        _reg = regs;
    }

    void reset() override
    {
        _reg.fill({0, 0});
    }

private:
    std::array<BiquadCoefficients<FloatType>, stages>   _coeff;
    std::array<BiquadRegisters<FloatType>, stages>      _reg;
};

/* Fixed filter with non-modulated filter parameters and stages calculated
 * in parallel, adds 1 sample delay per stage but allows for much more
 * cpu-efficient processing */
template<size_t stages, size_t os_factor = 1, typename FloatType = float>
class PipelinedFilterBrick : public DspBrickv2Impl<os_factor, 0, 0, 1, 1>
{
public:
    PipelinedFilterBrick() = default;

    void set_coeffs(const std::array<BiquadCoefficients<FloatType>, stages>& coeffs)
    {
        _coeff = coeffs;
    }

    void render(const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        auto pipeline = _pipeline;
        auto regs = _reg;

        for (int i = 0; i < PROC_BLOCK_SIZE * os_factor; ++i)
        {
            pipeline[0] = audio_in[i];
            for (int s = 0; s < stages; ++s)
            {
                pipeline[s] = render_biquad_sample<FloatType>(pipeline[s], _coeff[s], regs[s]);
            }
            audio_out[i] = pipeline[stages - 1];
            // advance buffer 1 sample
            for (int s = stages - 1; s > 0 ; --s)
            {
                pipeline[s] = pipeline[s - 1];
            }
        }
        _pipeline = pipeline;
        _reg = regs;
    }

    void reset() override
    {
        _reg.fill({0, 0});
        _pipeline.fill(0);
    }

private:
    static_assert(stages >= 2, "Needs at least 2 stages to be useful");
    std::array<BiquadCoefficients<FloatType>, stages>   _coeff;
    std::array<BiquadRegisters<FloatType>, stages>      _reg;
    std::array<FloatType, stages>                       _pipeline;
};


/* Fixed filter with templated number of parallel paths.
 * More efficient for multiple voices/channels */
template<int channel_count, size_t os_factor = 1, typename FloatType = float>
class ParallelFilterBrick : public DspBrickv2Impl<os_factor, 0, 0, channel_count, channel_count>
{
public:
    ParallelFilterBrick() = default;

    void set_coeffs(const BiquadCoefficients<FloatType>& coeffs)
    {
        _coeff = coeffs;
    }

    void render(const std::array<const AudioOsBuffer<os_factor>*, channel_count>& audio_ins,
                const std::array<AudioOsBuffer<os_factor>*, channel_count>& audio_outs)
    {
        auto regs = _reg;

        for (int s = 0; s < PROC_BLOCK_SIZE * os_factor; ++s)
        {
            for (int c = 0; c < channel_count; ++c)
            {
                audio_outs[c]->data()[s] = render_biquad_sample<FloatType>(audio_ins[c]->data()[s], _coeff, regs[c]);
            }
        }
        _reg = regs;
    }

    void reset() override
    {
        _reg.fill({0, 0});
    }

private:
    BiquadCoefficients<FloatType> _coeff;
    std::array<BiquadRegisters<FloatType>, channel_count> _reg;
};


/* State variable filter with multiple outs from Andrew Simper, Cytomic,
 * adapted from https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf */
template <size_t os_factor = 1>
class SVFFilterBrick : public DspBrickv2Impl<os_factor, 2, 0, 1, 3>
{
public:
    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override
    {
        _g_lag.reset();
        _reg = {0, 0};
    }

    void render(float cutoff, float resonance,
                const AudioOsBuffer<os_factor>& audio_in,
                AudioOsBuffer<os_factor>& lowpass_out,
                AudioOsBuffer<os_factor>& bandpass_out,
                AudioOsBuffer<os_factor>& highpass_out)
    {
        float freq = 20 * powf(2.0f, cutoff * 10.0f);
        freq = clamp(freq, 20.0f, 18000.0f);
        float k = 2 - 2 * resonance;
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

private:
    float                 _samplerate_inv {1.0f / DEFAULT_SAMPLERATE};
    std::array<float, 2>  _reg{0,0};
    ControlSmootherLinear _g_lag;
};

/* tanh(x)/x approximation, flatline at very high inputs
 * so might not be safe for very large feedback gains
 * [limit is 1/15 so very large means ~15 or +23dB] */
inline double tanhXdX(const double& x)
{
    double a = x*x;
    // IIRC I got this as Pade-approx for tanh(sqrt(x))/sqrt(x)
    return ((a + 105)*a + 945) / ((15*a + 420)*a + 945);
}

/* Topology-preserving (zero delay) ladder with non-linearities
 * Adapted from https://www.kvraudio.com/forum/viewtopic.php?t=349859
 * and Copyright 2012 Teemu Voipio (mystran @ kvr)  */
template <size_t os_factor = 1>
class MystransLadderFilter : public DspBrickv2Impl<os_factor, 2, 0, 1, 1>
{
public:
    void render(float cutoff, float resonance, const AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        float freq = 20 * powf(2.0f, cutoff * 10.0f);
        freq = clamp(freq, 20.0f, 22000.0f);
        _freq_lag.set(std::tan(static_cast<float>(M_PI) * freq * _samplerate_inv));
        double r = (40.0/9.0) * resonance;

        auto s = _states;
        auto zi = _zi;
        auto freq_lag = _freq_lag;

        for(int i = 0; i < PROC_BLOCK_SIZE * os_factor; ++i)
        {
            double f = freq_lag.get();
            // input with half delay, for non-linearities
            double ih = 0.5 * (audio_in[i] + zi);
            zi = audio_in[i];

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
            double y3 = (g3 * s[3] + f3 * g2 * s[2] + f2 * g1 * s[1] + f1 * g0 * s[0] + f0 * audio_in[i]) / (1 + r * f0);

            // then solve the remaining outputs (with the non-linear gains here)
            double xx = t0 * (audio_in[i] - r * y3);
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

    void set_samplerate(float samplerate) override
    {
        _samplerate_inv = 1.0f / samplerate;
    }

    void reset() override
    {
        _freq_lag.reset();
        _states.fill(0);
    }

private:
    ControlSmootherLinear  _freq_lag;
    double                 _zi;
    float                  _samplerate_inv{1.0 / DEFAULT_SAMPLERATE};
    std::array<double, 4>  _states{0, 0, 0, 0};
};

}// namespace bricks

#endif //BRICKS_DSP_FILTER_BRICKS_H
