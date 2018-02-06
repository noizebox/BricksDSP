#include "gtest/gtest.h"
#define private public

#include "utility_bricks.h"
#include "test_utils.h"

using namespace bricks;

class VcaBrickTest : public ::testing::Test
{
protected:
    VcaBrickTest() {}

    AudioBuffer _buffer;
    float       _gain;
    VcaBrick    _test_module{_gain, _buffer};
};

TEST_F(VcaBrickTest, OperationalTest)
{
    fill_buffer(_buffer, 0.5f);
    _gain = 2.0f;
    _test_module.render();
    auto& out_buffer = _test_module.audio_output(VcaBrick::VCA_OUT);
    float last = 0;
    for (auto& sample : out_buffer)
    {
        ASSERT_LT(last, sample);
        last = sample;
    }
    ASSERT_EQ(1.0f, last);
}

class AudioMixerBrickTest : public ::testing::Test
{
protected:
    AudioMixerBrickTest() {}

    AudioBuffer _buffer_1;
    AudioBuffer _buffer_2;
    float       _gain_1;
    float       _gain_2;
    AudioMixerBrick<2>   _test_module{{_gain_1, _gain_2}, {_buffer_1, _buffer_2}};
};

TEST_F(AudioMixerBrickTest, OperationalTest)
{
    fill_buffer(_buffer_1, 0.5f);
    fill_buffer(_buffer_2, 0.25f);
    _gain_1 = 0.5f;
    _gain_2 = 3.0f;
    _test_module.render();
    assert_buffer(_test_module.audio_output(AudioMixerBrick<2>::MIX_OUT), 1.0f);
}