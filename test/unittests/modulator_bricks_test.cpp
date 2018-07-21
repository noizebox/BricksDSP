#include "modulator_bricks.cpp"

#include "test_utils.h"

using namespace bricks;

class AudioSaturationBrickTest : public ::testing::Test
{
protected:
    AudioSaturationBrickTest() {}

    void SetUp()
    {
        make_test_sq_wave(_buffer);
    }

    AudioBuffer          _buffer;
    float                _clip_level;
    SaturationBrick _test_module{_clip_level, _buffer};
    const AudioBuffer&   _out_buffer{_test_module.audio_output(SaturationBrick::CLIP_OUT)};
};

TEST_F(AudioSaturationBrickTest, OperationTest)
{
    _clip_level = 1;
    _test_module.render();
    for (auto sample : _out_buffer)
    {
        ASSERT_LT(sample, 1.0f);
        ASSERT_GT(sample, -1.0f);
    }
    /* Lower the clip level until it starts affecting the waveshape */
    _clip_level = 0.2;
    _test_module.render();
    for (auto sample : _out_buffer)
    {
        ASSERT_LT(sample, 0.3f);
        ASSERT_GT(sample, -0.3f);
    }
}

class ModulatedDelayBrickTest : public ::testing::Test
{
protected:
    ModulatedDelayBrickTest() {}

    void SetUp()
    {
        fill_buffer(_buffer, 1.0f);
    }

    AudioBuffer          _buffer;
    float                _delay_time;
    ModulatedDelayBrick  _test_module{_delay_time, _buffer, 1};
    const AudioBuffer&   _out_buffer{_test_module.audio_output(ModulatedDelayBrick::DELAY_OUT)};
};

TEST_F(ModulatedDelayBrickTest, OperationTest)
{
    _test_module.render();
    assert_buffer(_out_buffer, 0.0f);
}