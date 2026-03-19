#ifndef BRICKS_DSP_ANALYZER_BRICKS_H
#define BRICKS_DSP_ANALYZER_BRICKS_H

#include <memory>
#include <vector>
#include <cassert>

#include "dsp_brick.h"
#include "utils.h"
#include "expiring_queue.h"

namespace bricks {

template <int block_size, int past_blocks = 4, size_t os_factor = 1>
class OscilloscopeBrick : public DspBrickImpl<os_factor, 2, 0, 1, 0>
{
public:
    using Block = AlignedArray<float, block_size>;

    OscilloscopeBrick()
    {
        _rt_data = std::make_unique<ExpiringQueue<Block, past_blocks>>();
        _data = std::make_unique<std::array<Block, past_blocks>>();
    };

    void reset() override
    {
        //_rt_data->clear();
        for (auto& i : *_data.get())
        {
            i.fill(0);
        }
    }

    void sync()
    {
        for (int i = 0; i < past_blocks; ++i)
        {
            (*_data)[i] = _rt_data->get(i);
        }
    }

    const Block& display_data(int history)
    {
        return (*_data)[history];
    }

    void render(float trig_level, float skip, const AudioOsBuffer<os_factor>& audio_in)
    {
        bool triggered = _triggered;
        int trig_count = _trig_count;
        float prev = 0;

        for (int i = 0; i < audio_in.size(); ++i)
        {
            float sample = audio_in[i];

            if (prev < 0.01f && sample > trig_level)
            {
                if (!triggered)
                {
                    _block_index = 0; // restart block
                    triggered = true;
                }
            }

            if (trig_count++ > skip)
            {
                _block[_block_index++] = sample;
                if (_block_index >= _block.size())
                {
                    _rt_data->push(_block);
                    _block_index = 0;
                    triggered = false;
                }
                trig_count = 0;
            }
            prev = sample;
        }

        _trig_count = trig_count;
        _triggered = triggered;
    }

private:
    static constexpr float HYSTERESIS = 0.1;

    std::unique_ptr<ExpiringQueue<Block, past_blocks>> _rt_data;
    std::unique_ptr<std::array<Block, past_blocks>> _data;

    Block _block;
    int _block_index{0};
    bool _triggered{false};
    float _trig_value{0};
    int _trig_count{0};
};

/* Metering, rms and peak */
template<int skip = 1, size_t os_factor = 1>
class MeterBrick : public DspBrickImpl<os_factor, 0, 2, 1, 0>
{
public:
    enum ControlOutput
    {
        RMS = 0,
        PEAK
    };

    MeterBrick() = default;

    void reset() override
    {
        _rms_smoother.reset();
        _peak_smoother.reset();
    }

    void render(float& rms_out, float& peak_out, const AudioOsBuffer<os_factor>& audio_in)
    {
        float sq_sum = 0;
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::min();

        for (int i = 0; i < audio_in.size(); i += skip)
        {
            float s = audio_in[i];
            sq_sum += s * s;
            min = s < min ? s : min;
            max = s > max ? s : max;
        }
        float rms = std::sqrt((skip / static_cast<float>(PROC_BLOCK_SIZE)) * sq_sum);
        _rms_smoother.set(from_db_approx(rms));
        _peak_smoother.set(from_db_approx(std::max(max, -min)));
        rms_out = _rms_smoother.get();
        peak_out = _peak_smoother.get();
    }

private:
    OnePoleLag<PROC_BLOCK_SIZE / 4>  _rms_smoother;
    OnePoleLag<PROC_BLOCK_SIZE / 4>  _peak_smoother;
};

}// namespace bricks

#endif //BRICKS_DSP_ANALYZER_BRICKS_H
