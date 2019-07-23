#include "gtest/gtest.h"
#define private public

#include "oscillator_bricks.cpp"
#include "test_utils.h"

#include "iostream"

using namespace bricks;

constexpr float TEST_SAMPLERATE = 44100;

class OscillatorBrickTest : public ::testing::Test
{
protected:
    OscillatorBrickTest() {}

    AudioBuffer     _buffer;
    float           _pitch;
    OscillatorBrick _test_module{_pitch};
};

TEST_F(OscillatorBrickTest, TestOperation)
{
    _pitch = 440 / DEFAULT_SAMPLERATE;
    _test_module.render();
    auto& buffer = _test_module.audio_output(OscillatorBrick::OSC_OUT);

    float sum = 0;
    for (auto sample : buffer)
    {
        sum += sample;
        ASSERT_LE(sample, 0.5f);
        ASSERT_GE(sample, -0.5f);
    }
    ASSERT_NE(0.0f, sum);
    ASSERT_TRUE(buffer[1] < buffer[2]);

    _test_module.set_waveform(OscillatorBrick::Waveform::PULSE);
    _test_module.render();

    sum = 0;
    for (auto sample : buffer)
    {
        sum += sample;
        ASSERT_FLOAT_EQ(0.5f, std::abs(sample));
    }
    ASSERT_NE(0.0f, sum);

    _test_module.set_waveform(OscillatorBrick::Waveform::TRIANGLE);
    _test_module.render();

    sum = 0;
    for (auto sample : buffer)
    {
        sum += sample;
        ASSERT_LE(sample, 0.5f);
        ASSERT_GE(sample, -0.5f);
    }
    ASSERT_NE(0.0f, sum);
}



class FmOscillatorBrickTest : public ::testing::Test
{
protected:
    FmOscillatorBrickTest() {}

    AudioBuffer         _buffer;
    float               _pitch;
    FmOscillatorBrick   _test_module{_pitch, _buffer};
};


TEST_F(FmOscillatorBrickTest, TestOperation)
{
    _pitch = 440 / DEFAULT_SAMPLERATE;
    fill_buffer(_buffer, 1.0f);
    _test_module.render();
    auto& buffer = _test_module.audio_output(OscillatorBrick::OSC_OUT);

    float sum = 0;
    for (auto sample : buffer)
    {
        sum += sample;
        ASSERT_LE(sample, 0.5f);
        ASSERT_GE(sample, -0.5f);
    }
    ASSERT_NE(0.0f, sum);
    ASSERT_TRUE(buffer[1] < buffer[2]);

    _test_module.set_waveform(FmOscillatorBrick::Waveform::PULSE);
    _test_module.render();

    sum = 0;
    for (auto sample : buffer)
    {
        sum += sample;
        ASSERT_FLOAT_EQ(0.5f, std::abs(sample));
    }
    ASSERT_NE(0.0f, sum);

    _test_module.set_waveform(FmOscillatorBrick::Waveform::TRIANGLE);
    _test_module.render();

    sum = 0;
    for (auto sample : buffer)
    {
        sum += sample;
        ASSERT_LE(sample, 0.5f);
        ASSERT_GE(sample, -0.5f);
    }
    ASSERT_NE(0.0f, sum);
}


class WtOscillatorBrickTest : public ::testing::Test
{
protected:
    WtOscillatorBrickTest() {}

    AudioBuffer         _buffer;
    float               _pitch;
    WtOscillatorBrick   _test_module{_pitch};
};


TEST_F(WtOscillatorBrickTest, TestOperation)
{
    _pitch = 440 / DEFAULT_SAMPLERATE;
    fill_buffer(_buffer, 1.0f);
    _test_module.render();
    auto& buffer = _test_module.audio_output(WtOscillatorBrick::OSC_OUT);

    float sum = 0;
    for (auto sample : buffer)
    {
        sum += sample;
        ASSERT_LE(std::abs(sample), 1.0f);
    }
    ASSERT_NE(0.0f, sum);

    _test_module.set_waveform(WtOscillatorBrick::Waveform::PULSE);
    _test_module.render();

    sum = 0;
    for (auto sample : buffer)
    {
        sum += sample;
        ASSERT_LE(std::abs(sample), 1.0f);
    }
    ASSERT_NE(0.0f, sum);

    _test_module.set_waveform(WtOscillatorBrick::Waveform::TRIANGLE);
    _test_module.render();

    sum = 0;
    for (auto sample : buffer)
    {
        sum += sample;
        ASSERT_LE(std::abs(sample), 1.0f);
    }
    ASSERT_NE(0.0f, sum);
}


class NoiseGeneratorBrickTest : public ::testing::Test
{
protected:
    NoiseGeneratorBrickTest() {}

    AudioBuffer           _buffer;
    NoiseGeneratorBrick   _test_module;
};


TEST_F(NoiseGeneratorBrickTest, TestOperation)
{
    _test_module.set_samplerate(TEST_SAMPLERATE);
    fill_buffer(_buffer, 1.0f);
    _test_module.set_waveform(NoiseGeneratorBrick::Waveform::BROWN);
    _test_module.render();
    auto& buffer = _test_module.audio_output(WtOscillatorBrick::OSC_OUT);

    float sum = 0;
    for (auto sample : buffer)
    {
        sum += sample;
        ASSERT_LE(std::abs(sample), 1.0f);
    }
    ASSERT_NE(0.0f, sum);

    _test_module.set_waveform(NoiseGeneratorBrick::Waveform::PINK);
    sum = 0;
    _test_module.render();
    for (auto sample : buffer)
    {
        sum += sample;
        ASSERT_LE(std::abs(sample), 1.0f);
    }
    ASSERT_NE(0.0f, sum);

    _test_module.set_waveform(NoiseGeneratorBrick::Waveform::WHITE);
    sum = 0;
    _test_module.render();
    for (auto sample : buffer)
    {
        sum += sample;
        ASSERT_LE(std::abs(sample), 1.0f);
    }
    ASSERT_NE(0.0f, sum);
}