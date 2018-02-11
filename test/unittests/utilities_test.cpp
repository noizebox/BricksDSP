#include "gtest/gtest.h"
#define private public

#include "dsp_brick.h"
#include "utils.h"

using namespace bricks;

TEST(NoteToControl, TestOperation)
{
    /* 69 = A4 = 440 Hz */
    float control = note_to_control(69);
    float freq = 20 * powf(2, control * 10);
    EXPECT_NEAR(440.0f, freq, 0.0005f);

    /* 36 = C2 = 65.406 Hz */
    float control2 = note_to_control(36);
    freq = 20 * powf(2, control2 * 10);
    EXPECT_NEAR(65.406, freq, 0.0005f);

    /* Test 0.1 / octave */
    EXPECT_FLOAT_EQ((69 - 36) / 120.0f, control - control2);
}


TEST(LinearInterpolatorTest, TestOperation)
{
    LinearInterpolator<PROC_BLOCK_SIZE> _module_under_test;
    AudioBuffer buffer;
    ASSERT_FLOAT_EQ(0.0f, _module_under_test.get());
    _module_under_test.set(2.0f);
    for (auto& sample : buffer)
    {
        sample = _module_under_test.get();
    }
    /* It should equal target in the last sample and be at 50% half way */
    ASSERT_FLOAT_EQ(2.0f, buffer[PROC_BLOCK_SIZE -1]);
    ASSERT_FLOAT_EQ(1.0f, buffer[PROC_BLOCK_SIZE / 2 -1]);
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
    ASSERT_NEAR(2.0f, buffer[PROC_BLOCK_SIZE - 1], 0.2f);
    ASSERT_GT(buffer[PROC_BLOCK_SIZE / 2 - 1], 1.0f);
}
