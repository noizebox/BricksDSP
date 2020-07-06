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

    AudioBuffer                     _buffer;
    float                           _clip_level;
    SaturationBrick<ClipType::SOFT> _test_module{_clip_level, _buffer};
    const AudioBuffer&              _out_buffer{_test_module.audio_output(SaturationBrick<ClipType::SOFT>::CLIP_OUT)};
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

class BitRateReducerBrickTest : public ::testing::Test
{
protected:
    BitRateReducerBrickTest() {}

    void SetUp()
    {
        make_test_sine_wave(_buffer);
    }

    AudioBuffer           _buffer;
    float                 _depth;
    BitRateReducerBrick   _test_module{_depth, _buffer};
    const AudioBuffer&    _out_buffer{_test_module.audio_output(BitRateReducerBrick::OUT)};
};

bool is_2_bit_predicate(float val)
{
    return val == 0 || val == -1.0f || val == 1.0f;
}

TEST_F(BitRateReducerBrickTest, OperationTest)
{
    _depth = 1.0f;
    _test_module.render();
    /* 24 bits should be within the float error margin so they should compare equal */
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        EXPECT_FLOAT_EQ(_buffer[i], _out_buffer[i]);
    }
    /* With bits = 0 we should have only 1 bit left, values should be 0.0 or +/- 1.0 */
    _depth = 0;
    _test_module.render();
    for (auto sample : _out_buffer)
    {
        EXPECT_PRED1(is_2_bit_predicate, sample);
    }
}
