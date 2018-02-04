#include "gtest/gtest.h"
#define private public

#include "dsp_brick.h"
#include "utils.h"

using namespace bricks;


TEST(LinearInterpolatorTest, TestOperation)
{
    LinearInterpolator<DSP_BRICKS_BLOCK_SIZE> _module_under_test;
    AudioBuffer buffer;
    ASSERT_FLOAT_EQ(0.0f, _module_under_test.get());
    _module_under_test.set(2.0f);
    for (auto& sample : buffer)
    {
        sample = _module_under_test.get();
    }
    /* It should equal target in the last sample and be at 50% half way */
    ASSERT_FLOAT_EQ(2.0f, buffer[DSP_BRICKS_BLOCK_SIZE -1]);
    ASSERT_FLOAT_EQ(1.0f, buffer[DSP_BRICKS_BLOCK_SIZE / 2 -1]);
}


TEST(OnePoleLagTest, TestOperation)
{
    OnePoleLag<16> _module_under_test;
    AudioBuffer buffer;
    ASSERT_FLOAT_EQ(0.0f, _module_under_test.get());
    _module_under_test.set(2.0f);
    for (auto& sample : buffer)
    {
        sample = _module_under_test.get();
    }
    /* Check that it is at least 90% of the way towards the value */
    ASSERT_NEAR(2.0f, buffer[DSP_BRICKS_BLOCK_SIZE - 1], 0.2f);
    ASSERT_GT(buffer[DSP_BRICKS_BLOCK_SIZE / 2 - 1], 1.0f);
}
