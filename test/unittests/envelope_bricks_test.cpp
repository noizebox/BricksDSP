#include "gtest/gtest.h"
#define private public

#include "envelope_bricks.cpp"

using namespace bricks;

constexpr float TEST_SAMPLERATE = 44100;

class AudioRateAdsrEnvelopeBrickTest : public ::testing::Test
{
protected:
    AudioRateAdsrEnvelopeBrickTest() {}

    float       _attack;
    float       _decay;
    float       _sustain;
    float       _release;
    AudioRateADSRBrick    _test_module{&_attack, &_decay, &_sustain, &_release};
};

TEST_F(AudioRateAdsrEnvelopeBrickTest, OperationalTest)
{
    _attack = 0.1f;
    _decay = 0.1f;
    _sustain = 1.0f;
    _release = 0.2f;
    ASSERT_TRUE(_test_module.finished());

    _test_module.gate(true);
    _test_module.render();
    const auto& out_buffer = _test_module.audio_output(AudioRateADSRBrick::ENV_OUT);
    ASSERT_FALSE(_test_module.finished());

    /* Test that it is rising */
    float prev = 0.0f;
    for (auto& next : *out_buffer)
    {
        ASSERT_GT(next, prev);
        prev = next;
    }
    _test_module.render();
    _test_module.gate(false);
    _test_module.render();

    /* Test that it is falling */
    prev = 1.0f;
    for (auto& next : *out_buffer)
    {
        ASSERT_LT(next, prev);
        prev = next;
    }
}

class AdsrEnvelopeBrickTest : public ::testing::Test
{
protected:
    AdsrEnvelopeBrickTest() {}

    float       _attack;
    float       _decay;
    float       _sustain;
    float       _release;
    LinearADSREnvelopeBrick    _test_module{&_attack, &_decay, &_sustain, &_release};
};

TEST_F(AdsrEnvelopeBrickTest, OperationalTest)
{
    _attack = 0.1f;
    _decay = 0.1f;
    _sustain = 1.0f;
    _release = 0.2f;
    ASSERT_TRUE(_test_module.finished());
    _test_module.gate(true);

    const float* out(_test_module.control_output(LinearADSREnvelopeBrick::ENV_OUT));
    _test_module.render();

    /* Test that it is rising */
    ASSERT_GT(*out, 0.0f);
    ASSERT_FALSE(_test_module.finished());

    _attack = 0.0f;
    _test_module.render();
    float sustain_level = *out;
    _test_module.gate(false);
    _test_module.render();
    ASSERT_FALSE(_test_module.finished());

    ASSERT_EQ(1.0f, sustain_level);
    ASSERT_LT(*out, 1.0f);
    ASSERT_GT(*out, 0.0f);

    /* Test that it is falling */

}

class LfoBrickTest : public ::testing::Test
{
protected:
    LfoBrickTest() {}

    float       _rate;
    LfoBrick    _test_module{&_rate};
};

TEST_F(LfoBrickTest, OperationalTest)
{
    _rate = 0.1;
    _test_module.set_waveform(LfoBrick::Waveform::PULSE);

    const float* out(_test_module.control_output(LfoBrick::LFO_OUT));
    _test_module.render();

    /* Test that it is high */
    ASSERT_FLOAT_EQ(std::abs(*out), 0.5f);

    /* Test that it is within the required range */
    _test_module.set_waveform(LfoBrick::Waveform::SAW);
    _test_module.render();
    ASSERT_LT(*out, 0.5f);
    ASSERT_GT(*out, -0.5f);

    _test_module.set_waveform(LfoBrick::Waveform::TRIANGLE);
    _test_module.render();
    _test_module.render();
    _test_module.render();

    ASSERT_LT(*out, 0.5f);
    ASSERT_GT(*out, -0.5f);
}


class RandLfoBrickTest : public ::testing::Test
{
protected:
    RandLfoBrickTest() {}

    void SetUp()
    {
        _test_module.set_samplerate(TEST_SAMPLERATE);
    }
    float _rate{1};
    RandLfoBrick    _test_module{&_rate};
};

TEST_F(RandLfoBrickTest, OperationalTest)
{
    const float& out = *_test_module.control_output(RandLfoBrick::RAND_OUT);
    for (int i = 0 ; i < 20; ++i)
    {
        _test_module.render();
        EXPECT_NE(0.0f, out);
        EXPECT_GT(out, -1.0f);
        EXPECT_LT(out, 1.0f);

    }
}