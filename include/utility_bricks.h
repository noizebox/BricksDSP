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
class VcaBrick : public DspBrick
{
public:
    enum AudioOutputs
    {
        VCA_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    VcaBrick(const float& gain, const AudioBuffer& audio_in) : _gain_port(gain),
                                                               _audio_in(audio_in) {}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _audio_buffer;
    }

    void render() override
    {
        if constexpr (response == Response::LINEAR)
        {
            _gain_lag.set(_gain_port.value());
        }
        else if (response == Response::LOG)
        {
            _gain_lag.set(to_db_approx(_gain_port.value()));
        }
        AudioBuffer gain = _gain_lag.get_all();
        for (size_t s = 0; s < _audio_buffer.size(); ++s)
        {
            _audio_buffer[s] = _audio_in[s] * gain[s];
        }
    }

private:
    ControlPort             _gain_port;
    const AudioBuffer&      _audio_in;
    AudioBuffer             _audio_buffer;
    ControlSmootherLinear   _gain_lag;
};

/* General n to 1 audio mixer with individual gain controls for each input
 * Instantiation example:
 * AudioMixerBrick<2> mixer({gain1_, gain_2}, {audio_in_1, audio_in_2}); */

template <size_t channel_count, Response response>
class AudioMixerBrick : public DspBrick
{
public:
    enum AudioOutputs
    {
        MIX_OUT = 0,
        MAX_AUDIO_OUTS,
    };

    // TODO - Make a nice variadic template constructor here
    AudioMixerBrick(std::array<ControlPort, channel_count> gains,
                    std::array<AudioPort, channel_count> audio_ins) : _gains{gains},
                                                                      _audio_ins{audio_ins} {}

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_AUDIO_OUTS);
        return _output_buffer;
    }

    void render() override
    {
        for (auto& s : _output_buffer)
        {
            s = 0.0f;
        }
        for (unsigned i = 0; i < channel_count; ++i)
        {
            if constexpr (response == Response::LINEAR)
            {
                _gain_lags[i].set(_gains[i].value());
            }
            else if (response == Response::LOG)
            {
                _gain_lags[i].set(to_db_approx(_gains[i].value()));
            }

            AudioBuffer gain = _gain_lags[i].get_all();
            auto& audio_in = _audio_ins[i].buffer();
            for (size_t s = 0; s < _output_buffer.size(); ++s)
            {
                _output_buffer[s] += audio_in[s] * gain[i];
            }
        }
    }

private:
    std::array<ControlPort, channel_count> _gains;
    std::array<ControlSmootherLinear, channel_count> _gain_lags;
    std::array<AudioPort, channel_count> _audio_ins;
    AudioBuffer _output_buffer;
};

/* General n to 1 audio mixer without gain controls */
template <size_t channel_count>
class AudioSummerBrick : public DspBrick
{
public:
    enum AudioOutputs
    {
        SUM_OUT = 0,
        MAX_CONTROL_OUTS,
    };
    template <class ...T>
    explicit AudioSummerBrick(T&... inputs) : _inputs{AudioPort(inputs)...}
    {
#ifdef LINUX
        static_assert(sizeof...(inputs) == channel_count);
#endif
    }

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_CONTROL_OUTS);
        return _output_buffer;
    }

    void render() override
    {
        for (auto& s : _output_buffer)
        {
            s = 0.0f;
        }
        for (auto& input : _inputs)
        {
            auto& in_buffer = input.buffer();
            for (unsigned s = 0; s < _output_buffer.size(); ++s)
            {
                _output_buffer[s] += in_buffer[s];
            }
        }
    }

private:
    std::array<AudioPort, channel_count> _inputs;
    AudioBuffer _output_buffer;
};

/* Audio rate multiplier */
template <size_t channel_count>
class AudioMultiplierBrick : public DspBrick
{
public:
    enum AudioOutputs
    {
        MULT_OUT = 0,
        MAX_CONTROL_OUTS,
    };
    template <class ...T>
    explicit AudioMultiplierBrick(T&... inputs) : _inputs{AudioPort(inputs)...}
    {
        static_assert(sizeof...(inputs) == channel_count);
    }

    const AudioBuffer& audio_output(int n) override
    {
        assert(n < MAX_CONTROL_OUTS);
        return _output_buffer;
    }

