#ifndef BRICKS_DSP_UTILITY_BRICKS_H
#define BRICKS_DSP_UTILITY_BRICKS_H

#include <cassert>

#include "dsp_brick.h"
#include "utils.h"

#include <iostream>
namespace bricks {

enum class Response
{
    LINEAR,
    LOG
};

/* Generic gain control of audio signal */
template <Response response, size_t os_factor = 1>
class VcaBrick : public DspBrickImpl<os_factor, 1, 0, 1, 1>
{
public:
    VcaBrick() = default;

    void reset() override
    {
        _gain_lag.reset();
    }

    void render(float gain, AudioOsBuffer<os_factor>& audio_in, AudioOsBuffer<os_factor>& audio_out)
    {
        if constexpr (response == Response::LOG)
        {
            gain = to_db_approx(gain);
        }
        _gain_lag.set(gain);
        if (_gain_lag.moving())
        {
            auto gain_lag = _gain_lag;
            for (int s = 0; s < audio_out.size(); ++s)
            {
                audio_out[s] = audio_in[s] * gain_lag.get();
            }
            _gain_lag = gain_lag;
        }
        else
        {
            for (int s = 0; s < audio_out.size(); ++s)
            {
                audio_out[s] = audio_in[s] * gain;
            }
        }
    }

private:
    LinearLag<os_factor>  _gain_lag;
};

/* General n to 1 audio mixer with individual gain controls for each input
 * Instantiation example:
 * AudioMixerBrick<2> mixer({gain1_, gain_2}, {audio_in_1, audio_in_2}); */

template <size_t channel_count, Response response, size_t os_factor = 1>
class AudioMixerBrick : public DspBrickImpl<os_factor, channel_count, 0, channel_count, 1>
{
public:
    AudioMixerBrick() = default;

    void reset() override
    {
        for (auto& lag : _gain_lags)
        {
            lag.reset();
        }
    }

    void render(const std::array<float, channel_count>& gains,
                const std::array<const AudioOsBuffer<os_factor>*, channel_count>& audio_ins,
                AudioOsBuffer<os_factor>& audio_out)
    {
        audio_out.fill(0.0f);

        for (int i = 0; i < channel_count; ++i)
        {
            auto gain_lag = _gain_lags[i];
            float gain = gains[i];

            if constexpr (response == Response::LOG)
            {
                gain = to_db_approx(gain);
            }
            gain_lag.set(gain);
            const AudioOsBuffer<os_factor>* audio_in = audio_ins[i];

            if (gain_lag.moving())
            {
                for (int s = 0; s < audio_out.size(); ++s)
                {
                    audio_out[s] += (*audio_in)[s] * gain_lag.get();
                }
            }
            else
            {
                for (int s = 0; s < audio_out.size(); ++s)
                {
                    audio_out[s] += (*audio_in)[s] * gain;
                }
            }
            _gain_lags[i] = gain_lag;
        }
    }

private:
    std::array<LinearLag<os_factor>, channel_count> _gain_lags;
};

/* General n to 2 audio mixer with individual gain controls for each input
 * Instantiation example:
 * StereoMixerBrick<2> mixer({pan_1, gain_1, pan_2, gain_2}, {audio_in_1, audio_in_2}); */
template <size_t channel_count, Response response, size_t os_factor = 1>
class StereoMixerBrick : public DspBrickImpl<os_factor, channel_count * 2, 0, channel_count, 2>
{
    using this_template = DspBrickImpl<os_factor, channel_count * 2, 0, channel_count, 2>;

public:
    void reset() override
    {
        for (auto& lag : _gain_lags)
        {
            lag.reset();
        }
    }
    /* Control values are: pan 0, gain 0, pan 1, gain1, pan 2 ...etc */
    void render(const std::array<float, channel_count * 2>& ctrl_values,
                const std::array<const AudioOsBuffer<os_factor>*, channel_count>& audio_ins,
                AudioOsBuffer<os_factor>& left_out,
                AudioOsBuffer<os_factor>& right_out)
    {
        left_out.fill(0.0f);
        right_out.fill(0.0f);

        for (int i = 0; i < channel_count; ++i)
        {
            auto left_lag = _gain_lags[i * 2];
            auto right_lag = _gain_lags[i * 2 + 1];

            // 0 is fully left and 1 fully right
            float pan = clamp(ctrl_values[i * 2], 0.0f, 1.0f);
            float gain = ctrl_values[i * 2 + 1];

            if constexpr (response == Response::LOG)
            {
                gain = to_db_approx(gain);
            }

            // Equal gain pan law (-6dB), should probably do better curves
            float left_gain = gain * (1.0f - pan);
            float right_gain = gain * pan;

            left_lag.set(left_gain);
            right_lag.set(right_gain);

            const auto audio_in = audio_ins[i];

            if (left_lag.moving() || right_lag.moving())
            {
                for (int s = 0; s < left_out.size(); ++s)
                {
                    left_out[s] += (*audio_in)[s] * left_lag.get();
                    right_out[s] += (*audio_in)[s] * right_lag.get();
                }
            }
            else
            {
                for (int s = 0; s < left_out.size(); ++s)
                {
                    left_out[s] += (*audio_in)[s] * left_gain;
                    right_out[s] += (*audio_in)[s] * right_gain;
                }
            }
            _gain_lags[i * 2] = left_lag;
            _gain_lags[i * 2 + 1] = right_lag;
        }
    }

private:
    std::array<LinearLag<os_factor>, channel_count * 2> _gain_lags;
};

/* General n to 1 unity gain audio mixer */
template <size_t channel_count, size_t os_factor = 1>
class AudioSummerBrick : public DspBrickImpl<os_factor, 0, 0, channel_count, 1>
{
public:
    void render(const std::array<const AudioOsBuffer<os_factor>*, channel_count>& audio_ins,
                AudioOsBuffer<os_factor>& audio_out)
    {
        audio_out.fill(0.0f);

        for (int i = 0; i < channel_count; ++i)
        {
            const auto& audio_in = audio_ins[i];
            for (int s = 0; s < audio_out.size(); ++s)
            {
                audio_out[s] += (*audio_in)[s];
            }
        }
    }
};


/* Audio rate multiplier */
template <size_t channel_count, size_t os_factor = 1>
class AudioMultiplierBrick : public DspBrickImpl<os_factor, 0, 0, channel_count, 1>
{
public:
    AudioMultiplierBrick() = default;

