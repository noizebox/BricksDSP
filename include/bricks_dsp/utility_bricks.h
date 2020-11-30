#ifndef BRICKS_DSP_UTILITY_BRICKS_H
#define BRICKS_DSP_UTILITY_BRICKS_H

#include <cassert>

#include "dsp_brick.h"
namespace bricks {

enum class Response
{
    LINEAR,
    LOG
};

/* Generic gain control of audio signal */
template <Response response>
class VcaBrick : public DspBrickImpl<1, 0, 1, 1>
{
public:
    enum ControlInput
    {
        GAIN = 0
    };

    enum AudioOutput
    {
        VCA_OUT = 0
    };

    VcaBrick() = default;

    VcaBrick(const float* gain, const AudioBuffer* audio_in)
    {
        set_control_input(ControlInput::GAIN, gain);
        set_audio_input(DEFAULT_INPUT, audio_in);
    }

    void render() override
    {
        const auto& audio_in = _input_buffer(DEFAULT_INPUT);
        auto& audio_out = _output_buffer(AudioOutput::VCA_OUT);
        float gain = _ctrl_value(ControlInput::GAIN);
        if constexpr (response == Response::LINEAR)
        {
            _gain_lag.set(gain);
        }
        else if (response == Response::LOG)
        {
            _gain_lag.set(to_db_approx(gain));
        }
        auto gain_lag = _gain_lag;
        for (int s = 0; s < audio_out.size(); ++s)
        {
            audio_out[s] = audio_in[s] * gain_lag.get();
        }
        _gain_lag = gain_lag;
    }

private:
    ControlSmootherLinear   _gain_lag;
};

/* General n to 1 audio mixer with individual gain controls for each input
 * Instantiation example:
 * AudioMixerBrick<2> mixer({gain1_, gain_2}, {audio_in_1, audio_in_2}); */

template <int channel_count, Response response>
class AudioMixerBrick : public DspBrickImpl<channel_count, 0, channel_count, 1>
{
    using this_template = DspBrickImpl<channel_count, 0, channel_count, 1>;

public:
    enum AudioOutput
    {
        MIX_OUT = 0
    };

    AudioMixerBrick() = default;

    AudioMixerBrick(std::array<const float*, channel_count> gains,
                    std::array<const AudioBuffer*, channel_count> audio_ins)
    {
        for (unsigned int i = 0; i < gains.size(); ++i)
        {
            this_template::set_control_input(i, gains[i]);
        }
        for (unsigned int i = 0; i < audio_ins.size(); ++i)
        {
            this_template::set_audio_input(i, audio_ins[i]);
        }
    }

    void render() override
    {
        auto& audio_out = this_template::_output_buffer(AudioOutput::MIX_OUT);
        audio_out.fill(0.0f);

        for (int i = 0; i < channel_count; ++i)
        {
            float gain = this_template::_ctrl_value(i);
            if constexpr (response == Response::LINEAR)
            {
                _gain_lags[i].set(gain);
            }
            else if (response == Response::LOG)
            {
                _gain_lags[i].set(to_db_approx(gain));
            }

            auto gain_lag = _gain_lags[i];
            const auto& audio_in = this_template::_input_buffer(i);
            for (int s = 0; s < audio_out.size(); ++s)
            {
                audio_out[s] += audio_in[s] * gain_lag.get();
            }
            _gain_lags[i] = gain_lag;
        }
    }

private:
    std::array<ControlSmootherLinear, channel_count> _gain_lags;
};

/* General n to 1 audio mixer without gain controls */
template <int channel_count>
class AudioSummerBrick : public DspBrickImpl<0, 0, channel_count, 1>
{
    using this_template = DspBrickImpl<0, 0, channel_count, 1>;

public:
    enum AudioOutput
    {
        SUM_OUT = 0,
    };

    AudioSummerBrick() = default;

    template <class ...T>
    explicit AudioSummerBrick(T... inputs)
    {
        static_assert(sizeof...(inputs) == channel_count);
        std::array<const AudioBuffer*, channel_count> audio_ins = {{inputs...}};
        for (int i = 0; i < channel_count; ++i)
        {
            this_template::set_audio_input(i, audio_ins[i]);
        }
    }

    void render() override
    {
        auto& audio_out = this_template::_output_buffer(AudioOutput::SUM_OUT);
        audio_out.fill(0.0f);

        for (int i = 0; i < channel_count; ++i)
        {
            const auto& audio_in = this_template::_input_buffer(i);
            for (int s = 0; s < audio_out.size(); ++s)
            {
                audio_out[s] += audio_in[s];
            }
        }
    }
};

/* Audio rate multiplier */
template <int channel_count>
class AudioMultiplierBrick : public DspBrickImpl<0, 0, channel_count, 1>
{
    using this_template = DspBrickImpl<0, 0, channel_count, 1>;

public:
    enum AudioOutput
    {
        MULT_OUT = 0,
    };

    AudioMultiplierBrick() = default;

