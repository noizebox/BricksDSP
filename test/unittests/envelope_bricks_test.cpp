#include "gtest/gtest.h"
#define private public

#include "envelope_bricks.cpp"

using namespace bricks;

class AdsrEnvelopeTest : public ::testing::Test
{
protected:
    AdsrEnvelopeTest() {}

    float       _attack;
    float       _decay;
    float       _sustain;
    float       _release;
    ADSREnvelopeBrick    _test_module{&_attack, &_decay, &_sustain, &_release};
};

TEST_F(AdsrEnvelopeTest, OperationalTest)
{
    _attack = 0.1f;
    _decay = 0.1f;
    _sustain = 1.0f;
    _release = 0.2f;
    _test_module.gate(true);
    _test_module.render();
    auto out_buffer = _test_module.audio_output(ADSREnvelopeBrick::ENV_OUT);
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

