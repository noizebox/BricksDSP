#include "gtest/gtest.h"

#include "bricks_dsp/dsp_brick.h"

#ifndef BRICKS_DSP_TEST_UTILS_H
#define BRICKS_DSP_TEST_UTILS_H

template <typename T>
void fill_buffer(T& buffer, float value)
{
    for (auto& sample : buffer)
    {
        sample = value;
    }
}

template <typename T>
void make_test_sq_wave(T& buffer)
{
    int i = 0;
    for (auto& sample : buffer)
    {
        sample = (++i > 4) - 0.5f;
        i = i % 8;
    }
}

template <typename T>
void make_test_sine_wave(T& buffer)
{
    float i = 0;
    for (auto& sample : buffer)
    {
        sample = std::sin(++i * 0.5f);
    }
}

template <typename T>
void assert_buffer(const T& buffer, float value)
{
    for (const auto& sample : buffer)
    {
        EXPECT_FLOAT_EQ(value, sample);
    }
}

#endif //BRICKS_DSP_TEST_UTILS_H
