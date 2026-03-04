#include "gtest/gtest.h"
#define private public

#include "envelope_bricks.h"

using namespace bricks;

constexpr float TEST_SAMPLERATE = 44100;

class AudioRateAdsrEnvelopeBrickTest : public ::testing::Test
{
protected:
    AudioRateAdsrEnvelopeBrickTest() {}

    AudioBuffer           _output;
    AudioRateADSRBrick<>  _test_module;
};

TEST_F(AudioRateAdsrEnvelopeBrickTest, OperationalTest)
{
    ASSERT_TRUE(_test_module.finished());

    _test_module.gate(true);
    _test_module.render(0.1, 0.1, 1.0, 0.2, _output);
    ASSERT_FALSE(_test_module.finished());

    /* Test that it is rising */
    float prev = 0.0f;
    for (auto& next : _output)
    {
        ASSERT_GT(next, prev);
        prev = next;
    }
    _test_module.render(0.1, 0.1, 1.0, 0.2, _output);
    _test_module.gate(false);
    _test_module.render(0.1, 0.1, 1.0, 0.2, _output);

    /* Test that it is falling */
    prev = 1.0f;
    for (auto& next : _output)
    {
        ASSERT_LT(next, prev);
        prev = next;
    }
}

class AdsrEnvelopeBrickTest : public ::testing::Test
{
protected:
    AdsrEnvelopeBrickTest() {}

    float                    _output{0};
    LinearADSREnvelopeBrick  _test_module;
};

TEST_F(AdsrEnvelopeBrickTest, OperationalTest)
{
    ASSERT_TRUE(_test_module.finished());
    _test_module.gate(true);

    _test_module.render(0.1, 0.1, 1.0, 0.2, _output);

    /* Test that it is rising */
    ASSERT_GT(_output, 0.0f);
    ASSERT_FALSE(_test_module.finished());

    _test_module.render(0, 0.1, 1.0, 0.2, _output);
    float sustain_level = _output;
    _test_module.gate(false);
    _test_module.render(0, 0.1, 1.0, 0.2, _output);
    ASSERT_FALSE(_test_module.finished());

    ASSERT_EQ(1.0f, sustain_level);
    ASSERT_LT(_output, 1.0f);
    ASSERT_GT(_output, 0.0f);

    /* Test that it is falling */

}

class LfoBrickTest : public ::testing::Test
{
protected:
    LfoBrickTest() {}

    float       _output;
    LfoBrick    _test_module;
};

TEST_F(LfoBrickTest, OperationalTest)
{
    _test_module.reset();
    _test_module.set_waveform(LfoBrick::Waveform::PULSE);
    _test_module.render(0.1, _output);;

    /* Test that it is high */
    ASSERT_FLOAT_EQ(std::abs(_output), 0.5f);

    /* Test that it is within the required range */
    _test_module.set_waveform(LfoBrick::Waveform::SAW);
    _test_module.render(0.1, _output);
    ASSERT_LT(_output, 0.5f);
    ASSERT_GT(_output, -0.5f);

    _test_module.set_waveform(LfoBrick::Waveform::TRIANGLE);
    _test_module.render(0.1, _output);
    _test_module.render(0.1, _output);
    _test_module.render(0.1, _output);

    ASSERT_LT(_output, 0.5f);
    ASSERT_GT(_output, -0.5f);
}


class RandLfoBrickTest : public ::testing::Test
{
protected:
    RandLfoBrickTest() {}

    void SetUp()
    {
        _test_module.set_samplerate(TEST_SAMPLERATE);
    }

    float         _output{1};
    RandLfoBrick  _test_module;
};

TEST_F(RandLfoBrickTest, OperationalTest)
{
    for (int i = 0 ; i < 20; ++i)
    {
        _test_module.render(_output);
        EXPECT_NE(0.0f, _output);
        EXPECT_GT(_output, -1.0f);
        EXPECT_LT(_output, 1.0f);

    }
}