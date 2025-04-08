#include "gtest/gtest.h"

#include "bricks_dsp/analyzer_bricks.h"

#include "test_utils.h"

using namespace bricks;

class MeterBrickTest : public ::testing::Test
{
protected:
    MeterBrickTest() {}

    AudioBuffer    _buffer;
    MeterBrick<>   _test_module{&_buffer};
};

TEST_F(MeterBrickTest, OperationalTest)
{
    _buffer.fill(0.0f);
    _test_module.render();

    EXPECT_NEAR(0.0, *_test_module.control_output(MeterBrick<>::ControlOutput::RMS), 0.001);
    EXPECT_NEAR(0.0, *_test_module.control_output(MeterBrick<>::ControlOutput::PEAK), 0.001);

    make_test_sq_wave(_buffer);
    for (int i = 0; i < PROC_BLOCK_SIZE * 4; ++i)
    {
        _test_module.render();
    }

    EXPECT_NEAR(0.8, *_test_module.control_output(MeterBrick<>::ControlOutput::RMS), 0.05);
    EXPECT_NEAR(0.5, *_test_module.control_output(MeterBrick<>::ControlOutput::PEAK), 0.05);
}
