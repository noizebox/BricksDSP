#include "modulator_bricks.h"

#include "test_utils.h"

using namespace bricks;

class SaturationBrickTest : public ::testing::Test
{
protected:
    SaturationBrickTest() {}

    void SetUp()override
    {
        make_test_sq_wave(_input);
    }

    AudioBuffer                     _input;
    AudioBuffer                     _output;
    SaturationBrick<ClipType::SOFT> _test_module;
};

TEST_F(SaturationBrickTest, OperationTest)
{
    _test_module.render(1, _input, _output);
    for (auto sample : _output)
    {
        ASSERT_LT(sample, 1.0f);
        ASSERT_GT(sample, -1.0f);
    }
    /* Raise the gain until it starts affecting the waveshape */
    _test_module.render(2, _input, _output);
    for (auto sample : _output)
    {
        ASSERT_LT(sample, 1.5f);
        ASSERT_GT(sample, -1.5f);
    }
}

class AASaturationBrickTest : public ::testing::Test
{
protected:
    AASaturationBrickTest() {}

    void SetUp()override
    {
        make_test_sq_wave(_input);
    }

    AudioBuffer                       _input;
    AudioBuffer                       _output;
    AASaturationBrick<ClipType::HARD> _test_module;
};

TEST_F(AASaturationBrickTest, OperationTest)
{
    _test_module.render(1, _input, _output  );
    for (auto sample : _output)
    {
        ASSERT_LT(sample, 1.0f);
        ASSERT_GT(sample, -1.0f);
    }
    /* Raise the gain until it starts affecting the waveshape */
    _test_module.render(2, _input, _output);
    for (auto sample : _output)
    {
        ASSERT_LT(sample, 1.5f);
        ASSERT_GT(sample, -1.5f);
    }
}

class SustainerBrickTest : public ::testing::Test
{
protected:
    SustainerBrickTest() {}

    void SetUp()override
    {
        make_test_sq_wave(_inputs[0]);
        make_test_sq_wave(_inputs[1]);
        _test_module.reset();
        _test_module.set_samplerate(DEFAULT_SAMPLERATE);
    }

    std::array<AudioBuffer, 2> _inputs;
    std::array<AudioBuffer, 2> _outputs;
    std::array<float, 4>       _params;
    SustainerBrick<>           _test_module;
};

TEST_F(SustainerBrickTest, OperationTest)
{
    _params.fill(0);
    /* set gain to 0.2 */
    _test_module.render(0.2, 0, 0, 0, _inputs[0], _inputs[1], _outputs[0], _outputs[1]);
    float rms = 0;
    for (auto sample : _outputs[0])
    {
        rms += sample * sample;
    }
    EXPECT_GT(rms, 0.1);

    rms = 0;
    _test_module.render(0.1, 0, 0, 0, _inputs[0], _inputs[1], _outputs[0], _outputs[1]);
    _test_module.render(0.1, 0, 0, 0, _inputs[0], _inputs[1], _outputs[0], _outputs[1]);
    for (auto sample : _outputs[0])
    {
        rms += sample * sample;
    }
    EXPECT_GT(rms, 0.9);
}


class FixedDelayBrickTest : public ::testing::Test
{
protected:
    FixedDelayBrickTest() {}

    void SetUp()override
    {
        fill_buffer(_input, 1.0f);
    }

    AudioBuffer          _input;
    AudioBuffer          _output;
    FixedDelayBrick<LinearInterpolation<float>>  _test_module{std::chrono::milliseconds(200)};
};

TEST_F(FixedDelayBrickTest, OperationTest)
{
    _test_module.render(1.0f, _input, _output);
    assert_buffer(_output, 0.0f);
}

TEST_F(FixedDelayBrickTest, AccuracyTest)
{
    /* Delay == 1 buffer */
    _test_module.set_delay_samples(PROC_BLOCK_SIZE);
    _test_module.render(1, _input, _output);
    assert_buffer(_output, 0.0f);
    _test_module.render(1, _input, _output);
    assert_buffer(_output, 1.0f);

    /* Delay == 1.5 buffers */
    _test_module.reset();
    _test_module.set_delay_samples(1.5 * PROC_BLOCK_SIZE);
    _test_module.render(1, _input, _output);
    assert_buffer(_output, 0.0f);
    _test_module.render(1, _input, _output);
    /* First half of buffer should be 0, the rest should be 1:s */
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        if (i < PROC_BLOCK_SIZE / 2)
        {
            EXPECT_FLOAT_EQ(0.0f, _output[i]);
        }
        else
        {
            EXPECT_GT(_output[i], 0.0f);
        }
    }
}

class ModDelayBrickTest : public ::testing::Test
{
protected:
    ModDelayBrickTest() {}

    void SetUp()override
    {
        fill_buffer(_input, 1.0f);
    }

    AudioBuffer          _input;
    AudioBuffer          _output;
    ModDelayBrick<LinearInterpolation<float>>  _test_module{std::chrono::milliseconds(200)};

};

