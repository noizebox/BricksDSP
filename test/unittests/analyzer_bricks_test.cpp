#include "gtest/gtest.h"

#include "bricks_dsp/analyzer_bricks.h"

#include "test_utils.h"

using namespace bricks;

class MeterBrickTest : public ::testing::Test
{
protected:
    MeterBrickTest() {}

    AudioBuffer   _input;
    float         _rms_out;
    float         _peak_out;
    MeterBrick<>  _test_module;
};

TEST_F(MeterBrickTest, OperationalTest)
{
    _input.fill(0.0f);
    _test_module.render(_rms_out, _peak_out, _input);

    EXPECT_NEAR(0.0, _rms_out, 0.001);
    EXPECT_NEAR(0.0, _peak_out, 0.001);

    make_test_sine_wave(_input);
    for (int i = 0; i < PROC_BLOCK_SIZE * 1000; ++i)
    {
        _test_module.render(_rms_out, _peak_out, _input);
    }

    /* Expected value should be 0.9 as it is 3dB down on a scale where [0, 1] maps to a 30 dB range. And 1/sqrt(2) = -3dB */
    EXPECT_NEAR(0.9, _rms_out, 0.05);
    EXPECT_NEAR(1.0, _peak_out, 0.05);
}
