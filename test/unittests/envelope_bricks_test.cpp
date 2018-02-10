#include "gtest/gtest.h"
#define private public

#include "envelope_bricks.cpp"

using namespace bricks;

class AudioRateAdsrEnvelopeTest : public ::testing::Test
{
protected:
    AudioRateAdsrEnvelopeTest() {}

    float       _attack;
    float       _decay;
    float       _sustain;
    float       _release;
    AudioRateADSRBrick    _test_module{_attack, _decay, _sustain, _release};
};

TEST_F(AudioRateAdsrEnvelopeTest, OperationalTest)
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

class AdsrEnvelopeTest : public ::testing::Test
{
protected:
    AdsrEnvelopeTest() {}

    float       _attack;
    float       _decay;
    float       _sustain;
    float       _release;
    ADSREnvelopeBrick    _test_module{_attack, _decay, _sustain, _release};
};

TEST_F(AdsrEnvelopeTest, OperationalTest)
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