    template <class ...T>
    explicit AudioMultiplierBrick(T... inputs)
    {
        static_assert(sizeof...(inputs) == channel_count);
        std::array<const AudioBuffer*, channel_count> audio_ins = {inputs...};
        for (int i = 0; i < channel_count; ++i)
        {
            this_template::set_audio_input(i, audio_ins[i]);
        }
    }

    void render() override
    {
        auto& audio_out = this_template::_output_buffer(AudioOutput::MULT_OUT);
        audio_out.fill(1.0f);

        for (int i = 0; i < channel_count; ++i)
        {
            const auto& audio_in = this_template::_input_buffer(i);
            for (int s = 0; s < audio_out.size(); ++s)
            {
                audio_out[s] *= audio_in[s];
            }
        }
    }
};

/* General n to 1 control signal mixer, linear gain control As with the
 * audio mixer, first n arguments are gains, the rest are control signals*/
template <int channel_count>
class ControlMixerBrick : public DspBrickImpl<channel_count * 2, 1, 0, 0>
{
    using this_template = DspBrickImpl<channel_count * 2, 1, 0, 0>;

public:
    enum ControlOutput
    {
        MIX_OUT = 0
    };

    ControlMixerBrick() = default;

    template <class ...T>
    explicit ControlMixerBrick(T... inputs)
    {
        static_assert(sizeof...(inputs) == channel_count * 2);
        std::array<const float*, channel_count * 2> ctrl_ins = {inputs...};
        for (int i = 0; i < channel_count * 2; ++i)
        {
            this_template::set_control_input(i, ctrl_ins[i]);
        }
    }

    void render() override
    {
        float output = 0.0f;
        for (int i = 0; i < channel_count; ++i)
        {
            output += this_template::_ctrl_value(i) * this_template::_ctrl_value(i + channel_count);
        }
        this_template::_set_ctrl_value(ControlOutput::MIX_OUT, output);
    }
};


/* General n to 1 control signal mixer without gain controls */
template <int channel_count>
class ControlSummerBrick : public DspBrickImpl<channel_count, 1, 0, 0>
{
    using this_template = DspBrickImpl<channel_count, 1, 0, 0>;

public:
    enum ControlOutput
    {
        SUM_OUT = 0
    };

    ControlSummerBrick() = default;

    template <class ...T>
    explicit ControlSummerBrick(T... inputs)
    {
        static_assert(sizeof...(inputs) == channel_count);
        std::array<const float*, channel_count> ctrl_ins = {inputs...};
        for (int i = 0; i < channel_count; ++i)
        {
            this_template::set_control_input(i, ctrl_ins[i]);
        }
    }

    void render() override
    {
        float output = 0.0f;
        for (int i = 0; i < channel_count; ++i)
        {
            output += this_template::_ctrl_value(i);
        }
        this_template::_set_ctrl_value(ControlOutput::SUM_OUT, output);
    }
};


/* General n to 1 control signal multiplier */
template <int channel_count>
class ControlMultiplierBrick : public DspBrickImpl<channel_count, 1, 0, 0>
{
    using this_template = DspBrickImpl<channel_count, 1, 0, 0>;

public:
    enum ControlOutput
    {
        MULT_OUT = 0
    };

    template <class ...T>
    explicit ControlMultiplierBrick(T... inputs)
    {
        static_assert(sizeof...(inputs) == channel_count);
        std::array<const float*, channel_count> ctrl_ins = {inputs...};
        for (int i = 0; i < channel_count; ++i)
        {
            this_template::set_control_input(i, ctrl_ins[i]);
        }
    }

    void render() override
    {
        float output = 1.0f;
        for (int i = 0; i < channel_count; ++i)
        {
            output *= this_template::_ctrl_value(i);
        }
        this_template::_set_ctrl_value(ControlOutput::MULT_OUT, output);
    }
};

/* N to M control signal linear combinator. Useful for creating meta controllers/parameters */
template <int input_count, int output_count, bool clamp_output = true>
class MetaControlBrick : public DspBrickImpl<input_count, output_count, 0, 0>
{
    using Matrix = std::array<std::array<float, output_count>, input_count>;
    using this_template = DspBrickImpl<input_count, output_count, 0, 0>;

public:
    MetaControlBrick() = default;

    template <class ...T>
    explicit MetaControlBrick(T... inputs)
    {
        static_assert(sizeof...(inputs) == input_count);
        std::array<const float*, input_count> ctrl_ins = {inputs...};
        for (int i = 0; i < input_count; ++i)
        {
            this_template::set_control_input(i, ctrl_ins[i]);
        }
    }

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

    void render() override
    {
        std::array<float, output_count> outputs;
        outputs.fill(0.0f);

        for (int i = 0; i < input_count; ++i)
        {
            float input = this_template::_ctrl_value(i);
            for (int j = 0; j < output_count; ++j)
            {
                outputs[j] += input * _components[i][j];
            }
        }
        for (int i = 0; i < output_count; ++i)
        {
            if constexpr(clamp_output)
            {
                this_template::_set_ctrl_value(i, clamp(outputs[i], _clamp_min, _clamp_max));
            }
            else
            {
                this_template::_set_ctrl_value(i, outputs[i]);
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
