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
    BiquadFilterBrick   _test_module{_freq, _res, _buffer};
};

TEST_F(BiquadBrickTest, OperationalTest)
{
    make_test_sq_wave(_buffer);
    _freq = 0.8;
    _res = 0.4;
    _test_module.render();
    auto& out_buffer = _test_module.audio_output(BiquadFilterBrick::FILTER_OUT);
    float sum = 0;
    for (auto sample : out_buffer)
    {
        sum += std::abs(sample);
    }
    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}
