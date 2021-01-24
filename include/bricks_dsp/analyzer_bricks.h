#ifndef BRICKS_DSP_ANALYZER_BRICKS_H
#define BRICKS_DSP_ANALYZER_BRICKS_H

#ifndef BRICKS_DSP_OSCILLATORS_H
#define BRICKS_DSP_OSCILLATORS_H

#include <memory>
#include <vector>
#include <cassert>

#include "dsp_brick.h"
#include "expiring_queue.h"

namespace bricks {

template <int block_size, int past_blocks = 4>
class OscilloscopeBrick : public DspBrickImpl<1, 0, 1, 0>
{
public:
    using Block = AlignedArray<float, block_size>;

    enum ControlInput
    {
        TRIG_LEVEL = 0,
    };

    OscilloscopeBrick()
    {
        _rt_data = std::make_unique<ExpiringQueue<Block, past_blocks>>();
        _data = std::make_unique<std::array<Block, past_blocks>>();
    };

    OscilloscopeBrick(const float* trig_level, const AudioBuffer* audio_in)
    {
        OscilloscopeBrick();
        set_control_input(0, trig_level);
        set_audio_input(0, audio_in);
    }

    void reset() override
    {
        //_rt_data->clear();
        for (auto& i : &_data.get())
        {
            i.fill(0);
        }
    }

    void sync()
    {
        for (int i = 0; i < past_blocks; ++i)
        {
            _data[i] = _rt_data->get(i);
        }
    }

    const Block& display_data(int history)
    {
        return _data.at(history);
    }

    void render() override
    {
        float trig_level = _ctrl_value(ControlInput::TRIG_LEVEL);
        bool triggered = _triggered;
        int trig_count = _trig_count;
        auto& in = _input_buffer(0);
        for (int i = 0; i < PROC_BLOCK_SIZE ; ++i)
        {
            float sample = in[i];
            /*if (_triggered)
            {

            }*/
            _block[_block_index++] = sample;
            if (_block_index >= _block.size())
            {
                _rt_data->push(_block);
                _block_index = 0;
            }

        }
    }

private:
    static constexpr float HYSTERESIS = 0.1;

    std::unique_ptr<ExpiringQueue<Block, past_blocks>>  _rt_data;
    std::unique_ptr<std::array<Block, past_blocks>>     _data;

    Block _block;
    int   _block_index{0};
    bool  _triggered{false};
    float _trig_value{0};
    int   _trig_count{0};
};

}// namespace bricks

#endif //BRICKS_DSP_ANALYZER_BRICKS_H
