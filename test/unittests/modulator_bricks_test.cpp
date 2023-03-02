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
    SaturationBrick<ClipType::SOFT> _test_module{&_clip_level, &_buffer};
    const AudioBuffer&              _out_buffer{*_test_module.audio_output(SaturationBrick<ClipType::SOFT>::CLIP_OUT)};
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
    AASaturationBrick<ClipType::HARD> _test_module{&_gain, &_buffer};
    const AudioBuffer&                _out_buffer{*_test_module.audio_output(AASaturationBrick<ClipType::HARD>::CLIP_OUT)};
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

class FixedDelayBrickTest : public ::testing::Test
{
protected:
    FixedDelayBrickTest() {}

    void SetUp()
    {
        fill_buffer(_buffer, 1.0f);
    }

    AudioBuffer          _buffer;
    FixedDelayBrick<LinearInterpolation<float>>  _test_module{&_buffer, std::chrono::milliseconds(200)};
    const AudioBuffer&   _out_buffer{*_test_module.audio_output(FixedDelayBrick<LinearInterpolation<float>>::DELAY_OUT)};
};

TEST_F(FixedDelayBrickTest, OperationTest)
{
    _test_module.render();
    assert_buffer(_out_buffer, 0.0f);
}

TEST_F(FixedDelayBrickTest, AccuracyTest)
{
    /* Delay == 1 buffer */
    _test_module.set_delay_samples(PROC_BLOCK_SIZE);
    _test_module.render();
    assert_buffer(_out_buffer, 0.0f);
    _test_module.render();
    assert_buffer(_out_buffer, 1.0f);

    /* Delay == 1.5 buffers */
    _test_module.reset();
    _test_module.set_delay_samples(1.5 * PROC_BLOCK_SIZE);
    _test_module.render();
    assert_buffer(_out_buffer, 0.0f);
    _test_module.render();
    /* First half of buffer should be 0, the rest should be 1:s */
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        if (i < PROC_BLOCK_SIZE / 2)
        {
            EXPECT_FLOAT_EQ(0.0f, _out_buffer[i]);
        }
        else
        {
            EXPECT_GT(_out_buffer[i], 0.0f);
        }
    }
}

class ModDelayBrickTest : public ::testing::Test
{
protected:
    ModDelayBrickTest() {}

    void SetUp()
    {
        fill_buffer(_buffer, 1.0f);
    }

    AudioBuffer          _buffer;
    float                _delay_mod{0.5};
    ModDelayBrick<LinearInterpolation<float>>  _test_module{&_delay_mod, &_buffer, std::chrono::milliseconds(200)};
    const AudioBuffer&   _out_buffer{*_test_module.audio_output(ModDelayBrick<CubicInterpolation<float>>::DELAY_OUT)};

};

TEST_F(ModDelayBrickTest, OperationTest)
{
    _test_module.render();
    assert_buffer(_out_buffer, 0.0f);
}

TEST_F(ModDelayBrickTest, AccuracyTest)
{
    /* Delay == 1 buffer */
    _test_module.set_delay_samples(PROC_BLOCK_SIZE);
    _test_module.render();
    _delay_mod = 0.5;
    assert_buffer(_out_buffer, 0.0f);
    _test_module.render();
    assert_buffer(_out_buffer, 1.0f);

    /* Modulate delay to 1.5 buffers */
    _test_module.reset();
    _delay_mod = 0.75;
    _test_module.render();
    assert_buffer(_out_buffer, 0.0f);
    _test_module.render();
    /* First half of buffer should be 0, the rest should be 1:s */
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        if (i < PROC_BLOCK_SIZE / 2)
        {
            EXPECT_FLOAT_EQ(0.0f, _out_buffer[i]);
        }
        else
        {
            EXPECT_GT(_out_buffer[i], 0.0f);
        }
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
    ModulatedDelayBrick  _test_module{&_delay_time, &_buffer, 1};
    const AudioBuffer&   _out_buffer{*_test_module.audio_output(ModulatedDelayBrick::DELAY_OUT)};
};

TEST_F(ModulatedDelayBrickTest, OperationTest)
{
    _test_module.render();
    assert_buffer(_out_buffer, 0.0f);
}

class AllpassDelayBrickTest : public ::testing::Test
{
protected:
    AllpassDelayBrickTest() {}

    void SetUp()
    {
        fill_buffer(_buffer, 0.0f);
        _buffer[0] = 1.0;
    }

    AudioBuffer          _buffer;
    float                _delay_time{1.0};
    float                _gain{0.5};
    AllpassDelayBrick<4>  _test_module{&_delay_time, &_gain, &_buffer};
    const AudioBuffer&   _out_buffer{*_test_module.audio_output(AllpassDelayBrick<4>::DELAY_OUT)};
};

TEST_F(AllpassDelayBrickTest, OperationTest)
{
    _test_module.render();
    EXPECT_FLOAT_EQ(_out_buffer[0], -0.5f);
    EXPECT_FLOAT_EQ(_out_buffer[1], 0.0f);
    EXPECT_FLOAT_EQ(_out_buffer[2], 0.0f);
    EXPECT_FLOAT_EQ(_out_buffer[3], 0.0f);
    EXPECT_FLOAT_EQ(_out_buffer[4], 0.75f); // Second impulse here
    EXPECT_FLOAT_EQ(_out_buffer[5], 0.0f);

    /* Modulate the delay */
    SetUp();
    _delay_time = 0.5f;
    _test_module.reset();
    _test_module.render();
    EXPECT_FLOAT_EQ(_out_buffer[0], -0.5f);
    EXPECT_FLOAT_EQ(_out_buffer[1], 0.0f);
    EXPECT_FLOAT_EQ(_out_buffer[2], 0.75f);  // Second impulse here
    EXPECT_FLOAT_EQ(_out_buffer[3], 0.0f);
    EXPECT_FLOAT_EQ(_out_buffer[4], 0.375f); // Third impulse here
    EXPECT_FLOAT_EQ(_out_buffer[5], 0.0f);
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
    BitRateReducerBrick   _test_module{&_depth, &_buffer};
    const AudioBuffer&    _out_buffer{*_test_module.audio_output(BitRateReducerBrick::BITRED_OUT)};
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
    /* With bits = 0 we should have only 1 bit left, values should be 0.0 or +/- 1.0
     * Which is 1,5 bits in reality. But it sounds better if it's symmetrical around 0 */
    _depth = 0;
    _test_module.render();
    for (auto sample : _out_buffer)
    {
        EXPECT_PRED1(is_2_bit_predicate, sample);
    }
}

class SampleRateReducerBrickTest : public ::testing::Test
{
protected:
    SampleRateReducerBrickTest() {}

    void SetUp()
    {
        make_test_sine_wave(_buffer);
    }

    AudioBuffer             _buffer;
    float                   _samplerate;
    SampleRateReducerBrick  _test_module{&_samplerate, &_buffer};
    const AudioBuffer&      _out_buffer{*_test_module.audio_output(SampleRateReducerBrick::DOWNSAMPLED_OUT)};
};

TEST_F(SampleRateReducerBrickTest, OperationTest)
{
    _samplerate = 1.0f;
    _test_module.render();
    /* Just test bibo stability at this point */
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        EXPECT_LE(-1.0f, _out_buffer[i]);
        EXPECT_GE(1.0f, _out_buffer[i]);
    }
    _samplerate = 0;
    _test_module.render();
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        EXPECT_LE(-1.0f, _out_buffer[i]);
        EXPECT_GE(1.0f, _out_buffer[i]);
    }
}
