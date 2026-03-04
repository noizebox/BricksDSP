#include "gtest/gtest.h"
#define private public

#include "bricks_dsp/utility_bricks.h"
#include "test_utils.h"

using namespace bricks;

class VcaBrickTest : public ::testing::Test
{
protected:
    VcaBrickTest() {}

    AudioBuffer _in_buffer;
    AudioBuffer _out_buffer;
    float       _gain;
    VcaBrick<Response::LOG>  _test_module;
};

TEST_F(VcaBrickTest, OperationalTest)
{
    fill_buffer(_in_buffer, 0.5f);
    _gain = 2.0f;
    _test_module.render(_gain, _in_buffer, _out_buffer);
    float last = -1;
    for (auto& sample : _out_buffer)
    {
        ASSERT_LT(last, sample);
        last = sample;
    }
    ASSERT_EQ(4.0f, last);
}

class AudioMixerBrickTest : public ::testing::Test
{
protected:
    AudioMixerBrickTest() {}

    AudioBuffer _buffer_1;
    AudioBuffer _buffer_2;
    AudioBuffer _buffer_out;
    float       _gain;
    AudioMixerBrick<2, Response::LINEAR>  _test_module;
};

TEST_F(AudioMixerBrickTest, OperationalTest)
{
    fill_buffer(_buffer_1, 0.5f);
    fill_buffer(_buffer_2, 0.25f);
    /* Render twice so that levels stabilise */
    _test_module.render({1.0, 2.0}, {&_buffer_1, &_buffer_2}, _buffer_out);
    _test_module.render({1.0, 2.0}, {&_buffer_1, &_buffer_2}, _buffer_out);

    /* 0.5 * 1.0 + 0.25 * 2.0 == 1 */
    assert_buffer(_buffer_out, 1.0f);
}

class StereoMixerBrickTest : public ::testing::Test
{
protected:
    StereoMixerBrickTest() {}

    AudioBuffer _in_1;
    AudioBuffer _in_2;
    AudioBuffer _out_l;
    AudioBuffer _out_r;
    float       _pan_1;
    float       _gain_1;
    float       _pan_2;
    float       _gain_2;
    StereoMixerBrick<2, Response::LOG>  _test_module;
};

TEST_F(StereoMixerBrickTest, OperationalTest)
{
    fill_buffer(_in_1, 0.5f);
    fill_buffer(_in_2, 0.25f);
    _pan_1 = 0.5f;
    _pan_2 = 0.5f;
    _gain_1 = 1.0f;
    _gain_2 = 1.0f;
    /* Render twice so that levels stabilise */
    _test_module.render({_pan_1, _gain_1, _pan_2, _gain_2}, {&_in_1, &_in_2}, _out_l, _out_r);
    _test_module.render({_pan_1, _gain_1, _pan_2, _gain_2}, {&_in_1, &_in_2}, _out_l, _out_r);
    assert_buffer(_out_l, 0.375f);
    assert_buffer(_out_r, 0.375f);

    // Tweak pan
    _pan_1 = 1.0f;
    _pan_2 = 0.0f;
    _gain_1 = 1.0f;
    _gain_2 = 1.0f;
    /* Render twice so that levels stabilise */
    _test_module.render({_pan_1, _gain_1, _pan_2, _gain_2}, {&_in_1, &_in_2}, _out_l, _out_r);
    _test_module.render({_pan_1, _gain_1, _pan_2, _gain_2}, {&_in_1, &_in_2}, _out_l, _out_r);
    assert_buffer(_out_l, 0.25f);
    assert_buffer(_out_r, 0.5f);

    // Zero gain
    _pan_1 = .75f;
    _pan_2 = 0.2f;
    _gain_1 = 0.0f;
    _gain_2 = 0.0f;
    /* Render twice so that levels stabilise */
    _test_module.render({_pan_1, _gain_1, _pan_2, _gain_2}, {&_in_1, &_in_2}, _out_l, _out_r);
    _test_module.render({_pan_1, _gain_1, _pan_2, _gain_2}, {&_in_1, &_in_2}, _out_l, _out_r);
    assert_buffer(_out_l, 0.0f);
    assert_buffer(_out_r, 0.0f);
}

