#include "modulator_bricks.cpp"

#include "test_utils.h"

using namespace bricks;

class SaturationBrickTest : public ::testing::Test
{
protected:
    SaturationBrickTest() {}

    void SetUp()
    {
        make_test_sq_wave(_buffer);
    }

    AudioBuffer          _buffer;
    float                _clip_level;
    SaturationBrick _test_module{_clip_level, _buffer};
    const AudioBuffer&   _out_buffer{_test_module.audio_output(SaturationBrick::CLIP_OUT)};
};

TEST_F(SaturationBrickTest, OperationTest)
{
    _clip_level = 1;
    _test_module.render();
    for (auto sample : _out_buffer)
    {
        ASSERT_LT(sample, 1.0f);
        ASSERT_GT(sample, -1.0f);
    }
    /* Raise the gain until it starts affecting the waveshape */
    _clip_level = 2;
    _test_module.render();
    for (auto sample : _out_buffer)
    {
        ASSERT_LT(sample, 1.5f);
        ASSERT_GT(sample, -1.5f);
    }
}

class AASaturationBrickTest : public ::testing::Test
{
protected:
    AASaturationBrickTest() {}

    void SetUp()
    {
        make_test_sq_wave(_buffer);
    }

    AudioBuffer                       _buffer;
    float                             _gain;
    AASaturationBrick<ClipType::HARD> _test_module{_gain, _buffer};
    const AudioBuffer&                _out_buffer{_test_module.audio_output(AASaturationBrick<ClipType::HARD>::CLIP_OUT)};
};

TEST_F(AASaturationBrickTest, OperationTest)
{
    _gain = 1;
    _test_module.render();
    for (auto sample : _out_buffer)
    {
        ASSERT_LT(sample, 1.0f);
        ASSERT_GT(sample, -1.0f);
    }
    /* Raise the gain until it starts affecting the waveshape */
    _gain = 2;
    _test_module.render();
    for (auto sample : _out_buffer)
    {
        ASSERT_LT(sample, 1.5f);
        ASSERT_GT(sample, -1.5f);
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