#include <numeric>

#include "filter_bricks.h"

#include "test_utils.h"

using namespace bricks;

class FixedFilterBrickTest : public ::testing::Test
{
protected:
    FixedFilterBrickTest() {}

    void SetUp()
    {
        make_test_sq_wave(_input);
    }

    float render_and_acc()
    {
        _test_module.render(_input, _output);
        float sum = std::accumulate(_output.begin(), _output.end(), 0.0f, [](auto sum, auto x) {return sum + std::abs(x);});
        return sum / PROC_BLOCK_SIZE;
    }

    AudioBuffer         _input;
    AudioBuffer         _output;
    FixedFilterBrick<>  _test_module;
};

TEST_F(FixedFilterBrickTest, LowpassTest)
{
    _test_module.set_coeffs(calc_lowpass(1000, 0.7, DEFAULT_SAMPLERATE));
    float sum = render_and_acc();
    /* Basic sanity check, filter does not run amok or output zeroes */
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}


TEST_F(FixedFilterBrickTest, HighpassTest)
{
    _test_module.set_coeffs(calc_highpass(1000, 0.7, DEFAULT_SAMPLERATE));
    float sum = render_and_acc();
    /* Basic sanity check, filter does not run amok or output zeroes */
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}


TEST_F(FixedFilterBrickTest, AllpassTest)
{
    _test_module.set_coeffs(calc_allpass(1000, 0.7, DEFAULT_SAMPLERATE));
    float sum = render_and_acc();
    /* Basic sanity check, filter does not run amok or output zeroes */
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}

class MultiStageFilterBrickTest : public ::testing::Test
{
protected:
    MultiStageFilterBrickTest() {}

    void SetUp()
    {
        make_test_sq_wave(_input);
    }

    AudioBuffer               _input;
    AudioBuffer               _output;
    MultiStageFilterBrick<3>  _test_module;
};

TEST_F(MultiStageFilterBrickTest, OperationalTest)
{
    auto coeff = calc_lowpass(2000, DEFAULT_Q, DEFAULT_SAMPLERATE);
    std::array<Coefficients, 3> coeffs;
    coeffs.fill(coeff);

    _test_module.set_coeffs(coeffs);
    _test_module.render(_input, _output);

    float sum = std::accumulate(_output.begin(), _output.end(), 0.0f, [](auto sum, auto x) {return sum + std::abs(x);});

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
        make_test_sq_wave(_input);
    }

    AudioBuffer              _input;
    AudioBuffer              _output;
    PipelinedFilterBrick<4>  _test_module;
};

TEST_F(PipelinedFilterBrickTest, OperationalTest)
{
    auto coeff = calc_lowpass(5000, DEFAULT_Q, DEFAULT_SAMPLERATE);
    std::array<Coefficients, 4> coeffs;
    coeffs.fill(coeff);

    _test_module.set_coeffs(coeffs);
    _test_module.render(_input, _output);

    float sum = std::accumulate(_output.begin(), _output.end(), 0.0f, [](auto sum, auto x) {return sum + std::abs(x);});

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
        make_test_sq_wave(_in_buffers[0]);
        _in_buffers[1].fill(0.0f);
    }
    ParallelFilterBrick<2>       _test_module;
    std::array<AudioBuffer, 2>   _in_buffers;
    std::array<AudioBuffer, 2>   _out_buffers;
};

TEST_F(ParallelFilterBrickTest, OperationalTest)
{
    _test_module.reset();
    auto coeff = calc_highpass(500, DEFAULT_Q, DEFAULT_SAMPLERATE);
    _test_module.set_coeffs(coeff);

    _test_module.render({&_in_buffers[0], &_in_buffers[1]}, {&_out_buffers[0], &_out_buffers[1]});
    float sum = std::accumulate(_out_buffers[0].begin(), _out_buffers[0].end(), 0.0f, [](auto sum, auto x) {return sum + std::abs(x);});

    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.10f);

    for (auto sample : _out_buffers[1])
    {
        /* second channels should be zero in - zero out (check there's no crosstalk */
        EXPECT_FLOAT_EQ(0.0f, sample);
    }
}

class SVFFilterBrickTest : public ::testing::Test
{
protected:
    SVFFilterBrickTest() {}

    void SetUp()
    {
        make_test_sq_wave(_input);
        _lp_out.fill(0.0f);
        _bp_out.fill(0.0f);
        _hp_out.fill(0.0f);
    }

    AudioBuffer        _input;
    AudioBuffer        _lp_out;
    AudioBuffer        _bp_out;
    AudioBuffer        _hp_out;
    SVFFilterBrick<>   _test_module;
};

TEST_F(SVFFilterBrickTest, OperationalTest)
{
    _test_module.render(0.7, 0.6, _input, _lp_out, _bp_out,_hp_out);
    float sum = std::accumulate(_lp_out.begin(), _lp_out.end(), 0.0f, [](auto s, auto x) {return s + std::abs(x);});

    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}

class MystransLadderBrickTest : public ::testing::Test
{
protected:
    MystransLadderBrickTest() {}

    void SetUp()
    {
        make_test_sq_wave(_input);
        _output.fill(0.0f);
    }

    AudioBuffer             _input;
    AudioBuffer             _output;
    MystransLadderFilter<>  _test_module;
};

TEST_F(MystransLadderBrickTest, OperationalTest)
{
    _test_module.render(0.8, 0.6, _input,_output);
    float sum = std::accumulate(_output.begin(), _output.end(), 0.0f, [](auto s, auto x) {return s + std::abs(x);});

    /* Basic sanity check, filter does not run amok or output zeroes */
    sum /= PROC_BLOCK_SIZE;
    ASSERT_LT(sum, 1.0f);
    ASSERT_GT(sum, 0.01f);
}