    void render(const std::array<const AudioOsBuffer<os_factor>*, channel_count>& audio_ins,
                AudioOsBuffer<os_factor>& audio_out)
    {
        audio_out.fill(1.0f);

        for (int i = 0; i < channel_count; ++i)
        {
            const auto& audio_in = audio_ins[i];
            for (int s = 0; s < audio_out.size(); ++s)
            {
                audio_out[s] *= (*audio_in)[s];
            }
        }
    }
};

/* General n to 1 control signal mixer, linear gain control As with the
 * audio mixer, first n arguments are gains, the rest are control signals*/
template <size_t channel_count>
class ControlMixerBrick : public DspBrickImpl<1, channel_count * 2, 1, 0, 0>
{
public:
    ControlMixerBrick() = default;

    void render(const std::array<float, channel_count * 2>& inputs, float& output)
    {
        float mix = 0.0f;
        for (int i = 0; i < channel_count; ++i)
        {
            mix += inputs[i] * inputs[i + channel_count];
        }
        output = mix;
    }
};


/* General n to 1 control signal mixer without gain controls */
template <size_t channel_count>
class ControlSummerBrick : public DspBrickImpl<1, channel_count, 1, 0, 0>
{
public:
    ControlSummerBrick() = default;

    void render(const std::array<float, channel_count>& inputs, float& output)
    {
        float sum = 0.0f;
        for (int i = 0; i < channel_count; ++i)
        {
            sum += inputs[i];
        }
        output = sum;
    }
};


/* General n to 1 control signal multiplier */
template <int channel_count>
class ControlMultiplierBrick : public DspBrickImpl<1, channel_count, 1, 0, 0>
{
public:
    ControlMultiplierBrick() = default;

    void render(const std::array<float, channel_count>& inputs, float& output)
    {
        float prod = 1.0f;
        for (int i = 0; i < channel_count; ++i)
        {
            prod *= inputs[i];
        }
        output = prod;
    }
};


/* N to M control signal linear combinator. Useful for creating meta controllers/parameters */
template <size_t input_count, size_t output_count, bool clamp_output = true>
class MetaControlBrick : public DspBrickImpl<1, input_count, output_count, 0, 0>
{
    using Matrix = std::array<std::array<float, output_count>, input_count>;

public:
    MetaControlBrick() = default;

    void set_component(int input, std::array<float, output_count> component, float weight = 1.0f)
    {
        assert(static_cast<size_t>(input) < input_count);
        for (auto& v : component)
        {
            v *= weight;
        }
        _components[input] = component;
    }

    void set_output_clamp(float min, float max)
    {
        _clamp_min = min;
        _clamp_max = max;
    }

    void render(const std::array<float, input_count>& inputs,
                const std::array<float*, output_count>& outputs)
    {
        std::array<float, output_count> out;
        out.fill(0.0f);

        for (int i = 0; i < input_count; ++i)
        {
            float input = inputs[i];
            for (int j = 0; j < output_count; ++j)
            {
                out[j] += input * _components[i][j];
            }
        }
        for (int i = 0; i < output_count; ++i)
        {
            if constexpr(clamp_output)
            {
                *(outputs[i]) = clamp(out[i], _clamp_min, _clamp_max);
            }
            else
            {
                *(outputs[i]) = out[i];
            }
        }
    }

private:
    Matrix _components;
    float  _clamp_min{0.0f};
    float  _clamp_max{1.0f};
};
}// namespace bricks

#endif //BRICKS_DSP_UTILITY_BRICKS_H
