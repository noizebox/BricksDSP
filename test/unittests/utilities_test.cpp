#include "gtest/gtest.h"
#define private public

#include "bricks_dsp/dsp_brick.h"
#include "bricks_dsp/utils.h"
#include "random_device.cpp"
#include "test_utils.h"

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

TEST(TodB, TestOperation)
{
    EXPECT_FLOAT_EQ(1.0f, to_db(1.0f));
    EXPECT_FLOAT_EQ(0.001f, to_db(0.0f));
    EXPECT_NEAR(2.0f, to_db(1.0f + 6.0 / 60.0), 0.01);
    EXPECT_NEAR(0.25f, to_db(1.0f - 12.0 / 60.0), 0.01);
}

TEST(FromdB, TestOperation)
{
    EXPECT_FLOAT_EQ(1.0f, from_db(1.0f));
    EXPECT_FLOAT_EQ(0.0f, from_db(0.001f));
    EXPECT_NEAR(1.0f + 6.0 / 60.0, from_db(2), 0.01);
    EXPECT_NEAR(1.0f - 12.0 / 60.0, from_db(0.25f), 0.01);
}

TEST(TodBApprox, TestOperation)
{
    /* Approximates a 30 db range in 0 to 1 */
    EXPECT_FLOAT_EQ(1.0f, to_db_approx(1.0f));
    EXPECT_FLOAT_EQ(0.0f, to_db_approx(0.0f));
    EXPECT_NEAR(0.5f, to_db_approx(1.0f - 12.0 / 60.0), 0.05);
    EXPECT_NEAR(0.10f, to_db_approx(0.33), 0.07);
}

TEST(FromdBApprox, TestOperation)
{
    /* Approximates a 30 db range in 0 to 1 */
    EXPECT_FLOAT_EQ(1.0f, from_db_approx(1.0f));
    EXPECT_FLOAT_EQ(0.0f, from_db_approx(0.0f));
    EXPECT_NEAR(1.0f + 6.0 / 30.0, from_db_approx(2), 0.06);
    EXPECT_NEAR(1.0f - 12.0 / 30.0, from_db_approx(0.25f), 0.06);
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

std::array<float, 10> INT_DATA = {1, 2, 3, 4, 5, 1, 1, 1, 0, 1};

TEST(InterpolatorTest, TestZeroth)
{
    EXPECT_EQ(2, zeroth_int(1.2f, INT_DATA.data()));
    EXPECT_EQ(2, zeroth_int(1.99f, INT_DATA.data()));
    EXPECT_EQ(4, zeroth_int(3.01f, INT_DATA.data()));
}

TEST(InterpolatorTest, TestLinear)
{
    EXPECT_FLOAT_EQ(2.5, linear_int(1.5f, INT_DATA.data()));
    EXPECT_FLOAT_EQ(1, linear_int(5.24f, INT_DATA.data()));
    EXPECT_FLOAT_EQ(4, linear_int(3.00f, INT_DATA.data()));
    EXPECT_FLOAT_EQ(1.25, linear_int(0.25f, INT_DATA.data()));
}

TEST(InterpolatorTest, TestCubic)
{
    EXPECT_FLOAT_EQ(2.5, cubic_int(1.5f, INT_DATA.data()));

    EXPECT_GE(cubic_int(5.24f, INT_DATA.data()), 0);
    EXPECT_LE(cubic_int(5.24f, INT_DATA.data()), 1);

    EXPECT_GE(cubic_int(3.30f, INT_DATA.data()), 4);
    EXPECT_LE(cubic_int(3.30f, INT_DATA.data()), 5);

    EXPECT_GE(cubic_int(7.0f, INT_DATA.data()), 0.5);
    EXPECT_LE(cubic_int(7.0f, INT_DATA.data()), 1);
}

TEST(InterpolatorTest, TestCatmullRom)
{
    EXPECT_FLOAT_EQ(2.5, catmull_rom_cubic_int(1.5f, INT_DATA.data()));

    EXPECT_GE(catmull_rom_cubic_int(5.24f, INT_DATA.data()), 0);
    EXPECT_LE(catmull_rom_cubic_int(5.24f, INT_DATA.data()), 1);

    EXPECT_GE(catmull_rom_cubic_int(3.25f, INT_DATA.data()), 4);
    EXPECT_LE(catmull_rom_cubic_int(3.25f, INT_DATA.data()), 5);

    EXPECT_GE(catmull_rom_cubic_int(7.0f, INT_DATA.data()), 0.5);
    EXPECT_LE(catmull_rom_cubic_int(7.0f, INT_DATA.data()), 1);
}

TEST(InterpolatorTest, TestCosine)
{
    EXPECT_FLOAT_EQ(2.5, cosine_int(1.5f, INT_DATA.data()));

    EXPECT_GE(cosine_int(5.24f, INT_DATA.data()), 0);
    EXPECT_LE(cosine_int(5.24f, INT_DATA.data()), 1);

    EXPECT_GE(cosine_int(3.5f, INT_DATA.data()), 4);
    EXPECT_LE(cosine_int(3.5f, INT_DATA.data()), 5);

    EXPECT_GE(cosine_int(7.0f, INT_DATA.data()), 0.5);
    EXPECT_LE(cosine_int(7.0f, INT_DATA.data()), 1);
}

TEST(DownSamplerTest, TestSkipDownsampler)
{
    AudioBuffer buffer;
    make_test_sq_wave(buffer);
    AlignedArray<float, PROC_BLOCK_SIZE / 4> downsampled;
    skip_downsample<32,8>(buffer, downsampled);

    for (auto sample :  downsampled)
    {
        EXPECT_GT(std::abs(sample), 0);
    }
}

TEST(UpSamplerTest, TestZeroStuffUpSampler)
{
    AlignedArray<float, PROC_BLOCK_SIZE / 4> buffer;
    AudioBuffer upsampled;
    buffer.fill(1);
    zero_stuff_upsample<8, 32>(buffer, upsampled);

    for (int i = 0; i < upsampled.size(); i += 4)
    {
        EXPECT_FLOAT_EQ(1, upsampled[i]);
        EXPECT_FLOAT_EQ(0, upsampled[i + 1]);
        EXPECT_FLOAT_EQ(0, upsampled[i + 2]);
        EXPECT_FLOAT_EQ(0, upsampled[i + 3]);
    }
}

TEST(UpSamplerTest, TestLinearUpSampler)
{
    AlignedArray<float, PROC_BLOCK_SIZE / 4> buffer;
    AudioBuffer upsampled;
    bool val = false;
    float mem = 0;
    /* prepare a square wave */
    for (auto& i : buffer)
    {
        i = val = !val;
    }
    linear_upsample<8, 32>(buffer, upsampled, mem);

    EXPECT_FLOAT_EQ(0.25, upsampled[0]);
    EXPECT_FLOAT_EQ(0.50, upsampled[1]);
    EXPECT_FLOAT_EQ(0.75, upsampled[2]);
    EXPECT_FLOAT_EQ(1.0,  upsampled[3]);
    EXPECT_FLOAT_EQ(0.75, upsampled[4]);
    EXPECT_FLOAT_EQ(0.5,  upsampled[5]);
    EXPECT_FLOAT_EQ(0.25, upsampled[6]);
    EXPECT_FLOAT_EQ(0.0,  upsampled[7]);

}