TEST_F(ModDelayBrickTest, OperationTest)
{
    _test_module.render(0.5, _input, _output);
    assert_buffer(_output, 0.0f);
}

TEST_F(ModDelayBrickTest, AccuracyTest)
{
    /* Delay == 1 buffer */
    _test_module.set_delay_samples(PROC_BLOCK_SIZE);
    _test_module.render(0.5, _input, _output);
    assert_buffer(_output, 0.0f);
    _test_module.render(0.5, _input, _output);
    assert_buffer(_output, 1.0f);

    /* Modulate delay to 1.5 buffers */
    _test_module.reset();
    _test_module.render(0.75, _input, _output);
    assert_buffer(_output, 0.0f);
    _test_module.render(0.75, _input, _output);
    /* First half of buffer should be 0, the rest should be 1:s */
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        if (i < PROC_BLOCK_SIZE / 2)
        {
            EXPECT_FLOAT_EQ(0.0f, _output[i]);
        }
        else
        {
            EXPECT_GT(_output[i], 0.0f);
        }
    }
}

class ModulatedDelayBrickTest : public ::testing::Test
{
protected:
    ModulatedDelayBrickTest() {}

    void SetUp()override
    {
        fill_buffer(_input, 1.0f);
    }

    AudioBuffer           _input;
    AudioBuffer           _output;
    ModulatedDelayBrick<> _test_module{1.0f};
};

TEST_F(ModulatedDelayBrickTest, OperationTest)
{
    _test_module.render(0.5, _input, _output);
    assert_buffer(_output, 0.0f);
}

class AllpassDelayBrickTest : public ::testing::Test
{
protected:
    AllpassDelayBrickTest() {}

    void SetUp()override
    {
        fill_buffer(_input, 0.0f);
        _input[0] = 1.0;
    }

    AudioBuffer          _input;
    AudioBuffer          _output;
    float                _delay_time{1.0};
    float                _gain{0.5};
    AllpassDelayBrick<4>  _test_module;
};

TEST_F(AllpassDelayBrickTest, OperationTest)
{
    _test_module.render(1.0f, 0.5, _input, _output);
    EXPECT_FLOAT_EQ(_output[0], -0.5f);
    EXPECT_FLOAT_EQ(_output[1], 0.0f);
    EXPECT_FLOAT_EQ(_output[2], 0.0f);
    EXPECT_FLOAT_EQ(_output[3], 0.0f);
    EXPECT_FLOAT_EQ(_output[4], 0.75f); // Second impulse here
    EXPECT_FLOAT_EQ(_output[5], 0.0f);

    /* Modulate the delay */
    SetUp();
    _delay_time = 0.5f;
    _test_module.reset();
    _test_module.render(0.5f, 0.5, _input, _output);
    EXPECT_FLOAT_EQ(_output[0], -0.5f);
    EXPECT_FLOAT_EQ(_output[1], 0.0f);
    EXPECT_FLOAT_EQ(_output[2], 0.75f);  // Second impulse here
    EXPECT_FLOAT_EQ(_output[3], 0.0f);
    EXPECT_FLOAT_EQ(_output[4], 0.375f); // Third impulse here
    EXPECT_FLOAT_EQ(_output[5], 0.0f);
}

class BitRateReducerBrickTest : public ::testing::Test
{
protected:
    BitRateReducerBrickTest() {}

    void SetUp()override
    {
        make_test_sine_wave(_input);
    }

    AudioBuffer           _input;
    AudioBuffer           _output;
    BitRateReducerBrick<> _test_module;
};

bool is_2_bit_predicate(float val)
{
    return val == 0 || val == -1.0f || val == 1.0f;
}

TEST_F(BitRateReducerBrickTest, OperationTest)
{
    _test_module.render(1.0f, _input, _output);
    /* 24 bits should be within the float error margin so they should compare equal */
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        EXPECT_FLOAT_EQ(_input[i], _output[i]);
    }
    /* With bits = 0 we should have only 1 bit left, values should be 0.0 or +/- 1.0
     * Which is 1,5 bits in reality. But it sounds better if it's symmetrical around 0 */
    _test_module.render(0, _input, _output);
    for (auto sample : _output)
    {
        EXPECT_PRED1(is_2_bit_predicate, sample);
    }
}

class SampleRateReducerBrickTest : public ::testing::Test
{
protected:
    SampleRateReducerBrickTest() {}

    void SetUp() override
    {
        make_test_sine_wave(_input);
    }

    AudioBuffer               _input;
    AudioBuffer               _output;
    SampleRateReducerBrick<>  _test_module;
};

TEST_F(SampleRateReducerBrickTest, OperationTest)
{
    _test_module.render(1, _input, _output);
    /* Just test bibo stability at this point */
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        EXPECT_LE(-1.0f, _output[i]);
        EXPECT_GE(1.0f, _output[i]);
    }
    _test_module.render(0, _input, _output);
    for (int i = 0; i < PROC_BLOCK_SIZE; ++i)
    {
        EXPECT_LE(-1.0f, _output[i]);
        EXPECT_GE(1.0f, _output[i]);
    }
}
