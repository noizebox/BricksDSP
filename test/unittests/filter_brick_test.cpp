#include "filter_bricks.cpp"

#include "test_utils.h"

using namespace bricks;

class BiquadBrickTest : public ::testing::Test
{
protected:
    BiquadBrickTest() {}

    AudioBuffer         _buffer;
    float               _freq;
    float               _res;
    float               _gain;
    BiquadFilterBrick   _test_module{&_freq, &_res, &_gain, &_buffer};
};

TEST_F(BiquadBrickTest, OperationalTest)
{
    make_test_sq_wave(_buffer);
    _freq = 0.8;
    _res = 0.4;
    _test_module.render();
    const AudioBuffer* out_buffer = _test_module.audio_output(BiquadFilterBrick::FILTER_OUT);
    float sum = 0;
    for (const auto sample : *out_buffer)
    {
        sum += std::abs(sample);
    }
    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}

class FixedFilterBrickTest : public ::testing::Test
{
protected:
    FixedFilterBrickTest() {}

    void SetUp()
    {
        make_test_sq_wave(_buffer);
    }

    AudioBuffer         _buffer;
    FixedFilterBrick    _test_module{&_buffer};
    const AudioBuffer*  _out_buffer{_test_module.audio_output(FixedFilterBrick::FILTER_OUT)};
};

TEST_F(FixedFilterBrickTest, LowpassTest)
{
    _test_module.set_lowpass(1000, 0.7);
    _test_module.render();
    float sum = 0;
    for (auto sample : *_out_buffer)
    {
        sum += std::abs(sample);
    }
    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}


TEST_F(FixedFilterBrickTest, HighpassTest)
{
    _test_module.set_highpass(1000, 0.7);
    _test_module.render();
    float sum = 0;
    for (auto sample : *_out_buffer)
    {
        sum += std::abs(sample);
    }
    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}


TEST_F(FixedFilterBrickTest, AllpassTest)
{
    _test_module.set_allpass(1000, 0.7);
    _test_module.render();
    float sum = 0;
    for (auto sample : *_out_buffer)
    {
        sum += std::abs(sample);
    }
    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}