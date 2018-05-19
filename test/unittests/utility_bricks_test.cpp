#include "gtest/gtest.h"
#define private public

#include "utility_bricks.h"
#include "test_utils.h"

using namespace bricks;

class VcaBrickTest : public ::testing::Test
{
protected:
    VcaBrickTest() {}

    AudioBuffer _buffer;
    float       _gain;
    VcaBrick    _test_module{_gain, _buffer};
};

TEST_F(VcaBrickTest, OperationalTest)
{
    fill_buffer(_buffer, 0.5f);
    _gain = 2.0f;
    _test_module.render();
    auto& out_buffer = _test_module.audio_output(VcaBrick::VCA_OUT);
    float last = 0;
    for (auto& sample : out_buffer)
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
    float       _gain_1;
    float       _gain_2;
    AudioMixerBrick<2>   _test_module{{_gain_1, _gain_2}, {_buffer_1, _buffer_2}};
};

TEST_F(AudioMixerBrickTest, OperationalTest)
{
    fill_buffer(_buffer_1, 0.5f);
    fill_buffer(_buffer_2, 0.25f);
    _gain_1 = 1.0f;
    _gain_2 = 1.0f;
    /* Render twice so that levels stabilise */
    _test_module.render();
    _test_module.render();
    assert_buffer(_test_module.audio_output(AudioMixerBrick<2>::MIX_OUT), 0.75f);
}

class AudioSummerBrickTest : public ::testing::Test
{
protected:
    AudioSummerBrickTest() {}

    AudioBuffer _in_1;
    AudioBuffer _in_2;

    AudioSummerBrick<2> _test_module{_in_1, _in_2};
};

TEST_F(AudioSummerBrickTest, OperationalTest)
{
    fill_buffer(_in_1,  0.2f);
    fill_buffer(_in_2,  0.3f);
    _test_module.render();

    auto& out = _test_module.audio_output(AudioSummerBrick<2>::SUM_OUT);
    assert_buffer(out, 0.5f);
}

class AudioMultiplierBrickTest : public ::testing::Test
{
protected:
    AudioMultiplierBrickTest() {}

    AudioBuffer _in_1;
    AudioBuffer _in_2;

    AudioMultiplierBrick<2> _test_module{_in_1, _in_2};
};

TEST_F(AudioMultiplierBrickTest, OperationalTest)
{
    fill_buffer(_in_1,  0.5f);
    fill_buffer(_in_2,  0.3f);
    _test_module.render();

    auto& out = _test_module.audio_output(AudioMultiplierBrick<2>::MULT_OUT);
    assert_buffer(out, 0.15f);
}

class ControlMixerBrickTest : public ::testing::Test
{
protected:
    ControlMixerBrickTest() {}

    float       _in_1;
    float       _in_2;
    float       _gain_1;
    float       _gain_2;

    ControlMixerBrick<2>  _test_module{_gain_1, _gain_2,_in_1, _in_2};
};

TEST_F(ControlMixerBrickTest, OperationalTest)
{
    _in_1 = 0.2f;
    _in_2 = 0.3f;
    _gain_1 = 1.0f;
    _gain_2 = 2.0f;
    _test_module.render();

    auto& out = _test_module.control_output(ControlMixerBrick<2>::MIX_OUT);
    ASSERT_FLOAT_EQ(0.8f, out);
}


class ControlSummerBrickTest : public ::testing::Test
{
protected:
    ControlSummerBrickTest() {}

    float       _in_1;
    float       _in_2;

    ControlSummerBrick<2>  _test_module{_in_1, _in_2};
};

TEST_F(ControlSummerBrickTest, OperationalTest)
{
    _in_1 = 0.2f;
    _in_2 = 0.3f;
    _test_module.render();

    ASSERT_FLOAT_EQ(0.2f, _test_module._inputs[0].value());
    auto& out = _test_module.control_output(ControlSummerBrick<2>::SUM_OUT);
    ASSERT_FLOAT_EQ(0.5f, out);
}

class ControlMultiplierBrickTest : public ::testing::Test
{
protected:
    ControlMultiplierBrickTest() {}

    float       _in_1;
    float       _in_2;
    float       _in_3;

    ControlMultiplierBrick<3>  _test_module{_in_1, _in_2, _in_3};
};

TEST_F(ControlMultiplierBrickTest, OperationalTest)
{
    _in_1 = 0.5f;
    _in_2 = 0.5f;
    _in_3 = 1.0f;
    _test_module.render();

    auto& out = _test_module.control_output(ControlMultiplierBrick<3>::MULT_OUT);
    ASSERT_FLOAT_EQ(0.25f, out);
}

TEST(MetaControlBrickTest, OperationalTest)
{
    float a = 1;
    float b = 2;
    MetaControlBrick<2, 4, true> module_under_test(a, b);
    module_under_test.set_component(0, {1,1,1,1}, 0.5f);
    module_under_test.set_component(1, {0,-1,2,0}, 0.25f);
    module_under_test.set_output_clamp(0, 5);
    module_under_test.render();
    EXPECT_FLOAT_EQ(0.5f, module_under_test.control_output(0));
    EXPECT_FLOAT_EQ(0.0f, module_under_test.control_output(1));
    EXPECT_FLOAT_EQ(1.5f, module_under_test.control_output(2));
    EXPECT_FLOAT_EQ(0.5f, module_under_test.control_output(3));

    a = 0;
    b = 4;
    module_under_test.set_output_clamp(0, 1);
    module_under_test.render();
    EXPECT_FLOAT_EQ(0.0f, module_under_test.control_output(0));
    EXPECT_FLOAT_EQ(0.0f, module_under_test.control_output(1));
    EXPECT_FLOAT_EQ(1.0f, module_under_test.control_output(2));
    EXPECT_FLOAT_EQ(0.0f, module_under_test.control_output(3));
}