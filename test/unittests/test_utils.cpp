#include <cmath>

#include "test_utils.h"

void fill_buffer(bricks::AudioBuffer& buffer, float value)
{
    for (auto& sample : buffer)
    {
        sample = value;
    }
}

void make_test_sq_wave(bricks::AudioBuffer& buffer)
{
    int i = 0;
    for (auto& sample : buffer)
    {
        sample = (++i > 3) - 0.5f;
        i = i % 6;
    }
}

void assert_buffer(const bricks::AudioBuffer& buffer, float value)
{
    for (const auto& sample : buffer)
    {
        ASSERT_FLOAT_EQ(value, sample);
    }
}