    void render() override
    {
        for (auto& s : _output_buffer)
        {
            s = 1.0f;
        }
        for (auto& input : _inputs)
        {
            auto& in_buffer = input.buffer();
            for (unsigned s = 0; s < _output_buffer.size(); ++s)
            {
                _output_buffer[s] *= in_buffer[s];
            }
        }
    }

private:
    std::array<AudioPort, channel_count> _inputs;
    AudioBuffer _output_buffer;
};

/* General n to 1 control signal mixer, linear gain control */
template <size_t channel_count>
class ControlMixerBrick : public DspBrick
{
public:
    enum ControlOutputs
    {
        MIX_OUT = 0,
        MAX_CONTROL_OUTS,
    };
    template <class ...T>
    explicit ControlMixerBrick(T&... inputs) : _inputs{ControlPort(inputs)...}
    {
        static_assert(sizeof...(inputs) == channel_count * 2);
    }

    const float& control_output(int n) override
    {
        assert(n < MAX_CONTROL_OUTS);
        return _output;
    }

    void render() override
    {
        _output = 0.0f;
        for (unsigned int i = 0; i < channel_count; ++i)
        {
            _output += _inputs[i].value() * _inputs[i + channel_count].value();
        }
    }

private:
    std::array<ControlPort, channel_count * 2> _inputs;
    float _output{0};
};


/* General n to 1 control signal mixer without gain controls */
template <size_t channel_count>
class ControlSummerBrick : public DspBrick
{
public:
    enum ControlOutputs
    {
        SUM_OUT = 0,
        MAX_CONTROL_OUTS,
    };
    template <class ...T>
    explicit ControlSummerBrick(T&... inputs) : _inputs{ControlPort(inputs)...}
    {
#ifdef LINUX
        static_assert(sizeof...(inputs) == channel_count);
#endif
    }

    const float& control_output(int n) override
    {
        assert(n < MAX_CONTROL_OUTS);
        return _output;
    }

    void render() override
    {
        _output = 0.0f;
        for (auto& input : _inputs)
        {
            _output += input.value();
        }
    }

private:
    std::array<ControlPort, channel_count> _inputs;
    float _output{0};
};


/* General n to 1 control signal multiplier */
template <size_t channel_count>
class ControlMultiplierBrick : public DspBrick
{
public:
    enum ControlOutputs
    {
        MULT_OUT = 0,
        MAX_CONTROL_OUTS,
    };
    template <class ...T>
    explicit ControlMultiplierBrick(T&... inputs) : _inputs{ControlPort(inputs)...}
    {
#ifdef LINUX
        static_assert(sizeof...(inputs) == channel_count);
#endif
    }

    const float& control_output(int n) override
    {
        assert(n < MAX_CONTROL_OUTS);
        return _output;
    }

    void render() override
    {
        _output = 1.0f;
        for (auto& input : _inputs)
        {
            _output *= input.value();
        }
    }

private:
    std::array<ControlPort, channel_count> _inputs;
    float _output{0};
};

/* N to M control signal linear combinator. Useful för creating meta controllers/parameters */
template <size_t input_count, size_t output_count, bool clamp_output = true>
class MetaControlBrick : public DspBrick
{
    using Matrix = std::array<std::array<float, output_count>, input_count>;
public:
    template <class ...T>
    explicit MetaControlBrick(T&... inputs) : _inputs{ControlPort(inputs)...}
    {
        static_assert(sizeof...(inputs) == input_count);
    }

    const float& control_output(int n) override
    {
        assert(static_cast<size_t>(n) < output_count);
        return _outputs[n];
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
        _outputs.fill(0.0f);
        for (unsigned int i = 0; i < input_count; ++i)
        {
            float input = _inputs[i].value();
            for (unsigned int j = 0; j < output_count; ++j)
            {
                _outputs[j] += input * _components[i][j];
            }
        }
        if constexpr(clamp_output)
        {
            for (auto& v :_outputs)
            {
                v = clamp(v, _clamp_min, _clamp_max);
            }
        }
    }

private:
    std::array<ControlPort, input_count> _inputs;
    std::array<float, output_count> _outputs;
    Matrix _components;
    float _clamp_min{0.0f};
    float _clamp_max{1.0f};
};
}// namespace bricks

#endif //BRICKS_DSP_UTILITY_BRICKS_H
