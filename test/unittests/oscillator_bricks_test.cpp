#include "gtest/gtest.h"
#define private public

#include "oscillator_bricks.h"
#include "test_utils.h"

#include "iostream"

using namespace bricks;

constexpr float TEST_SAMPLERATE = 44100;

class OscillatorBrickTest : public ::testing::Test
{
protected:
    OscillatorBrickTest() {}

    AudioBuffer       _buffer;
    OscillatorBrick<> _test_module;
};

TEST_F(OscillatorBrickTest, TestOperation)
{
    _test_module.render(0.7, _buffer);

    float sum = 0;
    for (auto sample : _buffer)
    {
        sum += sample;
        ASSERT_LE(sample, 0.5f);
        ASSERT_GE(sample, -0.5f);
    }
    ASSERT_NE(0.0f, sum);
    ASSERT_TRUE(_buffer[1] < _buffer[2]);

    _test_module.set_waveform(Waveform::PULSE);
    _test_module.render(0.7, _buffer);

    sum = 0;
    for (auto sample : _buffer)
    {
        sum += sample;
        ASSERT_FLOAT_EQ(0.5f, std::abs(sample));
    }
    ASSERT_NE(0.0f, sum);

    _test_module.set_waveform(Waveform::TRIANGLE);
    _test_module.render(0.8, _buffer);

    sum = 0;
    for (auto sample : _buffer)
    {
        sum += sample;
        EXPECT_LE(sample, 0.5f);
        EXPECT_GE(sample, -0.5f);
    }
    auto [min, max] = std::minmax_element(_buffer.begin(), _buffer.end());
    EXPECT_GE(*min, -0.5f);
    EXPECT_LE(*max, 0.5f);

    ASSERT_NE(0.0f, sum);
}



class FmOscillatorBrickTest : public ::testing::Test
{
protected:
    FmOscillatorBrickTest() {}

    AudioBuffer         _output;
    AudioBuffer         _fm_mod;
    float               _pitch;
    FmOscillatorBrick<> _test_module;
};


TEST_F(FmOscillatorBrickTest, TestOperation)
{
    _pitch = 440 / DEFAULT_SAMPLERATE;
    fill_buffer(_fm_mod, 1.0f);
    _test_module.render(_pitch, _fm_mod, _output);

    float sum = 0;
    for (auto sample : _output)
    {
        sum += sample;
        ASSERT_LE(sample, 0.5f);
        ASSERT_GE(sample, -0.5f);
    }
    ASSERT_NE(0.0f, sum);
    ASSERT_TRUE(_output[1] < _output[2]);

    _test_module.set_waveform(Waveform::PULSE);
    _test_module.render(_pitch, _fm_mod, _output);

    sum = 0;
    for (auto sample : _output)
    {
        sum += sample;
        ASSERT_FLOAT_EQ(0.5f, std::abs(sample));
    }
    ASSERT_NE(0.0f, sum);

    _test_module.set_waveform(Waveform::TRIANGLE);
    _test_module.render(_pitch, _fm_mod, _output);

    sum = 0;
    for (auto sample : _output)
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
    void SetUp() override
    {
        _table = create_wavetable(Waveform::SAW);
        _test_module.set_samplerate(TEST_SAMPLERATE);
        _test_module.set_wavetable(&_table);
    }

    AudioBuffer         _output;
    float               _pitch;
    WtOscillatorBrick<> _test_module;
    WaveTable           _table;
};


TEST_F(WtOscillatorBrickTest, TestOperation)
{
    _pitch = 440 / DEFAULT_SAMPLERATE;
    fill_buffer(_output, 1.0f);
    _test_module.render(_pitch, _output);

    float sum = 0;
    for (auto sample : _output)
    {
        sum += sample;
        ASSERT_LE(std::abs(sample), 1.0f);
    }
    ASSERT_NE(0.0f, sum);

    _test_module.set_waveform(Waveform::PULSE);
    _test_module.render(_pitch, _output);

    sum = 0;
    for (auto sample : _output)
    {
        sum += sample;
        ASSERT_LE(std::abs(sample), 1.0f);
    }
    ASSERT_NE(0.0f, sum);

    _test_module.set_waveform(Waveform::TRIANGLE);
    _test_module.render(_pitch, _output);

    sum = 0;
    for (auto sample : _output)
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

    AudioBuffer           _output;
    NoiseGeneratorBrick<> _test_module;
};

TEST_F(NoiseGeneratorBrickTest, TestOperation)
{
    _test_module.set_samplerate(TEST_SAMPLERATE);
    fill_buffer(_output, 1.0f);
    _test_module.set_waveform(NoiseGeneratorBrick<>::NoiseType::BROWN);
    _test_module.render(_output);

    float sum = 0;
    for (auto sample : _output)
    {
        sum += sample;
        ASSERT_LE(std::abs(sample), 1.0f);
    }
    ASSERT_NE(0.0f, sum);

    _test_module.set_waveform(NoiseGeneratorBrick<>::NoiseType::PINK);
    sum = 0;
    _test_module.render(_output);
    for (auto sample : _output)
    {
        sum += sample;
        ASSERT_LE(std::abs(sample), 1.0f);
    }
    ASSERT_NE(0.0f, sum);

    _test_module.set_waveform(NoiseGeneratorBrick<>::NoiseType::WHITE);
    sum = 0;
    _test_module.render(_output);
    for (auto sample : _output)
    {
        sum += sample;
        ASSERT_LE(std::abs(sample), 1.0f);
    }
    ASSERT_NE(0.0f, sum);
}