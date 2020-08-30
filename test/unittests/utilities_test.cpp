#include "gtest/gtest.h"
#define private public

#include "bricks_dsp/dsp_brick.h"
#include "bricks_dsp/utils.h"
#include "random_device.cpp"

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

TEST(ControlToBool, TestOperation)
{
    EXPECT_TRUE(control_to_bool(0.7f));
    EXPECT_TRUE(control_to_bool(14.0f));
    EXPECT_FALSE(control_to_bool(0.25f));
    EXPECT_FALSE(control_to_bool(-1.280f));
}

TEST(ControlToRange, TestOperation)
{
    EXPECT_EQ(3, control_to_range(0.3, 0, 10));
    EXPECT_EQ(0, control_to_range(0.5f, -12, 12));
    EXPECT_EQ(5, control_to_range(1.0f, -5, 5));
    EXPECT_EQ(-10, control_to_range(0.0f, -10, 100));
    EXPECT_EQ(-50, control_to_range(0.25, -75, 25));
}

TEST(ControlToFreq, TestOperation)
{
    EXPECT_FLOAT_EQ(20, control_to_freq(0.0f));
    EXPECT_FLOAT_EQ(20480, control_to_freq(1.0f));
    EXPECT_FLOAT_EQ(160, control_to_freq(0.3f));
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

TEST(RandomDeviceTest, TestOperation)
{
    RandomDevice module_under_test;
    auto na = module_under_test.get_norm();
    auto nb = module_under_test.get_norm();
    EXPECT_NE(na, nb);
    EXPECT_GE(na, -1.0f);
    EXPECT_LE(na, 1.0f);
    EXPECT_GE(nb, -1.0f);
    EXPECT_LE(nb, 1.0f);
}

TEST(RandomDeviceTest, TestUniquenes)
{
    RandomDevice dev_a;
    RandomDevice dev_b;
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_NE(dev_a.get_norm(), dev_b.get_norm());
    }
}