class AudioSummerBrickTest : public ::testing::Test
{
protected:
    AudioSummerBrickTest() {}

    AudioBuffer _in_1;
    AudioBuffer _in_2;
    AudioBuffer _out;

    AudioSummerBrick<2> _test_module;
};

TEST_F(AudioSummerBrickTest, OperationalTest)
{
    fill_buffer(_in_1,  0.2f);
    fill_buffer(_in_2,  0.3f);
    _test_module.render({&_in_1, &_in_2}, _out);
    assert_buffer(_out, 0.5f);
}

class AudioMultiplierBrickTest : public ::testing::Test
{
protected:
    AudioMultiplierBrickTest() {}

    AudioBuffer _in_1;
    AudioBuffer _in_2;
    AudioBuffer _out;
    AudioMultiplierBrick<2> _test_module;
};

TEST_F(AudioMultiplierBrickTest, OperationalTest)
{
    fill_buffer(_in_1,  0.5f);
    fill_buffer(_in_2,  0.3f);
    _test_module.render({&_in_1, &_in_2}, _out);
    assert_buffer(_out, 0.15f);
}

class ControlMixerBrickTest : public ::testing::Test
{
protected:
    ControlMixerBrickTest() {}

    float _output;
    ControlMixerBrick<2>  _test_module;
};

TEST_F(ControlMixerBrickTest, OperationalTest)
{
    _test_module.render({1.0f, 2.0f, 0.2f, 0.3f}, _output);
    ASSERT_FLOAT_EQ(0.8f, _output);
}

class ControlSummerBrickTest : public ::testing::Test
{
protected:
    ControlSummerBrickTest() {}

    float _output;
    ControlSummerBrick<2> _test_module;
};

TEST_F(ControlSummerBrickTest, OperationalTest)
{
    _test_module.render({0.2f, 0.3f}, _output);
    ASSERT_FLOAT_EQ(0.5f, _output);
}

class ControlMultiplierBrickTest : public ::testing::Test
{
protected:
    ControlMultiplierBrickTest() {}

    float _output;
    ControlMultiplierBrick<3> _test_module;
};

TEST_F(ControlMultiplierBrickTest, OperationalTest)
{
    _test_module.render({0.5f, 0.5f, 1.0f}, _output);
    ASSERT_FLOAT_EQ(0.25f, _output);
}

TEST(MetaControlBrickTest, OperationalTest)
{
    std::array<float, 4> outputs;

    MetaControlBrick<2, 4, true> test_module;
    test_module.set_component(0, {1,1,1,1}, 0.5f);
    test_module.set_component(1, {0,-1,2,0}, 0.25f);
    test_module.set_output_clamp(0, 5);
    test_module.render({1.0f, 2.0f}, {&outputs[0], &outputs[1], &outputs[2], &outputs[3]});
    EXPECT_FLOAT_EQ(0.5f, outputs[0]);
    EXPECT_FLOAT_EQ(0.0f, outputs[1]);
    EXPECT_FLOAT_EQ(1.5f, outputs[2]);
    EXPECT_FLOAT_EQ(0.5f, outputs[3]);

    test_module.set_output_clamp(0, 1);
    test_module.render({0.0f, 4.0f}, {&outputs[0], &outputs[1], &outputs[2], &outputs[3]});
    EXPECT_FLOAT_EQ(0.0f, outputs[0]);
    EXPECT_FLOAT_EQ(0.0f, outputs[1]);
    EXPECT_FLOAT_EQ(1.0f, outputs[2]);
    EXPECT_FLOAT_EQ(0.0f, outputs[3]);
}
