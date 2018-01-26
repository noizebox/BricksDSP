#include "test_utils.h"

void fill_buffer(bricks::AudioBuffer& buffer, float value)
{
    for (auto& sample : buffer)
    {
        sample = value;
    }
}


void assert_buffer(bricks::AudioBuffer& buffer, float value)
{
    for (const auto& sample : buffer)
    {
        ASSERT_FLOAT_EQ(value, sample);
    }
}