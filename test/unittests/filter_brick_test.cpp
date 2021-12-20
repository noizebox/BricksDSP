#include <numeric>

#include "filter_bricks.cpp"

#include "test_utils.h"

using namespace bricks;

class FixedFilterBrickTest : public ::testing::Test
{
protected:
    FixedFilterBrickTest() {}

    void SetUp()
    {
        make_test_sq_wave(_buffer);
    }

    AudioBuffer         _buffer;
    FixedFilterBrick    _test_module{&_buffer};
    const AudioBuffer*  _out_buffer{_test_module.audio_output(FixedFilterBrick::FILTER_OUT)};
};

TEST_F(FixedFilterBrickTest, LowpassTest)
{
    _test_module.set_coeffs(calc_lowpass(1000, 0.7, DEFAULT_SAMPLERATE));
    _test_module.render();

    float sum = std::accumulate(_out_buffer->begin(), _out_buffer->end(), 0.0f, [](auto sum, auto x) {return sum + std::abs(x);});

    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}


TEST_F(FixedFilterBrickTest, HighpassTest)
{
    _test_module.set_coeffs(calc_highpass(1000, 0.7, DEFAULT_SAMPLERATE));
    _test_module.render();

    float sum = std::accumulate(_out_buffer->begin(), _out_buffer->end(), 0.0f, [](auto sum, auto x) {return sum + std::abs(x);});

    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}


TEST_F(FixedFilterBrickTest, AllpassTest)
{
    _test_module.set_coeffs(calc_allpass(1000, 0.7, DEFAULT_SAMPLERATE));
    _test_module.render();

    float sum = std::accumulate(_out_buffer->begin(), _out_buffer->end(), 0.0f, [](auto sum, auto x) {return sum + std::abs(x);});

    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}

class MultiStageFilterBrickTest : public ::testing::Test
{
protected:
    MultiStageFilterBrickTest() {}

    void SetUp()
    {
        make_test_sq_wave(_buffer);
    }

    AudioBuffer               _buffer;
    MultiStageFilterBrick<3>  _test_module{&_buffer};
    const AudioBuffer*        _out_buffer{_test_module.audio_output(FixedFilterBrick::FILTER_OUT)};
};

TEST_F(MultiStageFilterBrickTest, OperationalTest)
{
    auto coeff = calc_lowpass(2000, DEFAULT_Q, DEFAULT_SAMPLERATE);
    std::array<Coefficients, 3> coeffs;
    coeffs.fill(coeff);

    make_test_sq_wave(_buffer);
    _test_module.set_coeffs(coeffs);
    _test_module.render();

    float sum = std::accumulate(_out_buffer->begin(), _out_buffer->end(), 0.0f, [](auto sum, auto x) {return sum + std::abs(x);});

    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}

class PipelinedFilterBrickTest : public ::testing::Test
{
protected:
    PipelinedFilterBrickTest() {}

    void SetUp()
    {
        make_test_sq_wave(_buffer);
    }

    AudioBuffer              _buffer;
    PipelinedFilterBrick<4>  _test_module{&_buffer};
    const AudioBuffer*       _out_buffer{_test_module.audio_output(FixedFilterBrick::FILTER_OUT)};
};

TEST_F(PipelinedFilterBrickTest, OperationalTest)
{
    auto coeff = calc_lowpass(5000, DEFAULT_Q, DEFAULT_SAMPLERATE);
    std::array<Coefficients, 4> coeffs;
    coeffs.fill(coeff);

    make_test_sq_wave(_buffer);
    _test_module.set_coeffs(coeffs);
    _test_module.render();

    float sum = std::accumulate(_out_buffer->begin(), _out_buffer->end(), 0.0f, [](auto sum, auto x) {return sum + std::abs(x);});

    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}

class ParallelFilterBrickTest : public ::testing::Test
{
protected:
    ParallelFilterBrickTest() {}

    void SetUp()
    {
        make_test_sq_wave(_buffers[0]);
        _buffers[1].fill(0.0f);
    }

    std::array<AudioBuffer, 2>          _buffers;
    ParallelFilterBrick<2>              _test_module{&_buffers[0], &_buffers[1]};
    std::array<const AudioBuffer*, 2>   _out_buffers{_test_module.audio_output(0), _test_module.audio_output(1)};
};

TEST_F(ParallelFilterBrickTest, OperationalTest)
{
    _test_module.reset();
    auto coeff = calc_highpass(500, DEFAULT_Q, DEFAULT_SAMPLERATE);
    _test_module.set_coeffs(coeff);

    _test_module.render();
    float sum = std::accumulate(_out_buffers[0]->begin(), _out_buffers[0]->end(), 0.0f, [](auto sum, auto x) {return sum + std::abs(x);});

    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.10f);

    for (auto sample : *_out_buffers[1])
    {
        /* second channels should be zero in - zero out (check there's no crosstalk */
        EXPECT_FLOAT_EQ(0.0f, sample);
    }
}