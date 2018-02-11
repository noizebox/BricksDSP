#include "gtest/gtest.h"
#define private public

#include "envelope_bricks.cpp"

using namespace bricks;

class AudioRateAdsrEnvelopeBrickTest : public ::testing::Test
{
protected:
    AudioRateAdsrEnvelopeBrickTest() {}

    float       _attack;
    float       _decay;
    float       _sustain;
    float       _release;
    AudioRateADSRBrick    _test_module{_attack, _decay, _sustain, _release};
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
    auto& out_buffer = _test_module.audio_output(AudioRateADSRBrick::ENV_OUT);
    ASSERT_FALSE(_test_module.finished());

    /* Test that it is rising */
    float prev = 0.0f;
    for (auto& next : out_buffer)
    {
        ASSERT_GT(next, prev);
        prev = next;
    }
    _test_module.render();
    _test_module.gate(false);
    _test_module.render();

    /* Test that it is falling */
    prev = 1.0f;
    for (auto& next : out_buffer)
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
    ADSREnvelopeBrick    _test_module{_attack, _decay, _sustain, _release};
};

TEST_F(AdsrEnvelopeBrickTest, OperationalTest)
{
    _attack = 0.1f;
    _decay = 0.1f;
    _sustain = 1.0f;
    _release = 0.2f;
    ASSERT_TRUE(_test_module.finished());
    _test_module.gate(true);

    ControlPort out(_test_module.control_output(ADSREnvelopeBrick::ENV_OUT));
    _test_module.render();

    /* Test that it is rising */
    ASSERT_GT(out.value(), 0.0f);
    ASSERT_FALSE(_test_module.finished());

    _attack = 0.0f;
    _test_module.render();
    float sustain_level = out.value();
    _test_module.gate(false);
    _test_module.render();
    ASSERT_FALSE(_test_module.finished());

    ASSERT_EQ(1.0f, sustain_level);
    ASSERT_LT(out.value(), 1.0f);
    ASSERT_GT(out.value(), 0.0f);

    /* Test that it is falling */

}

class LfoBrickTest : public ::testing::Test
{
protected:
    LfoBrickTest() {}

    float       _rate;
    LfoBrick    _test_module{_rate};
};

TEST_F(LfoBrickTest, OperationalTest)
{
    _rate = 0.1;
    _test_module.set_waveform(LfoBrick::Waveform::PULSE);

    ControlPort out(_test_module.control_output(LfoBrick::LFO_OUT));
    _test_module.render();

    /* Test that it is high */
    ASSERT_FLOAT_EQ(std::abs(out.value()), 0.5f);

    /* Test that it is within the required range */
    _test_module.set_waveform(LfoBrick::Waveform::SAW);
    _test_module.render();
    ASSERT_LT(out.value(), 0.5f);
    ASSERT_GT(out.value(), -0.5f);

    _test_module.set_waveform(LfoBrick::Waveform::TRIANGLE);
    _test_module.render();
    _test_module.render();
    _test_module.render();

    ASSERT_LT(out.value(), 0.5f);
    ASSERT_GT(out.value(), -0.5f);
}
